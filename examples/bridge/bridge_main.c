/****************************************************************************
 * examples/bridge/bridge_main.c
 *
 *   Copyright (C) 2014-2015 Gregory Nutt. All rights reserved.
 *
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <nuttx/net/arp.h>
#include "netutils/netlib.h"

#if defined(CONFIG_EXAMPLES_BRIDGE_NET1_DHCPC) || \
    defined(CONFIG_EXAMPLES_BRIDGE_NET2_DHCPC)
#  include <arpa/inet.h>
#  include "netutils/dhcpc.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static in_addr_t g_net1_ipaddr;
static in_addr_t g_net2_ipaddr;
static uint8_t g_net1_buffer[CONFIG_EXAMPLES_BRIDGE_NET1_IOBUFIZE];
static uint8_t g_net2_buffer[CONFIG_EXAMPLES_BRIDGE_NET2_IOBUFIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: briget_net1_setup
 ****************************************************************************/

static int briget_net1_setup(void)
{
  /* If this task is executed as an NSH built-in function, then the
   * network has already been configured by NSH's start-up logic.
   */

#ifndef CONFIG_NSH_NETINIT
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_BRIDGE_NET1_DHCPC) || \
    defined(CONFIG_EXAMPLES_BRIDGE_NET1_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_BRIDGE_NET1_DHCPC
  struct dhcpc_state ds;
  void *handle;
#endif

  printf("NET1: Configuring %s\n", CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME);

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_BRIDGE_NET1_NOMAC
  mac[0] = (CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR >> (8 * 5)) & 0xff;
  mac[1] = (CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR >> (8 * 4)) & 0xff;
  mac[2] = (CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR >> (8 * 3)) & 0xff;
  mac[3] = (CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR >> (8 * 2)) & 0xff;
  mac[4] = (CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR >> (8 * 1)) & 0xff;
  mac[5] = (CONFIG_EXAMPLES_BRIDGE_NET1_MACADDR >> (8 * 0)) & 0xff;
  netlib_setmacaddr(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME, mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_BRIDGE_NET1_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET1_IPADDR);
#endif
  netlib_set_ipv4addr(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET1_DRIPADDR);
  netlib_set_dripv4addr(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET1_NETMASK);
  netlib_set_ipv4netmask(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME, &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

#ifdef CONFIG_EXAMPLES_BRIDGE_NET1_DHCPC
  /* Get the MAC address of the NIC */

  netlib_getmacaddr(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME, mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open("eth0", &mac, IFHWADDRLEN);

  /* Get an IP address.  Note:  there is no logic here for renewing the
   * address in this example.  The address should be renewed in
   * ds.lease_time/2 seconds.
   */

  if (!handle)
    {
      fprintf(stderr, "NET1 ERROR: dhcpc_open failed\n");
      return ERROR;
    }

  if (dhcpc_request(handle, &ds) != OK)
    {
      fprintf(stderr, "NET1 ERROR: dhcpc_request failed\n");
      return ERROR;
    }

  netlib_set_ipv4addr(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME, &ds.ipaddr);

  if (ds.netmask.s_addr != 0)
    {
      netlib_set_ipv4netmask(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME,
                             &ds.netmask);
    }

  if (ds.default_router.s_addr != 0)
    {
      netlib_set_dripv4addr(CONFIG_EXAMPLES_BRIDGE_NET1_IFNAME,
                            &ds.default_router);
    }

  if (ds.dnsaddr.s_addr != 0)
    {
      netlib_set_ipv4dnsaddr(&ds.dnsaddr);
    }

  dhcpc_close(handle);
  printf("NET1: Assigned IP: %s\n", inet_ntoa(ds.ipaddr));

  /* Save the IP address in network order */

  g_net1_ipaddr = ds.ipaddr;

#else
  /* Save the IP address in network order */

  g_net1_ipaddr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET1_IPADDR);

#endif /* CONFIG_EXAMPLES_BRIDGE_NET1_DHCPC */

#else /* CONFIG_NSH_NETINIT */
  /* Hmmm.. there is an issue here.  Where do we get the IP address if we
   * are a builtin in application?
   */
#  warning Missing logic

#endif /* CONFIG_NSH_NETINIT */

  return OK;
}

/****************************************************************************
 * Name: briget_net2_setup
 ****************************************************************************/

static int briget_net2_setup(void)
{
  /* If this task is executed as an NSH built-in function, then the
   * network has already been configured by NSH's start-up logic.
   */

#ifndef CONFIG_NSH_NETINIT
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_BRIDGE_NET2_DHCPC) || \
    defined(CONFIG_EXAMPLES_BRIDGE_NET2_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif
#ifdef CONFIG_EXAMPLES_BRIDGE_NET2_DHCPC
  struct dhcpc_state ds;
  void *handle;
#endif

  printf("NET2: Configuring %s\n", CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME);

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_BRIDGE_NET2_NOMAC
  mac[0] = (CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR >> (8 * 5)) & 0xff;
  mac[1] = (CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR >> (8 * 4)) & 0xff;
  mac[2] = (CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR >> (8 * 3)) & 0xff;
  mac[3] = (CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR >> (8 * 2)) & 0xff;
  mac[4] = (CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR >> (8 * 1)) & 0xff;
  mac[5] = (CONFIG_EXAMPLES_BRIDGE_NET2_MACADDR >> (8 * 0)) & 0xff;
  netlib_setmacaddr(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME, mac);
#endif

  /* Set up our host address */

#ifdef CONFIG_EXAMPLES_BRIDGE_NET2_DHCPC
  addr.s_addr = 0;
#else
  addr.s_addr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET2_IPADDR);
#endif
  netlib_set_ipv4addr(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET2_DRIPADDR);
  netlib_set_dripv4addr(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET2_NETMASK);
  netlib_set_ipv4netmask(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME, &addr);

#ifdef CONFIG_EXAMPLES_BRIDGE_NET2_DHCPC
  /* Get the MAC address of the NIC */

  netlib_getmacaddr(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME, mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open(&mac, IFHWADDRLEN);

  /* Get an IP address.  Note:  there is no logic here for renewing the
   * address in this example.  The address should be renewed in
   * ds.lease_time/2 seconds.
   */

  if (!handle)
    {
      fprintf(stderr, "NET2 ERROR: dhcpc_open failed\n");
      return ERROR;
    }

  if (dhcpc_request(handle, &ds) != OK)
    {
      fprintf(stderr, "NET2 ERROR: dhcpc_request failed\n");
      return ERROR;
    }

  netlib_set_ipv4addr(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME, &ds.ipaddr);

  if (ds.netmask.s_addr != 0)
    {
      netlib_set_ipv4netmask(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME,
                             &ds.netmask);
    }

  if (ds.default_router.s_addr != 0)
    {
      netlib_set_dripv4addr(CONFIG_EXAMPLES_BRIDGE_NET2_IFNAME,
                            &ds.default_router);
    }

  if (ds.dnsaddr.s_addr != 0)
    {
      netlib_set_ipv4dnsaddr(&ds.dnsaddr);
    }

  dhcpc_close(handle);
  printf("NET1: Assigned IP: %s\n", inet_ntoa(ds.ipaddr));

  /* Save the IP address in network order */

  g_net2_ipaddr = ds.ipaddr;

#else
  /* Save the IP address in network order */

  g_net2_ipaddr = HTONL(CONFIG_EXAMPLES_BRIDGE_NET2_IPADDR);

#endif /* CONFIG_EXAMPLES_BRIDGE_NET2_DHCPC */

#else /* CONFIG_NSH_NETINIT */
  /* Hmmm.. there is an issue here.  Where do we get the IP address if we
   * are a builtin in application?
   */
#  warning Missing logic

#endif /* CONFIG_NSH_NETINIT */

  return OK;
}

/****************************************************************************
 * Name: bridge_net1_worker
 ****************************************************************************/

static int bridge_net1_worker(int argc, char *argv[])
{
  struct sockaddr_in receiver;
  struct sockaddr_in sender;
  struct sockaddr_in fromaddr;
  struct sockaddr_in toaddr;
  socklen_t addrlen;
  in_addr_t tmpaddr;
  ssize_t nrecvd;
  ssize_t nsent;
  int optval;
  int recvsd;
  int sndsd;

  printf("NET1: Worker started.  PID=%d\n", getpid());

  /* Create a UDP receive socket on network 1 */

  tmpaddr = ntohl(g_net1_ipaddr);
  printf("NET1: Create receive socket: %d.%d.%d.%d:%d\n",
         (int)(tmpaddr >> 24),
         (int)((tmpaddr >> 16) & 0xff),
         (int)((tmpaddr >> 8) & 0xff),
         (int)(tmpaddr & 0xff),
         CONFIG_EXAMPLES_BRIDGE_NET1_RECVPORT);

  recvsd = socket(PF_INET, SOCK_DGRAM, 0);
  if (recvsd < 0)
    {
      fprintf(stderr, "NET1 ERROR: Failed to create receive socket: %d\n",
              errno);
      return EXIT_FAILURE;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(recvsd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval,
                 sizeof(int)) < 0)
    {
      fprintf(stderr, "NET1 ERROR: setsockopt SO_REUSEADDR failure: %d\n",
              errno);
      goto errout_with_recvsd;
    }

  /* Bind the socket to a local address */

  receiver.sin_family      = AF_INET;
  receiver.sin_port        = HTONS(CONFIG_EXAMPLES_BRIDGE_NET1_RECVPORT);
  receiver.sin_addr.s_addr = g_net1_ipaddr;

  if (bind(recvsd, (struct sockaddr *)&receiver,
           sizeof(struct sockaddr_in)) < 0)
    {
      fprintf(stderr, "NET1 ERROR: bind failure: %d\n", errno);
      goto errout_with_recvsd;
    }

  /* Create a UDP send socket on network 2 */

  tmpaddr = ntohl(g_net2_ipaddr);
  printf("NET1: Create send socket: %d.%d.%d.%d:INPORT_ANY\n",
         (int)(tmpaddr >> 24),
         (int)((tmpaddr >> 16) & 0xff),
         (int)((tmpaddr >> 8) & 0xff),
         (int)(tmpaddr & 0xff));

  sndsd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sndsd < 0)
    {
      fprintf(stderr, "NET1 ERROR: Failed to create send socket: %d\n",
              errno);
      goto errout_with_recvsd;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(sndsd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval,
                 sizeof(int)) < 0)
    {
      fprintf(stderr, "NET1 ERROR: setsockopt SO_REUSEADDR failure: %d\n",
              errno);
      goto errout_with_sendsd;
    }

  /* Bind the socket to a local address */

  sender.sin_family      = AF_INET;
  sender.sin_port        = 0;
  sender.sin_addr.s_addr = g_net2_ipaddr;

  if (bind(sndsd, (struct sockaddr *)&sender,
           sizeof(struct sockaddr_in)) < 0)
    {
      printf("NET1: bind failure: %d\n", errno);
      goto errout_with_sendsd;
    }

  /* Then receive and forward UDP packets forever  */

  for (; ; )
    {
      /* Read a packet on network 1 */

      printf("NET1: Receiving up to %d bytes on network 1\n",
             CONFIG_EXAMPLES_BRIDGE_NET1_IOBUFIZE);

      addrlen = sizeof(struct sockaddr_in);
      nrecvd = recvfrom(recvsd, g_net1_buffer,
                        CONFIG_EXAMPLES_BRIDGE_NET1_IOBUFIZE, 0,
                        (FAR struct sockaddr *)&fromaddr, &addrlen);

      tmpaddr = ntohl(fromaddr.sin_addr.s_addr);
      printf("NET1: Received %ld bytes from %d.%d.%d.%d:%d\n",
             (long)nrecvd,
             (int)(tmpaddr >> 24),
             (int)((tmpaddr >> 16) & 0xff),
             (int)((tmpaddr >> 8) & 0xff),
             (int)(tmpaddr & 0xff),
             ntohs(fromaddr.sin_port));

      /* Check for a receive error or zero bytes received.  The negative
       * return value indicates a receive error; Zero would mean that the
       * other side of the "connection" performed an "orderly" shutdown.
       * This should not occur with a UDP socket and so must also be an
       * error of some kind.
       */

      if (nrecvd <= 0)
        {
          if (nrecvd < 0)
            {
              fprintf(stderr, "NET1 ERROR: recvfrom failed: %d\n", errno);
            }
          else
            {
              fprintf(stderr, "NET1 ERROR: recvfrom returned zero\n");
            }

          goto errout_with_sendsd;
        }

      /* Send the newly received packet out network 2 */

      printf("NET1: Sending %ld bytes on network 2: %d.%d.%d.%d:%d\n",
             (long)nrecvd,
             CONFIG_EXAMPLES_BRIDGE_NET2_IPHOST >> 24,
             (CONFIG_EXAMPLES_BRIDGE_NET2_IPHOST >> 16) & 0xff,
             (CONFIG_EXAMPLES_BRIDGE_NET2_IPHOST >> 8) & 0xff,
             CONFIG_EXAMPLES_BRIDGE_NET2_IPHOST & 0xff,
             CONFIG_EXAMPLES_BRIDGE_NET2_HOSTPORT);

      toaddr.sin_family      = AF_INET;
      toaddr.sin_port        = htons(CONFIG_EXAMPLES_BRIDGE_NET2_HOSTPORT);
      toaddr.sin_addr.s_addr = htonl(CONFIG_EXAMPLES_BRIDGE_NET2_IPHOST);

      nsent = sendto(sndsd, g_net1_buffer, nrecvd, 0,
                     (struct sockaddr *)&toaddr,
                     sizeof(struct sockaddr_in));

      /* Check for send errors */

      if (nsent < 0)
        {
          fprintf(stderr, "NET1 ERROR: sendto failed: %d\n", errno);
          goto errout_with_sendsd;
        }
      else if (nsent != nrecvd)
        {
          fprintf(stderr, "NET1 ERROR: Bad send length: %ld Expected: %ld\n",
                  (long)nsent, (long)nrecvd);
          goto errout_with_sendsd;
        }
    }

  close(recvsd);
  close(recvsd);
  return EXIT_SUCCESS;

errout_with_sendsd:
  close(sndsd);
errout_with_recvsd:
  close(recvsd);
  return EXIT_FAILURE;
}

