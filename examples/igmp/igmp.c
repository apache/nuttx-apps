/****************************************************************************
 * apps/examples/igmp/igmp.c
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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <debug.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include "netutils/netlib.h"
#include "netutils/ipmsfilter.h"

#include "igmp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check if the destination address is a multicast address
 *
 * - IPv4: multicast addresses lie in the class D group -- The address range
 *   224.0.0.0 to 239.255.255.255 (224.0.0.0/4)
 *
 * - IPv6 multicast addresses are have the high-order octet of the
 *   addresses=0xff (ff00::/8.)
 */

#if ((CONFIG_EXAMPLES_IGMP_GRPADDR & 0xffff0000) < 0xe0000000ul) || \
    ((CONFIG_EXAMPLES_IGMP_GRPADDR & 0xffff0000) > 0xeffffffful)
#  error "Bad range for IGMP group address"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * igmp_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct in_addr addr;
  struct in_addr mcast;
#if defined(CONFIG_EXAMPLES_IGMP_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif

  printf("Configuring Ethernet...\n");

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_IGMP_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_IGMP_IPADDR);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_IGMP_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_IGMP_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

  /* Not much of a test for now */

  addr.s_addr  = HTONL(CONFIG_EXAMPLES_IGMP_IPADDR);
  mcast.s_addr = HTONL(CONFIG_EXAMPLES_IGMP_GRPADDR);

  /* Join the group */

  printf("Join group...\n");
  ipmsfilter(&addr, &mcast, MCAST_INCLUDE);

  /* Wait a while */

  printf("Wait for timeout...\n");
  sleep(5);

  /* Leave the group */

  printf("Leave group...\n");
  ipmsfilter(&addr, &mcast, MCAST_EXCLUDE);

  /* Wait a while */

  sleep(5);
  printf("Exiting...\n");
  return 0;
}
