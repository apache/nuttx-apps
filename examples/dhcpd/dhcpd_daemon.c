/****************************************************************************
 * apps/examples/dhcpd/dhcpd_daemon.c
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
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "netutils/netlib.h"
#include "netutils/dhcpd.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Configuration Checks *****************************************************/

/* BEWARE:
 * There are other configuration settings needed in netutils/dhcpd/dhcpdc.c,
 * but there are default values for those so we cannot check them here.
 */

#ifndef CONFIG_NET
#  error "You must define CONFIG_NET"
#endif

#ifndef CONFIG_NET_UDP
#  error "You must define CONFIG_NET_UDP"
#endif

#ifndef CONFIG_NET_BROADCAST
#  error "You must define CONFIG_NET_BROADCAST"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * dhcpd_showusage
 ****************************************************************************/

static void dhcpd_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage: %s <device-name>\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpd_daemon
 ****************************************************************************/

int dhcpd_daemon(int argc, FAR char *argv[], bool daemon)
{
  FAR const char *devname;
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_DHCPD_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif

  /* One and only one argument is expected:  The network device name. */

  if (argc != 2)
    {
      fprintf(stderr, "ERROR: Invalid number of arguments\n");
      dhcpd_showusage(argv[0], EXIT_FAILURE);
    }

  devname = argv[1];

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_DHCPD_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr(devname, mac);
#endif

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_NETUTILS_DHCPD_ROUTERIP);
  netlib_set_ipv4addr(devname, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_NETUTILS_DHCPD_ROUTERIP);
  netlib_set_dripv4addr(devname, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_NETUTILS_DHCPD_NETMASK);
  netlib_set_ipv4netmask(devname, &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup(devname);

  /* Then start the dhcpd */

  if (daemon)
    {
      return dhcpd_run(devname);
    }

  return dhcpd_start(devname);
}