/****************************************************************************
 * Name: bridge_net2_worker
 ****************************************************************************/

static int bridge_net2_worker(int argc, char *argv[])
{
  struct sockaddr_in receiver;
  struct sockaddr_in sender;
  struct sockaddr_in fromaddr;
  struct sockaddr_in toaddr;
  socklen_t addrlen;
  in_addr_t tmpaddr;
  ssize_t nrecvd;
  ssize_t nsent;
  int optval;
  int recvsd;
  int sndsd;

  printf("NET2: Worker started.  PID=%d\n", getpid());

  /* Create a UDP receive socket on network 2 */

  tmpaddr = ntohl(g_net2_ipaddr);
  printf("NET2: Create receive socket: %d.%d.%d.%d:%d\n",
         (int)(tmpaddr >> 24),
         (int)((tmpaddr >> 16) & 0xff),
         (int)((tmpaddr >> 8) & 0xff),
         (int)(tmpaddr & 0xff),
         CONFIG_EXAMPLES_BRIDGE_NET2_RECVPORT);

  recvsd = socket(PF_INET, SOCK_DGRAM, 0);
  if (recvsd < 0)
    {
      fprintf(stderr, "NET2 ERROR: Failed to create receive socket: %d\n",
              errno);
      return EXIT_FAILURE;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(recvsd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval,
                 sizeof(int)) < 0)
    {
      fprintf(stderr, "NET2 ERROR: setsockopt SO_REUSEADDR failure: %d\n",
              errno);
      goto errout_with_recvsd;
    }

  /* Bind the socket to a local address */

  receiver.sin_family      = AF_INET;
  receiver.sin_port        = HTONS(CONFIG_EXAMPLES_BRIDGE_NET2_RECVPORT);
  receiver.sin_addr.s_addr = g_net2_ipaddr;

  if (bind(recvsd, (struct sockaddr *)&receiver,
           sizeof(struct sockaddr_in)) < 0)
    {
      fprintf(stderr, "NET2 ERROR: bind failure: %d\n", errno);
      goto errout_with_recvsd;
    }

  /* Create a UDP send socket on network 1 */

  tmpaddr = ntohl(g_net1_ipaddr);
  printf("NET2: Create send socket: %d.%d.%d.%d:INPORT_ANY\n",
         (int)(tmpaddr >> 24),
         (int)((tmpaddr >> 16) & 0xff),
         (int)((tmpaddr >> 8) & 0xff),
         (int)(tmpaddr & 0xff));

  sndsd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sndsd < 0)
    {
      fprintf(stderr, "NET2 ERROR: Failed to create send socket: %d\n",
              errno);
      goto errout_with_recvsd;
    }

  /* Set socket to reuse address */

  optval = 1;
  if (setsockopt(sndsd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval,
                 sizeof(int)) < 0)
    {
      fprintf(stderr, "NET2 ERROR: setsockopt SO_REUSEADDR failure: %d\n",
              errno);
      goto errout_with_sendsd;
    }

  /* Bind the socket to a local address */

  sender.sin_family      = AF_INET;
  sender.sin_port        = 0;
  sender.sin_addr.s_addr = g_net1_ipaddr;

  if (bind(sndsd, (struct sockaddr *)&sender,
           sizeof(struct sockaddr_in)) < 0)
    {
      printf("NET2: bind failure: %d\n", errno);
      goto errout_with_sendsd;
    }

  /* Then receive and forward UDP packets forever  */

  for (; ; )
    {
      /* Read a packet on network 2 */

      printf("NET2: Receiving up to %d bytes on network 2\n",
             CONFIG_EXAMPLES_BRIDGE_NET2_IOBUFIZE);

      addrlen = sizeof(struct sockaddr_in);
      nrecvd = recvfrom(recvsd, g_net2_buffer,
                        CONFIG_EXAMPLES_BRIDGE_NET2_IOBUFIZE, 0,
                        (FAR struct sockaddr *)&fromaddr, &addrlen);

      tmpaddr = ntohl(fromaddr.sin_addr.s_addr);
      printf("NET2: Received %ld bytes from %d.%d.%d.%d:%d\n",
             (long)nrecvd,
             (int)(tmpaddr >> 24),
             (int)((tmpaddr >> 16) & 0xff),
             (int)((tmpaddr >> 8) & 0xff),
             (int)(tmpaddr & 0xff),
             ntohs(fromaddr.sin_port));

      /* Check for a receive error or zero bytes received.  The negative
       * return value indicates a receive error; Zero would mean that the
       * other side of the "connection" performed an "orderly" shutdown.
       * This should not occur with a UDP socket and so must also be an
       * error of some kind.
       */

      if (nrecvd <= 0)
        {
          if (nrecvd < 0)
            {
              fprintf(stderr, "NET2 ERROR: recvfrom failed: %d\n", errno);
            }
          else
            {
              fprintf(stderr, "NET2 ERROR: recvfrom returned zero\n");
            }

          goto errout_with_sendsd;
        }

      /* Send the newly received packet out network 1 */

      printf("NET2: Sending %ld bytes on network 1: %d.%d.%d.%d:%d\n",
             (long)nrecvd,
             CONFIG_EXAMPLES_BRIDGE_NET1_IPHOST >> 24,
             (CONFIG_EXAMPLES_BRIDGE_NET1_IPHOST >> 16) & 0xff,
             (CONFIG_EXAMPLES_BRIDGE_NET1_IPHOST >> 8) & 0xff,
             CONFIG_EXAMPLES_BRIDGE_NET1_IPHOST & 0xff,
             CONFIG_EXAMPLES_BRIDGE_NET1_HOSTPORT);

      toaddr.sin_family      = AF_INET;
      toaddr.sin_port        = htons(CONFIG_EXAMPLES_BRIDGE_NET1_HOSTPORT);
      toaddr.sin_addr.s_addr = htonl(CONFIG_EXAMPLES_BRIDGE_NET1_IPHOST);

      nsent = sendto(sndsd, g_net2_buffer, nrecvd, 0,
                     (struct sockaddr *)&toaddr, sizeof(struct sockaddr_in));

      /* Check for send errors */

      if (nsent < 0)
        {
          fprintf(stderr, "NET2 ERROR: sendto failed: %d\n", errno);
          goto errout_with_sendsd;
        }
      else if (nsent != nrecvd)
        {
          fprintf(stderr, "NET2 ERROR: Bad send length: %ld Expected: %ld\n",
                  (long)nsent, (long)nrecvd);
          goto errout_with_sendsd;
        }
    }

  close(recvsd);
  close(recvsd);
  return EXIT_SUCCESS;

