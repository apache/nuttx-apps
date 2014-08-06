/****************************************************************************
 * apps/nshlib/nsh_netinit.c
 *
 *   Copyright (C) 2010-2012, 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * This is influenced by similar logic from uIP:
 *
 *   Author: Adam Dunkels <adam@sics.se>
 *   Copyright (c) 2003, Adam Dunkels.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <pthread.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <apps/netutils/netlib.h>
#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
#  include <apps/netutils/dnsclient.h>
#  include <apps/netutils/dhcpc.h>
#endif

#include "nsh.h"

#ifdef CONFIG_NET

/****************************************************************************
 * Definitions
 ****************************************************************************/

#if defined(CONFIG_NSH_DRIPADDR) && !defined(CONFIG_NSH_DNSIPADDR)
#  define CONFIG_NSH_DNSIPADDR CONFIG_NSH_DRIPADDR
#endif

/* SLIP-specific configuration
 *
 * REVISIT: How will we handle Ethernet and SLIP networks together?  In the
 * future, NSH will need to be extended to handle multiple networks with
 * mixed transports.
 */

#ifdef CONFIG_NET_SLIP
#  define NET_DEVNAME "sl0"
#  ifndef CONFIG_NSH_NOMAC
#    error "CONFIG_NSH_NOMAC must be defined for SLIP"
#  endif
#else
#  define NET_DEVNAME "eth0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_netinit_thread
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

pthread_addr_t nsh_netinit_thread(pthread_addr_t arg)
{
  struct in_addr addr;
#if defined(CONFIG_NSH_DHCPC)
  FAR void *handle;
#endif
#if (defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_NOMAC)) && !defined(CONFIG_NET_SLIP)
  uint8_t mac[IFHWADDRLEN];
#endif

  nvdbg("Entry\n");

  /* Many embedded network interfaces must have a software assigned MAC */

#if defined(CONFIG_NSH_NOMAC) && !defined(CONFIG_NET_SLIP)
#ifdef CONFIG_NSH_ARCHMAC
  /* Let platform-specific logic assign the MAC address. */

  (void)nsh_arch_macaddress(mac);

#else
  /* Use the configured, fixed MAC address */

  mac[0] = (CONFIG_NSH_MACADDR >> (8 * 5)) & 0xff;
  mac[1] = (CONFIG_NSH_MACADDR >> (8 * 4)) & 0xff;
  mac[2] = (CONFIG_NSH_MACADDR >> (8 * 3)) & 0xff;
  mac[3] = (CONFIG_NSH_MACADDR >> (8 * 2)) & 0xff;
  mac[4] = (CONFIG_NSH_MACADDR >> (8 * 1)) & 0xff;
  mac[5] = (CONFIG_NSH_MACADDR >> (8 * 0)) & 0xff;
#endif

  /* Set the MAC address */

  netlib_setmacaddr(NET_DEVNAME, mac);
#endif

  /* Set up our host address */

#if !defined(CONFIG_NSH_DHCPC)
  addr.s_addr = HTONL(CONFIG_NSH_IPADDR);
#else
  addr.s_addr = 0;
#endif
  netlib_sethostaddr(NET_DEVNAME, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_NSH_DRIPADDR);
  netlib_setdraddr(NET_DEVNAME, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_NSH_NETMASK);
  netlib_setnetmask(NET_DEVNAME, &addr);

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
  /* Set up the resolver */

  dns_bind();
#if defined(CONFIG_NSH_DNS)
  addr.s_addr = HTONL(CONFIG_NSH_DNSIPADDR);
  dns_setserver(&addr);
#endif
#endif

#if defined(CONFIG_NSH_DHCPC)
  /* Get the MAC address of the NIC */

  netlib_getmacaddr(NET_DEVNAME, mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open(&mac, IFHWADDRLEN);

  /* Get an IP address.  Note that there is no logic for renewing the IP address in this
   * example.  The address should be renewed in ds.lease_time/2 seconds.
   */

  if (handle)
    {
        struct dhcpc_state ds;
        (void)dhcpc_request(handle, &ds);
        netlib_sethostaddr(NET_DEVNAME, &ds.ipaddr);

        if (ds.netmask.s_addr != 0)
          {
            netlib_setnetmask(NET_DEVNAME, &ds.netmask);
          }

        if (ds.default_router.s_addr != 0)
          {
            netlib_setdraddr(NET_DEVNAME, &ds.default_router);
          }

        if (ds.dnsaddr.s_addr != 0)
          {
            dns_setserver(&ds.dnsaddr);
          }

        dhcpc_close(handle);
    }
#endif

  nvdbg("Exit\n");
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_netinit
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

int nsh_netinit(void)
{
#ifdef CONFIG_NSH_NETINIT_THREAD
  struct sched_param  sparam;
  pthread_attr_t      attr;
  pthread_t           tid;
  void               *value;
  int                 ret;

  /* Start the network initialization thread to perform the network bring-up
   * asynchronously.
   */

  pthread_attr_init(&attr);
  sparam.sched_priority = CONFIG_NSH_NETINIT_THREAD_PRIORITY;
  (void)pthread_attr_setschedparam(&attr, &sparam);
  (void)pthread_attr_setstacksize(&attr, CONFIG_NSH_NETINIT_THREAD_STACKSIZE);

  nvdbg("Starting netinit thread\n");
  ret = pthread_create(&tid, &attr, nsh_netinit_thread, NULL);
  if (ret != OK)
    {
      ndbg("ERROR: Failed to create netinit thread: %d\n", ret);
      (void)nsh_netinit_thread(NULL);
    }
  else
    {
      /* Detach the thread because we will not be joining to it */

      (void)pthread_detach(tid);

      /* Name the thread */

      pthread_setname_np(tid, "netinit");
    }

  return OK;

#else
  /* Perform network initialization sequentially */

  (void)nsh_netinit_thread(NULL);
#endif
}

#endif /* CONFIG_NET */
