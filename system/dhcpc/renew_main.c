/****************************************************************************
 * system/dhcpc/renew_main.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int renew_main(int argc, char *argv[])
#endif
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
      (void)dhcpc_close(handle);
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
