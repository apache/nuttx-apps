/****************************************************************************
 * apps/system/conntrack/conntrack.c
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

#include <netpacket/netlink.h>

#include <nuttx/net/ip.h>

#include "argtable3.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RXBUFFER_SIZE 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct conntrack_args_s
{
  FAR struct arg_lit *dump;
  FAR struct arg_lit *event;
  FAR struct arg_str *family;
  FAR struct arg_end *end;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_exiting;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigexit
 ****************************************************************************/

static void sigexit(int signo)
{
  g_exiting = true;
}

/****************************************************************************
 * Name: proto2str
 ****************************************************************************/

static FAR const char *proto2str(uint8_t proto)
{
  switch (proto)
    {
      case IPPROTO_TCP:
        return "tcp";
      case IPPROTO_UDP:
        return "udp";
      case IPPROTO_ICMP:
        return "icmp";
      case IPPROTO_ICMP6:
        return "icmp6";
      default:
        return "";
    }
}

/****************************************************************************
 * Name: conntrack_print_tuple
 ****************************************************************************/

static void conntrack_print_tuple(sa_family_t family,
                            FAR const struct netlib_conntrack_tuple_s *tuple)
{
  char ipstrbuf[INET6_ADDRSTRLEN];

  /* src=10.88.0.88 dst=192.168.66.66 sport=45065 dport=5001 */

  inet_ntop(family, &tuple->src, ipstrbuf, INET6_ADDRSTRLEN);
  printf("src=%s ", ipstrbuf);
  inet_ntop(family, &tuple->dst, ipstrbuf, INET6_ADDRSTRLEN);
  printf("dst=%s ", ipstrbuf);

  switch (tuple->l4proto)
    {
      case IPPROTO_TCP:
      case IPPROTO_UDP:
        printf("sport=%" PRIu16 " dport=%" PRIu16 " ",
               tuple->l4.tcp.sport, tuple->l4.tcp.dport);
        break;
      case IPPROTO_ICMP:
      case IPPROTO_ICMP6:
        printf("type=%" PRIu8 " code=%" PRIu8 " id=%" PRIu16 " ",
               tuple->l4.icmp.type, tuple->l4.icmp.code, tuple->l4.icmp.id);
        break;
    }
}

/****************************************************************************
 * Name: conntrack_print
 ****************************************************************************/

static int conntrack_print(FAR struct netlib_conntrack_s *ct)
{
  /* tcp  <orig> <reply> */

  printf("%-5s ", proto2str(ct->orig.l4proto));
  conntrack_print_tuple(ct->family, &ct->orig);
  conntrack_print_tuple(ct->family, &ct->reply);
  printf("\n");

  return 0;
}

/****************************************************************************
 * Name: conntrack_dump
 ****************************************************************************/

static void conntrack_dump(sa_family_t family)
{
  ssize_t ndumped = netlib_get_conntrack(family, conntrack_print);
  printf("conntrack: %zd flow entries have been shown.\n", ndumped);
}

/****************************************************************************
 * Name: conntrack_monitor_socket
 ****************************************************************************/

static int conntrack_monitor_socket(uint32_t groups)
{
  struct sockaddr_nl addr;
  int fd;

  /* Create a NetLink socket with NETLINK_NETFILTER protocol */

  fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_NETFILTER);
  if (fd < 0)
    {
      perror("ERROR: failed to create netlink socket");
      return -errno;
    }

  addr.nl_family = AF_NETLINK;
  addr.nl_pad    = 0;
  addr.nl_pid    = getpid();
  addr.nl_groups = groups;

  if (bind(fd, (FAR const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      perror("ERROR: failed to bind netlink socket");
      close(fd);
      return -errno;
    }

  return fd;
}

/****************************************************************************
 * Name: conntrack_monitor_event
 ****************************************************************************/

static void conntrack_monitor_event(void)
{
  FAR const struct nlmsghdr *nlh;
  struct netlib_conntrack_s ct;
  uint8_t buf[RXBUFFER_SIZE];
  ssize_t len;
  int fd;

  /* Setup exit signal handler */

  g_exiting = false;
  signal(SIGINT, sigexit);

  /* Create a NetLink socket with NETLINK_NETFILTER protocol */

  fd = conntrack_monitor_socket(NF_NETLINK_CONNTRACK_NEW |
                                NF_NETLINK_CONNTRACK_DESTROY);
  if (fd < 0)
    {
      return;
    }

  while ((len = read(fd, buf, sizeof(buf))) >= 0 && !g_exiting)
    {
      nlh = (FAR struct nlmsghdr *)buf;

      if (netlib_parse_conntrack(nlh, len, &ct) < 0)
        {
          fprintf(stderr, "Failed to parse conntrack message\n");
          continue;
        }

      /* Only event log needs to print type prefix */

      switch (ct.type)
        {
          case IPCTNL_MSG_CT_NEW:
            printf("    [NEW] ");
            break;

          case IPCTNL_MSG_CT_DELETE:
            printf("[DESTROY] ");
            break;

          default:
            printf("[UNKNOWN] ");
            break;
        }

      /* Print the remaining line */

      conntrack_print(&ct);
    }

  close(fd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct conntrack_args_s args;
  sa_family_t family = AF_INET;
  int nerrors;

  args.dump   = arg_lit0("L", "dump", "List connection tracking");
  args.event  = arg_lit0("E", "event", "Display a real-time event log");
  args.family = arg_str0("f", "family", "PROTO", "Specify L3 (ipv4, ipv6) "
                         "protocol, only for dump option (default ipv4)");
  args.end    = arg_end(3);

  nerrors = arg_parse(argc, argv, (FAR void **)&args);
  if (nerrors != 0 || args.dump->count + args.event->count == 0)
    {
      arg_print_errors(stdout, args.end, argv[0]);
      printf("Usage:\n");
      arg_print_glossary(stdout, (FAR void **)&args, NULL);
      goto out;
    }

  if (args.family->count != 0)
    {
      if (strncmp(args.family->sval[0], "ipv6", 4) == 0)
        {
          family = AF_INET6;
        }
    }

  if (args.dump->count != 0)
    {
      conntrack_dump(family);
    }

  if (args.event->count != 0)
    {
      conntrack_monitor_event();
    }

out:
  arg_freetable((FAR void **)&args, 1);
  return 0;
}
