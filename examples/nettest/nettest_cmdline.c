/****************************************************************************
 * apps/examples/nettest/netest_cmdline.c
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "nettest.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NETTEST_IPv6

uint16_t g_nettestserver_ipv6[8] =
{
#if defined(CONFIG_EXAMPLES_NETTEST_LOOPBACK) && defined(NET_LOOPBACK)
  0        /* Use the loopback address */
  0
  0
  0
  0
  0
  0
  HTONS(1);
#else
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_1),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_2),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_3),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_4),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_5),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_6),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_7),
  HTONS(CONFIG_EXAMPLES_NETTEST_SERVERIPv6ADDR_8)
#endif
};

#else

#if defined(CONFIG_EXAMPLES_NETTEST_LOOPBACK) && defined(CONFIG_NET_LOOPBACK)
uint32_t g_nettestserver_ipv4 = HTONL(0x7f000001);
#else
uint32_t g_nettestserver_ipv4 = HTONL(CONFIG_EXAMPLES_NETTEST_SERVERIP);
#endif

#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s [<server-addr>]\n", progname);
  exit(1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * nettest_cmdline
 ****************************************************************************/

void nettest_cmdline(int argc, char **argv)
{
  /* Currently only a single command line option is supported:  The server
   * IP address.
   */

  if (argc == 2)
    {
      int ret;

      /* Convert the <server-addr> argument into a binary address */

#ifdef CONFIG_EXAMPLES_NETTEST_IPv6
      ret = inet_pton(AF_INET6, argv[1], g_nettestserver_ipv6);
#else
      ret = inet_pton(AF_INET, argv[1], &g_nettestserver_ipv4);
#endif
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: <server-addr> is invalid\n");
          show_usage(argv[0]);
        }
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Too many arguments\n");
      show_usage(argv[0]);
    }
}
