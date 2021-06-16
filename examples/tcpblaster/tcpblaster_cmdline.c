/****************************************************************************
 * apps/examples/tcpblaster/tcpblaster_cmdline.c
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

#include "tcpblaster.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6

uint16_t g_tcpblasterserver_ipv6[8];

#else
uint32_t g_tcpblasterserver_ipv4;
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
 * tcpblaster_cmdline
 ****************************************************************************/

void tcpblaster_cmdline(int argc, char **argv)
{
  /* Init the default IP address. */

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
#  if defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK) && \
      defined(CONFIG_NET_LOOPBACK)
  g_tcpblasterserver_ipv6[0] = 0;
  g_tcpblasterserver_ipv6[1] = 0;
  g_tcpblasterserver_ipv6[2] = 0;
  g_tcpblasterserver_ipv6[3] = 0;
  g_tcpblasterserver_ipv6[4] = 0;
  g_tcpblasterserver_ipv6[5] = 0;
  g_tcpblasterserver_ipv6[6] = 0;
  g_tcpblasterserver_ipv6[7] = HTONS(1);
#  else
  g_tcpblasterserver_ipv6[0] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_1);
  g_tcpblasterserver_ipv6[1] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_2);
  g_tcpblasterserver_ipv6[2] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_3);
  g_tcpblasterserver_ipv6[3] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_4);
  g_tcpblasterserver_ipv6[4] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_5);
  g_tcpblasterserver_ipv6[5] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_6);
  g_tcpblasterserver_ipv6[6] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_7);
  g_tcpblasterserver_ipv6[7] =
    HTONS(CONFIG_EXAMPLES_TCPBLASTER_SERVERIPv6ADDR_8);
#  endif
#else
#  if defined(CONFIG_EXAMPLES_TCPBLASTER_LOOPBACK) && defined(CONFIG_NET_LOOPBACK)
  g_tcpblasterserver_ipv4 = HTONL(0x7f000001);
#  else
  g_tcpblasterserver_ipv4 = HTONL(CONFIG_EXAMPLES_TCPBLASTER_SERVERIP);
#  endif
#endif

  /* Currently only a single command line option is supported:  The server
   * IP address. Used to override default.
   */

  if (argc == 2)
    {
      int ret;

      /* Convert the <server-addr> argument into a binary address */

#ifdef CONFIG_EXAMPLES_TCPBLASTER_IPv6
      ret = inet_pton(AF_INET6, argv[1], g_tcpblasterserver_ipv6);
#else
      ret = inet_pton(AF_INET, argv[1], &g_tcpblasterserver_ipv4);
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