errout_with_sendsd:
  close(sndsd);
errout_with_recvsd:
  close(recvsd);
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * bridge_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pid_t net1_worker;
  pid_t net2_worker;
  int ret;

  /* Initialize networks */

  ret = briget_net1_setup();
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: Failed to setup network 1\n");
      return EXIT_FAILURE;
    }

  ret = briget_net2_setup();
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: Failed to setup network 2\n");
      return EXIT_FAILURE;
    }

  /* Start network daemons */

  printf("Start network 1 worker\n");

  net1_worker = task_create("NET1 Worker",
                            CONFIG_EXAMPLES_BRIDGE_NET1_PRIORITY,
                            CONFIG_EXAMPLES_BRIDGE_NET1_STACKSIZE,
                            bridge_net1_worker,
                            NULL);
  if (net1_worker < 0)
    {
      fprintf(stderr, "ERROR: Failed to start network daemon 1\n");
      return EXIT_FAILURE;
    }

  printf("Start network 2 worker\n");

  net2_worker = task_create("NET2 Worker",
                            CONFIG_EXAMPLES_BRIDGE_NET2_PRIORITY,
                            CONFIG_EXAMPLES_BRIDGE_NET2_STACKSIZE,
                            bridge_net2_worker,
                            NULL);
  if (net2_worker < 0)
    {
      fprintf(stderr, "ERROR: Failed to start network daemon 2\n");
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
