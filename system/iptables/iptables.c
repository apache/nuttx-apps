/****************************************************************************
 * apps/system/iptables/iptables.c
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
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <nuttx/net/netfilter/ip_tables.h>
#include <nuttx/net/netfilter/nf_nat.h>

#include "argtable3.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* TODO: Our getopt(), which argtable3 depends, does not support nonoptions
 * in the middle, so commands like 'iptables -I chain rulenum -j MASQUERADE'
 * cannot be supported, because the 'rulenum' will stop getopt() logic and
 * the -j option will never be parsed. Although we write 'rulenum' option
 * here, it may not work well, but it will work when our getopt() is updated.
 */

struct iptables_args_s
{
  FAR struct arg_str *table;

  FAR struct arg_str *append_chain;
  FAR struct arg_str *insert_chain;
  FAR struct arg_str *delete_chain;
  FAR struct arg_str *flush_chain;
  FAR struct arg_str *list_chain;

  FAR struct arg_int *rulenum;

  FAR struct arg_str *target;
  FAR struct arg_str *outifname;

  FAR struct arg_end *end;
};

enum iptables_command_e
{
  COMMAND_APPEND,
  COMMAND_INSERT,
  COMMAND_DELETE,
  COMMAND_FLUSH,
  COMMAND_LIST,
  COMMAND_NUM,
};

