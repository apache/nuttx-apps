/****************************************************************************
 * apps/examples/discover/discover_main.c
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <debug.h>

#include <net/if.h>
#include <netinet/in.h>
#include <nuttx/net/arp.h>

#include "netutils/netlib.h"
#include "netutils/discover.h"

#ifdef CONFIG_EXAMPLES_DISCOVER_DHCPC
#  include <arpa/inet.h>
#endif

/* Here we include the header file for the application(s) we use in
 * our project as defined in the config/<board-name>/defconfig file
 */

/* DHCPC may be used in conjunction with any other feature (or not) */

#ifdef CONFIG_EXAMPLES_DISCOVER_DHCPC
#  include "netutils/dhcpc.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * discover_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* If this task is excecutated as an NSH built-in function, then the
   * network has already been configured by NSH's start-up logic.
   */

#ifndef CONFIG_NSH_NETINIT
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_DISCOVER_DHCPC) || defined(CONFIG_EXAMPLES_DISCOVER_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_DISCOVER_DHCPC
  void *handle;
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_DISCOVER_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_DISCOVER_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = htonl(CONFIG_EXAMPLES_DISCOVER_IPADDR);
#endif
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = htonl(CONFIG_EXAMPLES_DISCOVER_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = htonl(CONFIG_EXAMPLES_DISCOVER_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

#ifdef CONFIG_EXAMPLES_DISCOVER_DHCPC
  /* Get the MAC address of the NIC */

  netlib_getmacaddr("eth0", mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open("eth0", &mac, IFHWADDRLEN);

  /* Get an IP address.
   * Note:  there is no logic here for renewing the address in this
   * example.  The address should be renewed in ds.lease_time/2 seconds.
   */

  printf("Getting IP address\n");
  if (handle)
    {
      struct dhcpc_state ds;
      char inetaddr[INET_ADDRSTRLEN];

      dhcpc_request(handle, &ds);
      netlib_set_ipv4addr("eth0", &ds.ipaddr);

      if (ds.netmask.s_addr != 0)
        {
          netlib_set_ipv4netmask("eth0", &ds.netmask);
        }

      if (ds.default_router.s_addr != 0)
        {
          netlib_set_dripv4addr("eth0", &ds.default_router);
        }

      if (ds.dnsaddr.s_addr != 0)
        {
          netlib_set_ipv4dnsaddr(&ds.dnsaddr);
        }

      dhcpc_close(handle);
      printf("IP: %s\n", inet_ntoa_r(ds.ipaddr, inetaddr, sizeof(inetaddr)));
    }

#endif /* CONFIG_EXAMPLES_DISCOVER_DHCPC */
#endif /* CONFIG_NSH_NETINIT */

  if (discover_start(NULL) < 0)
    {
      nerr("ERROR: Could not start discover daemon.\n");
      return ERROR;
    }

  return OK;
}
