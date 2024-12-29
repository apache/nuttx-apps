/****************************************************************************
 * apps/system/iptables/iptables.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <arpa/inet.h>

#include <nuttx/net/netfilter/ip_tables.h>

#include "iptables.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE int
  (*iptables_command_func_t)(FAR const struct iptables_args_s *args);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iptables_addr2str
 *
 * Description:
 *   Format address and mask to ip/preflen string.
 *
 ****************************************************************************/

static FAR char *iptables_addr2str(struct in_addr addr, struct in_addr msk,
                                   FAR char *buf, size_t bufflen)
{
  unsigned int preflen = popcount(msk.s_addr);
  if (preflen != 0)
    {
      snprintf(buf, bufflen, "%s/%d", inet_ntoa(addr), preflen);
    }
  else
    {
      snprintf(buf, bufflen, "anywhere");
    }

  return buf;
}

/****************************************************************************
 * Name: iptables_print_chain
 *
 * Description:
 *   Print all rules in a chain
 *
 ****************************************************************************/

static void iptables_print_chain(FAR const struct ipt_replace *repl,
                                 enum nf_inet_hooks hook)
{
  /* Format:         target !prot   !idev   !odev   !saddr   !daddr  match */

  const char fmt[] = "%-12s %1s%-4s %1s%-4s %1s%-4s %1s%-18s %1s%-18s %s\n";
  char src[INET_ADDRSTRLEN + 3]; /* Format: 123.123.123.123/24 */
  char dst[INET_ADDRSTRLEN + 3];

  FAR struct ipt_entry *entry;
  FAR struct xt_entry_match *match;
  FAR struct xt_entry_target *target;
  FAR uint8_t *head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  int size = repl->underflow[hook] - repl->hook_entry[hook];

  /* The underflow entry contains the default rule. */

  entry  = (FAR struct ipt_entry *)(head + size);
  target = IPT_TARGET(entry);

  printf("Chain %s (policy %s)\n",
         iptables_hook2str(hook), iptables_target2str(target));
  printf(fmt, "target", "", "prot", "", "idev", "", "odev",
                        "", "source", "", "destination", "");

  ipt_entry_for_every(entry, head, size)
    {
      target = IPT_TARGET(entry);
      match  = entry->target_offset >= sizeof(struct xt_entry_match) ?
                                                    IPT_MATCH(entry) : NULL;
      printf(fmt, iptables_target2str(target),
          INV_FLAG_STR(entry->ip.invflags & IPT_INV_PROTO),
          iptables_proto2str(entry->ip.proto),
          INV_FLAG_STR(entry->ip.invflags & IPT_INV_VIA_IN),
          iptables_iface2str(entry->ip.iniface),
          INV_FLAG_STR(entry->ip.invflags & IPT_INV_VIA_OUT),
          iptables_iface2str(entry->ip.outiface),
          INV_FLAG_STR(entry->ip.invflags & IPT_INV_SRCIP),
          iptables_addr2str(entry->ip.src, entry->ip.smsk, src, sizeof(src)),
          INV_FLAG_STR(entry->ip.invflags & IPT_INV_DSTIP),
          iptables_addr2str(entry->ip.dst, entry->ip.dmsk, dst, sizeof(dst)),
          iptables_match2str(match));
    }

  printf("\n");
}

/****************************************************************************
 * Name: iptables_list
 *
 * Description:
 *   List all rules in a table
 *
 ****************************************************************************/

static int iptables_list(FAR const char *table, enum nf_inet_hooks hook)
{
  FAR struct ipt_replace *repl = netlib_ipt_prepare(table);
  unsigned int cur_hook;

  if (repl == NULL)
    {
      printf("Failed to read table %s from kernel!\n", table);
      return -EIO;
    }

  for (cur_hook = 0; cur_hook < NF_INET_NUMHOOKS; cur_hook++)
    {
      if ((repl->valid_hooks & (1 << cur_hook)) != 0 &&
          (hook == NF_INET_NUMHOOKS || hook == cur_hook))
        {
          iptables_print_chain(repl, cur_hook);
        }
    }

  free(repl);
  return OK;
}

/****************************************************************************
 * Name: iptables_finish_command
 *
 * Description:
 *   Do a command and commit it
 *
 ****************************************************************************/