struct iptables_command_s
{
  enum iptables_command_e cmd;
  enum nf_inet_hooks hook;
  int rulenum;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR const char *g_hooknames[] =
{
  "PREROUTING", "INPUT", "FORWARD", "OUTPUT", "POSTROUTING"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iptables_showusage
 *
 * Description:
 *   Show usage of the demo program
 *
 ****************************************************************************/

static void iptables_showusage(FAR const char *progname, FAR void** argtable)
{
  printf("USAGE: %s -t table -[AD] chain rule-specification\n", progname);
  printf("       %s -t table -I chain [rulenum] rule-specification\n",
         progname);
  printf("       %s -t table -D chain rulenum\n", progname);
  printf("       %s -t table -[FL] [chain]\n", progname);
  printf("iptables command:\n");
  arg_print_glossary(stdout, argtable, NULL);
}

/****************************************************************************
 * Name: iptables_parse_hook
 *
 * Description:
 *   Get hook from string
 *
 ****************************************************************************/

static enum nf_inet_hooks iptables_parse_hook(FAR const char *str)
{
  unsigned int hook;

  if (str == NULL || strlen(str) == 0) /* Might be no input (-F/-L). */
    {
      return NF_INET_NUMHOOKS;
    }

  for (hook = 0; hook < NF_INET_NUMHOOKS; hook++)
    {
      if (strcmp(str, g_hooknames[hook]) == 0)
        {
          return hook;
        }
    }

  printf("Failed to parse hook: %s, default to %d\n", str, NF_INET_NUMHOOKS);
  return NF_INET_NUMHOOKS; /* Failed to parse. */
}

/****************************************************************************
 * Name: iptables_command
 *
 * Description:
 *   Get command from arg list
 *
 ****************************************************************************/

static struct iptables_command_s
iptables_command(FAR const struct iptables_args_s *args)
{
  struct iptables_command_s ret =
    {
      COMMAND_NUM,
      NF_INET_NUMHOOKS,
      -1
    };

  if (args->rulenum->count > 0)
    {
      ret.rulenum = *args->rulenum->ival;
    }

  /* Flush & list with no chain specified will result in count > 0 and empty
   * string in sval[0], then we map the hook to NF_INET_NUMHOOKS.
   */

  if (args->flush_chain->count > 0)
    {
      ret.cmd = COMMAND_FLUSH;
      ret.hook = iptables_parse_hook(args->flush_chain->sval[0]);
    }
  else if (args->list_chain->count > 0)
    {
      ret.cmd = COMMAND_LIST;
      ret.hook = iptables_parse_hook(args->list_chain->sval[0]);
    }
  else if (args->append_chain->count > 0)
    {
      ret.cmd = COMMAND_APPEND;
      ret.hook = iptables_parse_hook(args->append_chain->sval[0]);
    }
  else if (args->insert_chain->count > 0)
    {
      ret.cmd = COMMAND_INSERT;
      ret.hook = iptables_parse_hook(args->insert_chain->sval[0]);
      ret.rulenum = MAX(ret.rulenum, 1);
    }
  else if (args->delete_chain->count > 0)
    {
      ret.cmd = COMMAND_DELETE;
      ret.hook = iptables_parse_hook(args->delete_chain->sval[0]);
    }

  return ret;
}

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
  const char fmt[] = "%12s %4s %4s %16s %16s\n";
  char src[INET_ADDRSTRLEN];
  char dst[INET_ADDRSTRLEN];
  FAR struct ipt_entry *entry;
  FAR uint8_t *head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  int size = repl->underflow[hook] - repl->hook_entry[hook];

  printf("Chain %s\n", g_hooknames[hook]);
  printf(fmt, "target", "idev", "odev", "source", "destination");

  ipt_entry_for_every(entry, head, size)
    {
      FAR struct xt_entry_target *target = IPT_TARGET(entry);
      printf(fmt, target->u.user.name, entry->ip.iniface, entry->ip.outiface,
        iptables_addr2str(entry->ip.src, entry->ip.smsk, src, sizeof(src)),
        iptables_addr2str(entry->ip.dst, entry->ip.dmsk, dst, sizeof(dst)));
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
 * Name: iptables_nat_command
 *
 * Description:
 *   Do a NAT command
 *
 ****************************************************************************/

static int iptables_nat_command(FAR const struct iptables_command_s *cmd,
                                FAR const char *ifname)
{
  FAR struct ipt_replace *repl = netlib_ipt_prepare(TABLE_NAME_NAT);
  FAR struct ipt_entry *entry  = NULL;
  int ret;

  if (repl == NULL)
    {
      printf("Failed to read table '" TABLE_NAME_NAT "' from kernel!\n");
      return -EIO;
    }

  if (ifname && strlen(ifname) > 0) /* No ifname if we delete with rulenum. */
    {
      entry = netlib_ipt_masquerade_entry(ifname);
      if (entry == NULL)
        {
          printf("Failed to prepare entry for dev %s!\n", ifname);
          ret = -ENOMEM;
          goto errout_with_repl;
        }
    }

  switch (cmd->cmd)
    {
      case COMMAND_APPEND:
        ret = netlib_ipt_append(&repl, entry, cmd->hook);
        break;

      case COMMAND_INSERT:
        ret = netlib_ipt_insert(&repl, entry, cmd->hook, cmd->rulenum);
        break;

      case COMMAND_DELETE:
        ret = netlib_ipt_delete(repl, entry, cmd->hook, cmd->rulenum);
        break;

      default: /* Other commands should not call into this function. */
        ret = -EINVAL;
        break;
    }

  if (ret == OK)
    {
      ret = netlib_ipt_commit(repl);
    }

  if (entry)
    {
      free(entry);
    }

errout_with_repl:
  free(repl);
  return ret;
}

/****************************************************************************
 * Name: iptables_nat
 *
 * Description:
 *   Apply rules for NAT
 *
 ****************************************************************************/

static int iptables_nat(FAR const struct iptables_args_s *args)
{
  struct iptables_command_s cmd = iptables_command(args);

  switch (cmd.cmd)
    {
      case COMMAND_FLUSH:
        return netlib_ipt_flush(TABLE_NAME_NAT, cmd.hook);

      case COMMAND_LIST:
        return iptables_list(TABLE_NAME_NAT, cmd.hook);

      case COMMAND_APPEND:
      case COMMAND_INSERT:
      case COMMAND_DELETE:
        if (args->outifname->count == 0 &&
            !(cmd.cmd == COMMAND_DELETE && cmd.rulenum > 0))
          {
            printf("Table '" TABLE_NAME_NAT "' needs an out interface!\n");
            return -EINVAL;
          }

        if (args->target->count > 0 &&
            strcmp(args->target->sval[0], XT_MASQUERADE_TARGET))
          {
            printf("Only target '" XT_MASQUERADE_TARGET
                  "' is supported for table '" TABLE_NAME_NAT "'!\n");
            return -EINVAL;
          }

        return iptables_nat_command(&cmd, args->outifname->sval[0]);

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
  int nerrors;
  int ret = 0;

  args.table        = arg_str1("t", "table", "table", "table to manipulate");

  args.append_chain = arg_str0("A", "append", "chain",
                               "Append a rule to chain");
  args.insert_chain = arg_str0("I", "insert", "chain",
                               "Insert a rule to chain at rulenum "
                               "(default = 1)");
  args.delete_chain = arg_str0("D", "delete", "chain",
                               "Delete matching rule from chain");
  args.flush_chain  = arg_str0("F", "flush", "chain",
                               "Delete all rules in chain or all chains");
  args.list_chain   = arg_str0("L", "list", "chain",
                               "List all rules in chain or all chains");

  args.rulenum      = arg_int0(NULL, NULL, "rulenum", "Rule num (1=first)");

  args.target       = arg_str0("j", "jump", "target", "target for rule");
  args.outifname    = arg_str0("o", "out-interface", "dev",
                               "output network interface name");

  args.end          = arg_end(1);

  /* The chain of -F or -L is optional. */

  args.flush_chain->hdr.flag |= ARG_HASOPTVALUE;
  args.list_chain->hdr.flag  |= ARG_HASOPTVALUE;

  nerrors = arg_parse(argc, argv, (FAR void**)&args);
  if (nerrors != 0)
    {
      arg_print_errors(stderr, args.end, argv[0]);
      iptables_showusage(argv[0], (FAR void**)&args);
    }
  else if (strcmp(args.table->sval[0], TABLE_NAME_NAT) == 0)
    {
      ret = iptables_nat(&args);
      if (ret < 0)
        {
          printf("iptables got error on NAT: %d!\n", ret);
        }
    }
  else
    {
      printf("Unknown table: %s\n", args.table->sval[0]);
    }

  arg_freetable((FAR void **)&args, 1);
  return ret;
}
