/****************************************************************************
 * apps/netutils/iperf/iperf_main.c
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
#include <sys/time.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "netutils/netlib.h"
#include "argtable3.h"
#include "iperf.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_IPERFTEST_DEVNAME
#  define DEVNAME CONFIG_NETUTILS_IPERFTEST_DEVNAME
#else
#  define DEVNAME "wlan0"
#endif

#define IPERF_DEFAULT_PORT     5001
#define IPERF_DEFAULT_INTERVAL 3
#define IPERF_DEFAULT_TIME     30

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct wifi_iperf_t
{
  struct arg_str *ip;
  struct arg_lit *server;
  struct arg_lit *udp;
  struct arg_int *port;
  struct arg_int *interval;
  struct arg_int *time;
  struct arg_lit *abort;
  struct arg_end *end;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: iperf_showusage
 *
 * Description:
 *   Show usage of the demo program and exit
 *
 ****************************************************************************/

static void iperf_showusage(FAR const char *progname, int exitcode)
{
  printf("USAGE: %s [-sua] [-c <ip>] [-p <port>]\
         [-i <interval>] [-t <time>]\n", progname);
  printf("iperf command:\n");
  printf("  -c, --client=<ip>  run in client mode,\
         connecting to <host>\n");
  printf("  -s, --server  run in server mode\n");
  printf("  -u, --udp  use UDP rather than TCP\n");
  printf("  -p, --port=<port>  server port to listen on/connect to\n");
  printf("  -i, --interval=<interval>\
         seconds between periodic bandwidth reports\n");
  printf("  -t, --time=<time>\
         time in seconds to transmit for (default 10 secs)\n");
  printf("  -a, --abort  abort running iperf\n");
  printf("\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct wifi_iperf_t iperf_args;
  struct iperf_cfg_t cfg;
  struct in_addr addr;
  int nerrors;

  bzero(&addr, sizeof(struct in_addr));
  bzero(&cfg, sizeof(cfg));

  iperf_args.ip = arg_str0("c", "client", "<ip>",
                           "run in client mode, connecting to <host>");
  iperf_args.server = arg_lit0("s", "server", "run in server mode");
  iperf_args.udp = arg_lit0("u", "udp", "use UDP rather than TCP");
  iperf_args.port = arg_int0("p", "port", "<port>",
                             "server port to listen on/connect to");
  iperf_args.interval = arg_int0("i", "interval", "<interval>",
                            "seconds between periodic bandwidth reports");
  iperf_args.time = arg_int0("t", "time", "<time>",
                        "time in seconds to transmit for (default 10 secs)");
  iperf_args.abort = arg_lit0("a", "abort", "abort running iperf");
  iperf_args.end = arg_end(1);

  nerrors = arg_parse(argc, argv, (void**) &iperf_args);
  if (nerrors != 0)
    {
      arg_print_errors(stderr, iperf_args.end, argv[0]);
      iperf_showusage(argv[0], 0);
    }

  if (iperf_args.abort->count != 0)
    {
      iperf_stop();
      printf("ERROR: abort->count: %d\n", iperf_args.abort->count);
      iperf_showusage(argv[0], 0);
    }

  if (((iperf_args.ip->count == 0) && (iperf_args.server->count == 0)) ||
         ((iperf_args.ip->count != 0) && (iperf_args.server->count != 0)))
    {
      printf("ERROR: should specific client/server mode\n");
      iperf_showusage(argv[0], 0);
    }

  if (iperf_args.ip->count == 0)
    {
      cfg.flag |= IPERF_FLAG_SERVER;
    }
  else
    {
      cfg.dip = inet_addr(iperf_args.ip->sval[0]);
      cfg.flag |= IPERF_FLAG_CLIENT;
    }

  addr.s_addr = 0;
  netlib_get_ipv4addr(DEVNAME, &addr);
  if (addr.s_addr == 0)
    {
      printf("ERROR: access IP is 0x00 \n");
      return -1;
    }

  printf("       IP: %s\n", inet_ntoa(addr));

  cfg.sip = addr.s_addr;

  if (iperf_args.udp->count == 0)
    {
      cfg.flag |= IPERF_FLAG_TCP;
    }
  else
    {
      cfg.flag |= IPERF_FLAG_UDP;
    }

  if (iperf_args.port->count == 0)
    {
      cfg.sport = IPERF_DEFAULT_PORT;
      cfg.dport = IPERF_DEFAULT_PORT;
    }
  else
    {
      if (cfg.flag & IPERF_FLAG_SERVER)
        {
          cfg.sport = iperf_args.port->ival[0];
          cfg.dport = IPERF_DEFAULT_PORT;
        }
      else
        {
          cfg.sport = IPERF_DEFAULT_PORT;
          cfg.dport = iperf_args.port->ival[0];
        }
    }

  if (iperf_args.interval->count == 0)
    {
      cfg.interval = IPERF_DEFAULT_INTERVAL;
    }
  else
    {
      cfg.interval = iperf_args.interval->ival[0];
      if (cfg.interval <= 0)
        {
          cfg.interval = IPERF_DEFAULT_INTERVAL;
        }
    }

  if (iperf_args.time->count == 0)
    {
      if (iperf_args.server->count != 0)
        {
          /* Note: -t is a client-only option for the original iperf 2. */

          cfg.time = 0;
        }
      else
        {
          cfg.time = IPERF_DEFAULT_TIME;
        }
    }
  else
    {
      cfg.time = iperf_args.time->ival[0];
      if (cfg.time != 0 && cfg.time <= cfg.interval)
        {
          cfg.time = cfg.interval;
        }
    }

  arg_freetable((FAR void **)&iperf_args,
                sizeof(iperf_args) / sizeof(FAR void *));

  printf("\n mode=%s-%s "
         "sip=%" PRId32 ".%" PRId32 ".%" PRId32 ".%" PRId32 ":%d,"
         "dip=%" PRId32 ".%" PRId32 ".%" PRId32 ".%" PRId32 ":%d, "
         "interval=%" PRId32 ", time=%" PRId32 " \n",
         cfg.flag & IPERF_FLAG_TCP ?"tcp":"udp",
         cfg.flag & IPERF_FLAG_SERVER ?"server":"client",
         cfg.sip & 0xff, (cfg.sip >> 8) & 0xff, (cfg.sip >> 16) & 0xff,
         (cfg.sip >> 24) & 0xff, cfg.sport,
         cfg.dip & 0xff, (cfg.dip >> 8) & 0xff, (cfg.dip >> 16) & 0xff,
         (cfg.dip >> 24) & 0xff, cfg.dport,
         cfg.interval, cfg.time);
  iperf_start(&cfg);

  return 0;
}