static int iptables_finish_command(FAR const struct iptables_args_s *args,
                                   FAR struct ipt_replace **repl,
                                   FAR struct ipt_entry *entry)
{
  int ret;

  switch (args->cmd)
    {
      case COMMAND_APPEND:
        ret = netlib_ipt_append(repl, entry, args->hook);
        break;

      case COMMAND_INSERT:
        ret = netlib_ipt_insert(repl, entry, args->hook, args->rulenum);
        break;

      case COMMAND_DELETE:
        ret = netlib_ipt_delete(*repl, entry, args->hook, args->rulenum);
        break;

      default: /* Other commands should not call into this function. */
        ret = -EINVAL;
        break;
    }

  if (ret == OK)
    {
      ret = netlib_ipt_commit(*repl);
    }

  return ret;
}

/****************************************************************************
 * Name: iptables_finish_directly
 *
 * Description:
 *   Finish command directly without preparing any entry
 *
 ****************************************************************************/

static int iptables_finish_directly(FAR const struct iptables_args_s *args)
{
  FAR struct ipt_replace *repl = netlib_ipt_prepare(args->table);
  int ret;

  if (repl == NULL)
    {
      printf("Failed to read table '%s' from kernel!\n", args->table);
      return -EIO;
    }

  ret = iptables_finish_command(args, &repl, NULL);

  free(repl);
  return ret;
}

/****************************************************************************
 * Name: iptables_nat_command
 *
 * Description:
 *   Do a NAT command
 *
 ****************************************************************************/

#ifdef CONFIG_NET_NAT
static int iptables_nat_command(FAR const struct iptables_args_s *args)
{
  FAR struct ipt_replace *repl;
  FAR struct ipt_entry *entry = NULL;
  int ret;

  if (args->outifname == NULL || args->outifname[0] == '\0')
    {
      printf("Table '" TABLE_NAME_NAT "' needs an out interface!\n");
      return -EINVAL;
    }

  if (args->target != NULL &&
      strcmp(args->target, XT_MASQUERADE_TARGET))
    {
      printf("Only target '" XT_MASQUERADE_TARGET
             "' is supported for table '" TABLE_NAME_NAT "'!\n");
      return -EINVAL;
    }

  repl = netlib_ipt_prepare(TABLE_NAME_NAT);
  if (repl == NULL)
    {
      printf("Failed to read table '" TABLE_NAME_NAT "' from kernel!\n");
      return -EIO;
    }

  entry = netlib_ipt_masquerade_entry(args->outifname);
  if (entry == NULL)
    {
      printf("Failed to prepare entry for dev %s!\n", args->outifname);
      ret = -ENOMEM;
      goto errout_with_repl;
    }

  ret = iptables_finish_command(args, &repl, entry);

  free(entry);
errout_with_repl:
  free(repl);
  return ret;
}
#endif

