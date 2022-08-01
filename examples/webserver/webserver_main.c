/****************************************************************************
 * apps/examples/webserver/webserver_main.c
 *
 *   Copyright (C) 2007, 2009-2012, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Based on uIP which also has a BSD style license:
 *
 *   Copyright (c) 2001, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
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

#ifdef CONFIG_EXAMPLES_WEBSERVER_DHCPC
#include <arpa/inet.h>
#endif

/* Here we include the header file for the application(s) we use in
 * our project as defined in the config/<board-name>/defconfig file
 */

/* DHCPC may be used in conjunction with any other feature (or not) */

#ifdef CONFIG_EXAMPLES_WEBSERVER_DHCPC
# include "netutils/dhcpc.h"
#endif

/* Include uIP webserver definitions */

#include "netutils/httpd.h"

#include "cgi.h"

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
 * webserver_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifndef CONFIG_NSH_NETINIT
  /* We are running standalone (as opposed to a NSH built-in app). Therefore
   * we need to initialize the network before we start.
   */

  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_WEBSERVER_DHCPC) || defined(CONFIG_EXAMPLES_WEBSERVER_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_WEBSERVER_DHCPC
  void *handle;
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_WEBSERVER_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_WEBSERVER_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = HTONL(CONFIG_EXAMPLES_WEBSERVER_IPADDR);
#endif
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_WEBSERVER_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_WEBSERVER_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

#ifdef CONFIG_EXAMPLES_WEBSERVER_DHCPC
  /* Get the MAC address of the NIC */

  netlib_getmacaddr("eth0", mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open("eth0", &mac, IFHWADDRLEN);

  /* Get an IP address.  Note:  there is no logic here for renewing the
   * address in this example.  The address should be renewed in
   * ds.lease_time/2 seconds.
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
#endif
#endif /* CONFIG_NSH_NETINIT */

#ifdef CONFIG_NET_TCP
  printf("Starting webserver\n");
  httpd_init();
#ifndef CONFIG_NETUTILS_HTTPD_SCRIPT_DISABLE
  cgi_register();
#endif
  httpd_listen();
#endif

#ifndef CONFIG_NSH_NETINIT
  /* We are running standalone (as opposed to a NSH built-in app). Therefore
   * we should not exit after httpd failure.
   */

  while (1)
    {
      sleep(3);
      printf("webserver_main: Still running\n");
      fflush(stdout);
    }

#else /* CONFIG_NSH_NETINIT */
  /* We are running as a NSH built-in app.  Therefore we should exit.  This
   * allows to 'kill -9' the webserver app, assuming it was started as a
   * background process.  For example:
   *
   *    nsh> webserver &
   *    webserver [6:100]
   *    nsh> Starting webserver
   *
   *    nsh> kill -9 6
   *    nsh> webserver_main: Exiting
   */

  printf("webserver_main: Exiting\n");
  fflush(stdout);

#endif /* CONFIG_NSH_NETINIT */

  return 0;
}
