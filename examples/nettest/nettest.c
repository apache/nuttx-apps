/****************************************************************************
 * examples/nettest/nettest.c
 *
 *   Copyright (C) 2007, 2009-2011, 2015 Gregory Nutt. All rights reserved.
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
//#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <apps/netutils/netlib.h>

#include "nettest.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_NETTEST_IPv6) && !defined(CONFIG_NET_ICMPv6_AUTOCONF)
/* Our host IPv6 address */

static const uint16_t g_ipv6_hostaddr[8] =
{
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_1),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_2),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_3),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_4),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_5),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_6),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_7),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6ADDR_8),
};

/* Default routine IPv6 address */

static const uint16_t g_ipv6_draddr[8] =
{
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_1),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_2),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_3),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_4),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_5),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_6),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_7),
  HTONS(CONFIG_EXAMPLES_NETTEST_DRIPv6ADDR_8),
};

/* IPv6 netmask */

static const uint16_t g_ipv6_netmask[8] =
{
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_1),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_2),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_3),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_4),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_5),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_6),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_7),
  HTONS(CONFIG_EXAMPLES_NETTEST_IPv6NETMASK_8),
};
#endif /* CONFIG_EXAMPLES_NETTEST_IPv6 && !CONFIG_NET_ICMPv6_AUTOCONF */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * nettest_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int nettest_main(int argc, char *argv[])
#endif
{
#ifndef CONFIG_EXAMPLES_NETTEST_IPv6
  struct in_addr addr;
#endif
#ifdef CONFIG_EXAMPLES_NETTEST_NOMAC
  uint8_t mac[IFHWADDRLEN];
#endif

/* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_NETTEST_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

#ifdef CONFIG_EXAMPLES_NETTEST_IPv6
#ifdef CONFIG_NET_ICMPv6_AUTOCONF
  /* Perform ICMPv6 auto-configuration */

  netlib_icmpv6_autoconfiguration("eth0");

#else /* CONFIG_NET_ICMPv6_AUTOCONF */

  /* Set up our fixed host address */

  netlib_set_ipv6addr("eth0",
                      (FAR const struct in6_addr *)g_ipv6_hostaddr);

  /* Set up the default router address */

  netlib_set_dripv6addr("eth0",
                        (FAR const struct in6_addr *)g_ipv6_draddr);

  /* Setup the subnet mask */

  netlib_set_ipv6netmask("eth0",
                        (FAR const struct in6_addr *)g_ipv6_netmask);

#endif /* CONFIG_NET_ICMPv6_AUTOCONF */
#else /* CONFIG_EXAMPLES_NETTEST_IPv6 */

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_NETTEST_IPADDR);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_NETTEST_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_NETTEST_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

#endif /* CONFIG_EXAMPLES_NETTEST_IPv6 */

#ifdef CONFIG_EXAMPLES_NETTEST_SERVER
  recv_server();
#else
  send_client();
#endif

  return 0;
}