/****************************************************************************
 * Name: iptables_filter_command
 *
 * Description:
 *   Do a FILTER command
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPFILTER
static int iptables_filter_command(FAR const struct iptables_args_s *args)
{
  FAR struct xt_entry_match *match;
  FAR struct ipt_replace *repl = netlib_ipt_prepare(XT_TABLE_NAME_FILTER);
  FAR struct ipt_entry *entry  = NULL;
  int ret;

  if (repl == NULL)
    {
      printf("Failed to read table '" XT_TABLE_NAME_FILTER
             "' from kernel!\n");
      return -EIO;
    }

  /* Get entry and fill in proto-specific details. */

  if (args->sport != NULL || args->dport != NULL)
    {
      FAR struct xt_udp *tcpudp;

      if (args->protocol != IPPROTO_TCP && args->protocol != IPPROTO_UDP)
        {
          printf("Source/destination port is only supported for TCP/UDP!\n");
          ret = -EINVAL;
          goto errout;
        }

      entry = netlib_ipt_filter_entry(args->target, args->verdict,
                                      args->protocol);
      if (entry == NULL)
        {
          goto errout_prepare_entry;
        }

      match  = IPT_MATCH(entry);
      tcpudp = (FAR struct xt_udp *)(match + 1);

      switch (args->protocol)
        {
          case IPPROTO_TCP:
            ((FAR struct xt_tcp *)tcpudp)->invflags = args->tcpudpinv;
            break;

          case IPPROTO_UDP:
            ((FAR struct xt_udp *)tcpudp)->invflags = args->tcpudpinv;
            break;
        }

      ret = iptables_parse_ports(args->sport, tcpudp->spts);
      if (ret < 0)
        {
          printf("Failed to parse source port!\n");
          goto errout;
        }

      ret = iptables_parse_ports(args->dport, tcpudp->dpts);
      if (ret < 0)
        {
          printf("Failed to parse destination port!\n");
          goto errout;
        }
    }
  else if (args->icmp_type != NULL)
    {
      FAR struct ipt_icmp *icmp;

      if (args->protocol != IPPROTO_ICMP)
        {
          printf("ICMP type is only supported for ICMP protocol!\n");
          ret = -EINVAL;
          goto errout;
        }

      entry = netlib_ipt_filter_entry(args->target, args->verdict,
                                      args->protocol);
      if (entry == NULL)
        {
          goto errout_prepare_entry;
        }

      match = IPT_MATCH(entry);
      icmp  = (FAR struct ipt_icmp *)(match + 1);

      ret = iptables_parse_icmp(args->icmp_type);
      if (ret < 0)
        {
          printf("Failed to parse ICMP type!\n");
          goto errout;
        }

      icmp->type = ret;
      icmp->invflags = args->icmpinv;
    }
  else
    {
      entry = netlib_ipt_filter_entry(args->target, args->verdict, 0);
      if (entry == NULL)
        {
          goto errout_prepare_entry;
        }
    }

  /* Fill in common details. */

  ret = netlib_ipt_fillifname(entry, args->inifname, args->outifname);
  if (ret < 0)
    {
      printf("Failed to fill in interface names!\n");
      goto errout;
    }

  if (args->saddr != NULL)
    {
      ret = iptables_parse_ip(args->saddr, &entry->ip.src, &entry->ip.smsk,
                              AF_INET);
      if (ret < 0)
        {
          printf("Failed to parse source address %s!\n", args->saddr);
          goto errout;
        }
    }

  if (args->daddr != NULL)
    {
      ret = iptables_parse_ip(args->daddr, &entry->ip.dst, &entry->ip.dmsk,
                              AF_INET);
      if (ret < 0)
        {
          printf("Failed to parse destination address %s!\n", args->daddr);
          goto errout;
        }
    }

  entry->ip.proto    = args->protocol;
  entry->ip.invflags = args->ipinv;

  /* Finish command. */

  ret = iptables_finish_command(args, &repl, entry);

errout:
  free(entry);
  free(repl);
  return ret;

errout_prepare_entry:
  printf("Failed to prepare entry!\n");
  ret = -ENOMEM;
  goto errout;
}
#endif

/****************************************************************************
 * Name: iptables_apply
 *
 * Description:
 *   Apply rules for corresponding table
 *
 ****************************************************************************/

static int iptables_apply(FAR const struct iptables_args_s *args,
                          iptables_command_func_t command_func)
{
  switch (args->cmd)
    {
      case COMMAND_FLUSH:
        return netlib_ipt_flush(args->table, args->hook);

      case COMMAND_LIST:
        return iptables_list(args->table, args->hook);

      case COMMAND_POLICY:
        return netlib_ipt_policy(args->table, args->hook, args->verdict);

      case COMMAND_DELETE:

        /* Delete rule with rulenum can be done directly. */

        if (args->rulenum > 0)
          {
            return iptables_finish_directly(args);
          }

        /* Fall through. */

      case COMMAND_APPEND:
      case COMMAND_INSERT:
        return command_func(args);

      default:
        printf("No supported command specified!\n");
        return -EINVAL;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct iptables_args_s args;
  int ret = iptables_parse(&args, argc, argv);

  if (ret < 0 || args.cmd == COMMAND_INVALID)
    {
      iptables_showusage(argv[0]);
      return ret;
    }

#ifdef CONFIG_NET_IPFILTER
  if (args.table == NULL || strcmp(args.table, XT_TABLE_NAME_FILTER) == 0)
    {
      args.table = XT_TABLE_NAME_FILTER;
      ret = iptables_apply(&args, iptables_filter_command);
      if (ret < 0)
        {
          printf("iptables got error on filter: %d!\n", ret);
        }
    }
  else
#endif
#ifdef CONFIG_NET_NAT
  if (strcmp(args.table, TABLE_NAME_NAT) == 0)
    {
      ret = iptables_apply(&args, iptables_nat_command);
      if (ret < 0)
        {
          printf("iptables got error on NAT: %d!\n", ret);
        }
    }
  else
#endif
    {
      printf("Unknown table: %s\n", args.table);
    }

  return ret;
}
