/****************************************************************************
 * apps/system/iptables/ip6tables.c
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

#include <nuttx/net/ip.h>
#include <nuttx/net/netfilter/ip6_tables.h>

#include "iptables.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE int
  (*ip6tables_command_func_t)(FAR const struct iptables_args_s *args);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ip6tables_addr2str
 *
 * Description:
 *   Format address and mask to ip/preflen string.
 *
 ****************************************************************************/

static FAR char *ip6tables_addr2str(net_ipv6addr_t addr, net_ipv6addr_t msk,
                                    FAR char *buf, size_t bufflen)
{
  unsigned int preflen = netlib_ipv6netmask2prefix(msk);
  if (preflen != 0)
    {
      char ipbuf[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, addr, ipbuf, INET6_ADDRSTRLEN);
      snprintf(buf, bufflen, "%s/%d", ipbuf, preflen);
    }
  else
    {
      snprintf(buf, bufflen, "anywhere");
    }

  return buf;
}

/****************************************************************************
 * Name: ip6tables_print_chain
 *
 * Description:
 *   Print all rules in a chain
 *
 ****************************************************************************/

static void ip6tables_print_chain(FAR const struct ip6t_replace *repl,
                                  enum nf_inet_hooks hook)
{
  /* Format:         target !prot   !idev   !odev   !saddr   !daddr  match */

  const char fmt[] = "%-12s %1s%-4s %1s%-4s %1s%-4s %1s%-18s %1s%-18s %s\n";
  char src[INET6_ADDRSTRLEN + 4]; /* Format: fc00::1/128 */
  char dst[INET6_ADDRSTRLEN + 4];

  FAR struct ip6t_entry *entry;
  FAR struct xt_entry_match *match;
  FAR struct xt_entry_target *target;
  FAR uint8_t *head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  int size = repl->underflow[hook] - repl->hook_entry[hook];

  /* The underflow entry contains the default rule. */

  entry  = (FAR struct ip6t_entry *)(head + size);
  target = IP6T_TARGET(entry);

  printf("Chain %s (policy %s)\n",
         iptables_hook2str(hook), iptables_target2str(target));
  printf(fmt, "target", "", "prot", "", "idev", "", "odev",
                        "", "source", "", "destination", "");

  ip6t_entry_for_every(entry, head, size)
    {
      target = IP6T_TARGET(entry);
      match  = entry->target_offset >= sizeof(struct xt_entry_match) ?
                                                    IP6T_MATCH(entry) : NULL;
      printf(fmt, iptables_target2str(target),
          INV_FLAG_STR(entry->ipv6.invflags & IP6T_INV_PROTO),
          iptables_proto2str(entry->ipv6.proto),
          INV_FLAG_STR(entry->ipv6.invflags & IP6T_INV_VIA_IN),
          iptables_iface2str(entry->ipv6.iniface),
          INV_FLAG_STR(entry->ipv6.invflags & IP6T_INV_VIA_OUT),
          iptables_iface2str(entry->ipv6.outiface),
          INV_FLAG_STR(entry->ipv6.invflags & IP6T_INV_SRCIP),
          ip6tables_addr2str(entry->ipv6.src.s6_addr16,
                             entry->ipv6.smsk.s6_addr16, src, sizeof(src)),
          INV_FLAG_STR(entry->ipv6.invflags & IP6T_INV_DSTIP),
          ip6tables_addr2str(entry->ipv6.dst.s6_addr16,
                             entry->ipv6.dmsk.s6_addr16, dst, sizeof(dst)),
          iptables_match2str(match));
    }

  printf("\n");
}

/****************************************************************************
 * Name: ip6tables_list
 *
 * Description:
 *   List all rules in a table
 *
 ****************************************************************************/

static int ip6tables_list(FAR const char *table, enum nf_inet_hooks hook)
{
  FAR struct ip6t_replace *repl = netlib_ip6t_prepare(table);
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
          ip6tables_print_chain(repl, cur_hook);
        }
    }

  free(repl);
  return OK;
}

/****************************************************************************
 * Name: ip6tables_finish_command
 *
 * Description:
 *   Do a command and commit it
 *
 ****************************************************************************/

static int ip6tables_finish_command(FAR const struct iptables_args_s *args,
                                    FAR struct ip6t_replace **repl,
                                    FAR struct ip6t_entry *entry)
{
  int ret;

  switch (args->cmd)
    {
      case COMMAND_APPEND:
        ret = netlib_ip6t_append(repl, entry, args->hook);
        break;

      case COMMAND_INSERT:
        ret = netlib_ip6t_insert(repl, entry, args->hook, args->rulenum);
        break;

      case COMMAND_DELETE:
        ret = netlib_ip6t_delete(*repl, entry, args->hook, args->rulenum);
        break;

      default: /* Other commands should not call into this function. */
        ret = -EINVAL;
        break;
    }

  if (ret == OK)
    {
      ret = netlib_ip6t_commit(*repl);
    }

  return ret;
}

