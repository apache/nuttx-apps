/****************************************************************************
 * apps/system/iptables/iptables_utils.c
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
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <nuttx/net/netfilter/ip_tables.h>
#include <nuttx/net/netfilter/ip6_tables.h>

#include "iptables.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MATCH_BUFSIZE sizeof("tcp spts:!65535:65535 dpts:!65535:65535")

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
 * Name: iptables_parse_cmd
 *
 * Description:
 *   Get command from string
 *
 * Return Value:
 *   true if success, false if failed
 *
 ****************************************************************************/

static bool iptables_parse_cmd(FAR const char *str,
                               FAR enum iptables_command_e *command)
{
  if (strcmp(str, "-A") == 0 || strcmp(str, "--append") == 0)
    {
      *command = COMMAND_APPEND;
    }
  else if (strcmp(str, "-D") == 0 || strcmp(str, "--delete") == 0)
    {
      *command = COMMAND_DELETE;
    }
  else if (strcmp(str, "-I") == 0 || strcmp(str, "--insert") == 0)
    {
      *command = COMMAND_INSERT;
    }
  else if (strcmp(str, "-F") == 0 || strcmp(str, "--flush") == 0)
    {
      *command = COMMAND_FLUSH;
    }
  else if (strcmp(str, "-L") == 0 || strcmp(str, "--list") == 0)
    {
      *command = COMMAND_LIST;
    }
  else if (strcmp(str, "-P") == 0 || strcmp(str, "--policy") == 0)
    {
      *command = COMMAND_POLICY;
    }
  else
    {
      return false;
    }

  return true;
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

  return NF_INET_NUMHOOKS; /* Failed to parse. */
}

/****************************************************************************
 * Name: iptables_parse_proto
 *
 * Description:
 *   Get protocol number from protocol name
 *
 ****************************************************************************/

static uint8_t iptables_parse_proto(FAR const char *proto)
{
  if (strcmp(proto, "all") == 0)
    {
      return 0;
    }
  else if (strcmp(proto, "esp") == 0)
    {
      return IPPROTO_ESP;
    }
  else if (strcmp(proto, "icmp") == 0)
    {
      return IPPROTO_ICMP;
    }
  else if (strcmp(proto, "icmp6") == 0 || strcmp(proto, "icmpv6") == 0 ||
           strcmp(proto, "ipv6-icmp") == 0)
    {
      return IPPROTO_ICMP6;
    }
  else if (strcmp(proto, "tcp") == 0)
    {
      return IPPROTO_TCP;
    }
  else if (strcmp(proto, "udp") == 0)
    {
      return IPPROTO_UDP;
    }
  else
    {
      printf("Unknown protocol: %s\n", proto);
      return 0;
    }
}

/****************************************************************************
 * Name: iptables_parse_target
 *
 * Description:
 *   Get target name from string
 *
 ****************************************************************************/

