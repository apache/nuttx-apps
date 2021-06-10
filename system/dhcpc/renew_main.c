/****************************************************************************
 * apps/system/dhcpc/renew_main.c
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

#include <stdlib.h>
#include <stdio.h>

#include <net/if.h>

#include "netutils/netlib.h"
#include "netutils/dhcpc.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * dhcpc_showusage
 ****************************************************************************/

static void dhcpc_showusage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage: %s <device-name>\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * renew_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *devname;
  FAR void *handle;
  uint8_t mac[IFHWADDRLEN];
  struct dhcpc_state ds;
  int ret;

  /* One and only one argument is expected:  The network device name. */

  if (argc != 2)
    {
      fprintf(stderr, "ERROR: Invalid number of arguments\n");
      dhcpc_showusage(argv[0], EXIT_FAILURE);
    }

  devname = argv[1];

  /* Get the MAC address of the NIC */

  netlib_getmacaddr(devname, mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open(devname, &mac, IFHWADDRLEN);
  if (handle == NULL)
    {
      fprintf(stderr, "ERROR: dhcpc_open() for '%s' failed\n", devname);
      return EXIT_FAILURE;
    }

  /* Get an IP address. */

  ret = dhcpc_request(handle, &ds);
  if (ret < 0)
    {
      dhcpc_close(handle);
      fprintf(stderr, "ERROR: dhcpc_request() failed\n");
      return EXIT_FAILURE;
    }

  /* Save the addresses that we obtained. */

  netlib_set_ipv4addr(devname, &ds.ipaddr);

  if (ds.netmask.s_addr != 0)
    {
      netlib_set_ipv4netmask(devname, &ds.netmask);
    }

  if (ds.default_router.s_addr != 0)
    {
      netlib_set_dripv4addr(devname, &ds.default_router);
    }

  if (ds.dnsaddr.s_addr != 0)
    {
      netlib_set_ipv4dnsaddr(&ds.dnsaddr);
    }

  dhcpc_close(handle);
  return EXIT_SUCCESS;
}