/****************************************************************************
 * Name: ip6tables_finish_directly
 *
 * Description:
 *   Finish command directly without preparing any entry
 *
 ****************************************************************************/

static int ip6tables_finish_directly(FAR const struct iptables_args_s *args)
{
  FAR struct ip6t_replace *repl = netlib_ip6t_prepare(args->table);
  int ret;

  if (repl == NULL)
    {
      printf("Failed to read table '%s' from kernel!\n", args->table);
      return -EIO;
    }

  ret = ip6tables_finish_command(args, &repl, NULL);

  free(repl);
  return ret;
}

/****************************************************************************
 * Name: ip6tables_filter_command
 *
 * Description:
 *   Do a FILTER command
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPFILTER
static int ip6tables_filter_command(FAR const struct iptables_args_s *args)
{
  FAR struct xt_entry_match *match;
  FAR struct ip6t_replace *repl = netlib_ip6t_prepare(XT_TABLE_NAME_FILTER);
  FAR struct ip6t_entry *entry  = NULL;
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

      entry = netlib_ip6t_filter_entry(args->target, args->verdict,
                                       args->protocol);
      if (entry == NULL)
        {
          goto errout_prepare_entry;
        }

      match  = IP6T_MATCH(entry);
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
      FAR struct ip6t_icmp *icmp6;

      if (args->protocol != IPPROTO_ICMP6)
        {
          printf("ICMPv6 type is only supported for ICMPv6 protocol!\n");
          ret = -EINVAL;
          goto errout;
        }

      entry = netlib_ip6t_filter_entry(args->target, args->verdict,
                                       args->protocol);
      if (entry == NULL)
        {
          goto errout_prepare_entry;
        }

      match = IP6T_MATCH(entry);
      icmp6 = (FAR struct ip6t_icmp *)(match + 1);

      ret = iptables_parse_icmp(args->icmp_type);
      if (ret < 0)
        {
          printf("Failed to parse ICMP type!\n");
          goto errout;
        }

      icmp6->type = ret;
      icmp6->invflags = args->icmpinv;
    }
  else
    {
      entry = netlib_ip6t_filter_entry(args->target, args->verdict, 0);
      if (entry == NULL)
        {
          goto errout_prepare_entry;
        }
    }

  /* Fill in common details. */

  ret = netlib_ip6t_fillifname(entry, args->inifname, args->outifname);
  if (ret < 0)
    {
      printf("Failed to fill in interface names!\n");
      goto errout;
    }

  if (args->saddr != NULL)
    {
      ret = iptables_parse_ip(args->saddr, &entry->ipv6.src,
                              &entry->ipv6.smsk, AF_INET6);
      if (ret < 0)
        {
          printf("Failed to parse source address %s!\n", args->saddr);
          goto errout;
        }
    }

  if (args->daddr != NULL)
    {
      ret = iptables_parse_ip(args->daddr, &entry->ipv6.dst,
                              &entry->ipv6.dmsk, AF_INET6);
      if (ret < 0)
        {
          printf("Failed to parse destination address %s!\n", args->daddr);
          goto errout;
        }
    }

  entry->ipv6.proto    = args->protocol;
  entry->ipv6.invflags = args->ipinv;

  /* Finish command. */

  ret = ip6tables_finish_command(args, &repl, entry);

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
 * Name: ip6tables_apply
 *
 * Description:
 *   Apply rules for corresponding table
 *
 ****************************************************************************/

static int ip6tables_apply(FAR const struct iptables_args_s *args,
                           ip6tables_command_func_t command_func)
{
  switch (args->cmd)
    {
      case COMMAND_FLUSH:
        return netlib_ip6t_flush(args->table, args->hook);

      case COMMAND_LIST:
        return ip6tables_list(args->table, args->hook);

      case COMMAND_POLICY:
        return netlib_ip6t_policy(args->table, args->hook, args->verdict);

      case COMMAND_DELETE:

        /* Delete rule with rulenum can be done directly. */

        if (args->rulenum > 0)
          {
            return ip6tables_finish_directly(args);
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
      ret = ip6tables_apply(&args, ip6tables_filter_command);
      if (ret < 0)
        {
          printf("ip6tables got error on filter: %d!\n", ret);
        }
    }
  else
#endif
    {
      printf("Unknown table: %s\n", args.table);
    }

  return ret;
}