static FAR const char *iptables_parse_target(FAR const char *str,
                                             FAR int8_t *verdict)
{
  if (strcmp(str, "ACCEPT") == 0)
    {
      *verdict = -NF_ACCEPT - 1;
      return XT_STANDARD_TARGET;
    }
  else if (strcmp(str, "DROP") == 0)
    {
      *verdict = -NF_DROP - 1;
      return XT_STANDARD_TARGET;
    }
  else
    {
      *verdict = 0;
      return str;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iptables_parse
 *
 * Description:
 *   Parse args from arg list
 *
 * Returned Value:
 *   0 on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse(FAR struct iptables_args_s *args,
                   int argc, FAR char *argv[])
{
  bool inv = false;
  int i;

  bzero(args, sizeof(struct iptables_args_s));
  args->hook = NF_INET_NUMHOOKS;

  /* Parse arguments. */

  for (i = 1; i < argc; i++)
    {
      /* Table */

      if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--table") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing table name!\n");
              return -EINVAL;
            }

          args->table = argv[i];
          continue;
        }

      /* Commands */

      if (iptables_parse_cmd(argv[i], &args->cmd))
        {
          /* The chain name is following the command */

          if (i + 1 < argc)
            {
              args->hook = iptables_parse_hook(argv[i + 1]);
            }

          if (args->hook != NF_INET_NUMHOOKS)
            {
              i++; /* Success to parse as hook. */
            }
          else if (args->cmd != COMMAND_LIST && args->cmd != COMMAND_FLUSH)
            {
              printf("Wrong chain name %s!\n", argv[i + 1]);
              return -EINVAL;
            }

          /* Insert or delete command may have rule number */

          if (args->cmd == COMMAND_INSERT || args->cmd == COMMAND_DELETE)
            {
              if (i + 1 < argc)
                {
                  args->rulenum = atoi(argv[i + 1]);
                }

              if (args->rulenum >= 1)
                {
                  i++;
                }
              else if (args->cmd == COMMAND_INSERT)
                {
                  /* Default insert position is 1 */

                  args->rulenum = 1;
                }
            }

          /* Policy command should have target */

          if (args->cmd == COMMAND_POLICY)
            {
              if (++i >= argc)
                {
                  printf("Missing target name!\n");
                  return -EINVAL;
                }

              args->target = iptables_parse_target(argv[i], &args->verdict);
              if (args->verdict == 0)
                {
                  printf("Invalid target name %s!\n", argv[i]);
                  return -EINVAL;
                }
            }

          continue;
        }

      /* Target */

      if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jump") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing target name!\n");
              return -EINVAL;
            }

          args->target = iptables_parse_target(argv[i], &args->verdict);
          continue;
        }

      /* Invert */

      if (strcmp(argv[i], "!") == 0)
        {
          inv = true;
          continue;
        }

      /* Protocol */

      if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--protocol") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing protocol name!\n");
              return -EINVAL;
            }

          args->protocol = iptables_parse_proto(argv[i]);
          if (inv)
            {
              args->ipinv |= IPT_INV_PROTO;
              inv = false;
            }

          continue;
        }

      /* Source address */

      if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--source") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing source address!\n");
              return -EINVAL;
            }

          args->saddr = argv[i];
          if (inv)
            {
              args->ipinv |= IPT_INV_SRCIP;
              inv = false;
            }

          continue;
        }

      /* Destination address */

      if (strcmp(argv[i], "-d") == 0 ||
          strcmp(argv[i], "--destination") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing destination address!\n");
              return -EINVAL;
            }

          args->daddr = argv[i];
          if (inv)
            {
              args->ipinv |= IPT_INV_DSTIP;
              inv = false;
            }

          continue;
        }

      /* Source port */

      if (strcmp(argv[i], "--sport") == 0 ||
          strcmp(argv[i], "--source-port") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing source port!\n");
              return -EINVAL;
            }

          args->sport = argv[i];
          if (inv)
            {
              args->tcpudpinv |= XT_TCP_INV_SRCPT;
              inv = false;
            }

          continue;
        }

      /* Destination port */

      if (strcmp(argv[i], "--dport") == 0 ||
          strcmp(argv[i], "--destination-port") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing destination port!\n");
              return -EINVAL;
            }

          args->dport = argv[i];
          if (inv)
            {
              args->tcpudpinv |= XT_TCP_INV_DSTPT;
              inv = false;
            }

          continue;
        }

      /* In interface */

      if (strcmp(argv[i], "-i") == 0 ||
          strcmp(argv[i], "--in-interface") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing in-interface name!\n");
              return -EINVAL;
            }

          args->inifname = argv[i];
          if (inv)
            {
              args->ipinv |= IPT_INV_VIA_IN;
              inv = false;
            }

          continue;
        }

      /* Out interface */

      if (strcmp(argv[i], "-o") == 0 ||
          strcmp(argv[i], "--out-interface") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing out-interface name!\n");
              return -EINVAL;
            }

          args->outifname = argv[i];
          if (inv)
            {
              args->ipinv |= IPT_INV_VIA_OUT;
              inv = false;
            }

          continue;
        }

      /* ICMP type */

      if (strcmp(argv[i], "--icmp-type") == 0 ||
          strcmp(argv[i], "--icmpv6-type") == 0)
        {
          if (++i >= argc)
            {
              printf("Missing ICMP type!\n");
              return -EINVAL;
            }

          args->icmp_type = argv[i];
          if (inv)
            {
              args->icmpinv |= IPT_ICMP_INV;
              inv = false;
            }

          continue;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: iptables_parse_ip
 *
 * Description:
 *   Parse ip string into address and mask
 *
 * Returned Value:
 *   0 on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse_ip(FAR const char *str, FAR void *addr, FAR void *mask,
                      uint8_t family)
{
  FAR char *ch = strchr(str, '/');

  if (family != AF_INET && family != AF_INET6)
    {
      return -EINVAL;
    }

  memset(mask, 0xff, family == AF_INET ? 4 : 16);

  if (ch != NULL)
    {
      FAR char *endp;
      int prefixlen;

      *ch++ = 0;
      prefixlen = strtol(ch, &endp, 10);

      if (*endp != '\0')
        {
          return -EINVAL;
        }

#ifdef CONFIG_NET_IPv4
      if (family == AF_INET)
        {
          FAR in_addr_t *mask4 = mask;

          if (prefixlen > 32 || prefixlen < 0)
            {
              return -EINVAL;
            }

          *mask4 <<= 32 - prefixlen;
        }
#endif

#ifdef CONFIG_NET_IPv6
      if (family == AF_INET6)
        {
          if (prefixlen > 128 || prefixlen < 0)
            {
              return -EINVAL;
            }

          netlib_prefix2ipv6netmask(atoi(ch), mask);
        }
#endif
    }

  if (inet_pton(family, str, addr) != OK)
    {
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: iptables_parse_ports
 *
 * Description:
 *   Parse port string into min/max port, NULL for ALL
 *
 * Returned Value:
 *   0 on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse_ports(FAR const char *str, uint16_t ports[2])
{
  FAR char *endp;
  int port;

  /* Maybe sports has value but dports is NULL, then we set NULL to ALL. */

  if (str == NULL)
    {
      ports[0] = 0;
      ports[1] = 65535;
      return OK;
    }

  port = strtol(str, &endp, 10);
  if (*endp == '\0' && port >= 0 && port <= 65535)
    {
      ports[0] = ports[1] = port;
      return OK;
    }

  if (*endp == ':' && port >= 0 && port <= 65535)
    {
      ports[0] = port;
      port = strtol(endp + 1, &endp, 10);
      if (*endp == '\0' && port >= 0 && port <= 65535)
        {
          ports[1] = port;
          return OK;
        }
    }

  return -EINVAL;
}

/****************************************************************************
 * Name: iptables_parse_icmp
 *
 * Description:
 *   Parse icmp type string into type number
 *
 * Returned Value:
 *   Type code on success, or a negative error code on failure
 *
 ****************************************************************************/

int iptables_parse_icmp(FAR const char *str)
{
  FAR char *endp;
  int type;

  type = strtol(str, &endp, 10);
  if (*endp == '\0' && type >= 0 && type <= 255)
    {
      return type;
    }

  /* TODO: Support string description of icmp type, e.g. "echo-request" */

  return -EINVAL;
}

/****************************************************************************
 * Name: iptables_showusage
 *
 * Description:
 *   Show usage of the iptables program
 *
 ****************************************************************************/

void iptables_showusage(FAR const char *progname)
{
  /* Format:          [!] --source -s address <description> */

  const char fmt[] = "%3s %-15s %-2s %-16s %s\n";

  printf("USAGE: %s -t table -[AD] chain rule-specification\n", progname);
  printf("       %s -t table -I chain [rulenum] rule-specification\n",
         progname);
  printf("       %s -t table -D chain rulenum\n", progname);
  printf("       %s -t table -P chain target\n", progname);
  printf("       %s -t table -[FL] [chain]\n", progname);

  printf("\nCommands:\n");
  printf(fmt, "", "--append", "-A", "chain", "Append a rule to chain");
  printf(fmt, "", "--insert", "-I", "chain [rulenum]",
                    "Insert a rule to chain at rulenum (default = 1)");
  printf(fmt, "", "--delete", "-D", "chain [rulenum]",
                    "Delete matching rule from chain");
  printf(fmt, "", "--policy", "-P", "chain target",
                    "Set policy for chain to target");
  printf(fmt, "", "--flush", "-F", "[chain]",
                    "Delete all rules in chain or all chains");
  printf(fmt, "", "--list", "-L", "[chain]",
                    "List all rules in chain or all chains");

  printf("\nOptions:\n");
  printf(fmt, "   ", "--table", "-t", "table",
                       "Table to manipulate (default: filter)");
  printf(fmt, "   ", "--jump", "-j", "target", "Target for rule");
  printf(fmt, "[!]", "--in-interface", "-i", "dev",
                       "Input network interface name");
  printf(fmt, "[!]", "--out-interface", "-o", "dev",
                       "Output network interface name");
  printf(fmt, "[!]", "--source", "-s", "address[/mask]", "Source address");
  printf(fmt, "[!]", "--destination", "-d", "address[/mask]",
                       "Destination address");
  printf(fmt, "[!]", "--protocol", "-p", "proto",
                       "Protocol (tcp, udp, icmp, esp, all)");
  printf(fmt, "[!]", "--source-port,--sport", "", "", "");
  printf(fmt, "   ", "", "", "port[:port]", "Source port");
  printf(fmt, "[!]", "--destination-port,--dport", "", "", "");
  printf(fmt, "   ", "", "", "port[:port]", "Destination port");
  printf(fmt, "[!]", "--icmp-type", "", "type", "ICMP type");
  printf(fmt, "[!]", "--icmpv6-type", "", "type", "ICMPv6 type");
}

/****************************************************************************
 * Name: iptables_hook2str
 *
 * Description:
 *   Get hook name from hook number
 *
 ****************************************************************************/

FAR const char *iptables_hook2str(enum nf_inet_hooks hook)
{
  if (hook < NF_INET_NUMHOOKS)
    {
      return g_hooknames[hook];
    }

  return "UNKNOWN";
}

/****************************************************************************
 * Name: iptables_target2str
 *
 * Description:
 *   Get target name from target structure
 *
 ****************************************************************************/

FAR const char *iptables_target2str(FAR const struct xt_entry_target *tgt)
{
  if (strcmp(tgt->u.user.name, XT_STANDARD_TARGET) == 0)
    {
      int verdict = ((FAR const struct xt_standard_target *)tgt)->verdict;
      verdict = -verdict - 1;

      if (verdict == NF_ACCEPT)
        {
          return "ACCEPT";
        }
      else if (verdict == NF_DROP)
        {
          return "DROP";
        }
      else
        {
          return "UNKNOWN";
        }
    }

  return tgt->u.user.name;
}

/****************************************************************************
 * Name: iptables_proto2str
 *
 * Description:
 *   Get protocol name from protocol number
 *
 ****************************************************************************/

FAR const char *iptables_proto2str(uint8_t proto)
{
  switch (proto)
    {
      case 0:
        return "all";

      case IPPROTO_ICMP:
        return "icmp";

      case IPPROTO_ICMPV6:
        return "ipv6-icmp";

      case IPPROTO_TCP:
        return "tcp";

      case IPPROTO_UDP:
        return "udp";

      case IPPROTO_ESP:
        return "esp";

      default:
        return "unknown";
    }
}

/****************************************************************************
 * Name: iptables_match2str
 *
 * Description:
 *   Get match details from match structure
 *
 ****************************************************************************/

FAR const char *iptables_match2str(FAR const struct xt_entry_match *match)
{
  static char s_buf[MATCH_BUFSIZE];

  if (match == NULL)
    {
      return "";
    }

  if (strcmp(match->u.user.name, XT_MATCH_NAME_TCP) == 0)
    {
      FAR struct xt_tcp *tcp = (FAR struct xt_tcp *)(match + 1);

      snprintf(s_buf, MATCH_BUFSIZE,
               "tcp spts:%s%u:%u dpts:%s%u:%u",
               INV_FLAG_STR(tcp->invflags & XT_TCP_INV_SRCPT),
               tcp->spts[0], tcp->spts[1],
               INV_FLAG_STR(tcp->invflags & XT_TCP_INV_DSTPT),
               tcp->dpts[0], tcp->dpts[1]);
    }
  else if (strcmp(match->u.user.name, XT_MATCH_NAME_UDP) == 0)
    {
      FAR struct xt_udp *udp = (FAR struct xt_udp *)(match + 1);

      snprintf(s_buf, MATCH_BUFSIZE,
               "udp spts:%s%u:%u dpts:%s%u:%u",
               INV_FLAG_STR(udp->invflags & XT_UDP_INV_SRCPT),
               udp->spts[0], udp->spts[1],
               INV_FLAG_STR(udp->invflags & XT_UDP_INV_DSTPT),
               udp->dpts[0], udp->dpts[1]);
    }
  else if (strcmp(match->u.user.name, XT_MATCH_NAME_ICMP) == 0)
    {
      FAR struct ipt_icmp *icmp = (FAR struct ipt_icmp *)(match + 1);

      snprintf(s_buf, MATCH_BUFSIZE,
               "icmp %stype %u",
               INV_FLAG_STR(icmp->invflags & IPT_ICMP_INV),
               icmp->type);
    }
  else if (strcmp(match->u.user.name, XT_MATCH_NAME_ICMP6) == 0)
    {
      FAR struct ip6t_icmp *icmp6 = (FAR struct ip6t_icmp *)(match + 1);

      snprintf(s_buf, MATCH_BUFSIZE,
               "ipv6-icmp %stype %u",
               INV_FLAG_STR(icmp6->invflags & IP6T_ICMP_INV),
               icmp6->type);
    }
  else
    {
      return match->u.user.name;
    }

  return s_buf;
}
