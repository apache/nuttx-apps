/****************************************************************************
 * apps/examples/nettest/nettest_netinit.c
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

#include <stdbool.h>
#include <stdio.h>
#include <debug.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include "netutils/netlib.h"

#include "nettest.h"

#ifdef CONFIG_EXAMPLES_NETTEST_INIT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_NETTEST_DEVNAME
#  define DEVNAME CONFIG_EXAMPLES_NETTEST_DEVNAME
#else
#  define DEVNAME "eth0"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_NETTEST_INIT) && \
    defined(CONFIG_EXAMPLES_NETTEST_IPv6) && \
    !defined(CONFIG_NET_ICMPv6_AUTOCONF)
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
#endif /* CONFIG_EXAMPLES_NETTEST_INIT && CONFIG_EXAMPLES_NETTEST_IPv6 && !CONFIG_NET_ICMPv6_AUTOCONF */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void nettest_initialize(void)
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
  netlib_setmacaddr(DEVNAME, mac);
#endif

#ifdef CONFIG_EXAMPLES_NETTEST_IPv6
#ifdef CONFIG_NET_ICMPv6_AUTOCONF
  /* Perform ICMPv6 auto-configuration */

  netlib_icmpv6_autoconfiguration(DEVNAME);

#else /* CONFIG_NET_ICMPv6_AUTOCONF */

  /* Set up our fixed host address */

  netlib_set_ipv6addr(DEVNAME,
                      (FAR const struct in6_addr *)g_ipv6_hostaddr);

  /* Set up the default router address */

  netlib_set_dripv6addr(DEVNAME,
                        (FAR const struct in6_addr *)g_ipv6_draddr);

  /* Setup the subnet mask */

  netlib_set_ipv6netmask(DEVNAME,
                        (FAR const struct in6_addr *)g_ipv6_netmask);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup(DEVNAME);

#endif /* CONFIG_NET_ICMPv6_AUTOCONF */
#else /* CONFIG_EXAMPLES_NETTEST_IPv6 */

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_NETTEST_IPADDR);
  netlib_set_ipv4addr(DEVNAME, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_NETTEST_DRIPADDR);
  netlib_set_dripv4addr(DEVNAME, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_NETTEST_NETMASK);
  netlib_set_ipv4netmask(DEVNAME, &addr);

#endif /* CONFIG_EXAMPLES_NETTEST_IPv6 */
}

#endif /* CONFIG_EXAMPLES_NETTEST_INIT */
