/****************************************************************************
 * examples/tcpblaster/tcpblaster_cmdline.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
