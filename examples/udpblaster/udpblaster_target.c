/****************************************************************************
 * apps/examples/udpblaster/udpblaster_target.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "netutils/netlib.h"

#include "udpblaster.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_UDPBLASTER_INIT) && \
    defined(CONFIG_EXAMPLES_UDPBLASTER_IPv6) && \
    !defined(CONFIG_NET_ICMPv6_AUTOCONF)
/* Our host IPv6 address */

static const uint16_t g_ipv6_hostaddr[8] =
{
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_1),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_2),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_3),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_4),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_5),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_6),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_7),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_8),
};

/* Default routine IPv6 address */

static const uint16_t g_ipv6_draddr[8] =
{
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_1),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_2),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_3),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_4),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_5),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_6),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_7),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_8),
};

/* IPv6 netmask */

static const uint16_t g_ipv6_netmask[8] =
{
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_1),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_2),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_3),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_4),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_5),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_6),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_7),
  HTONS(CONFIG_EXAMPLES_UDPBLASTER_IPv6NETMASK_8),
};
#endif /* CONFIG_EXAMPLES_UDPBLASTER_INIT && CONFIG_EXAMPLES_UDPBLASTER_IPv6 && !CONFIG_NET_ICMPv6_AUTOCONF */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_UDPBLASTER_INIT
static void netest_initialize(void)
{
#ifndef CONFIG_EXAMPLES_UDPBLASTER_IPv6
  struct in_addr addr;
#endif
#ifdef CONFIG_EXAMPLES_UDPBLASTER_NOMAC
  uint8_t mac[IFHWADDRLEN];
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_UDPBLASTER_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

#ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv6
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
#else /* CONFIG_EXAMPLES_UDPBLASTER_IPv6 */

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_UDPBLASTER_TARGETIP);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_UDPBLASTER_HOSTIP);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_UDPBLASTER_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

#endif /* CONFIG_EXAMPLES_UDPBLASTER_IPv6 */

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");
}
#endif /* CONFIG_EXAMPLES_UDPBLASTER_INIT */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * udpblaster_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv4
  struct sockaddr_in host;
  struct sockaddr_in target;
#else
  struct sockaddr_in6 host;
  struct sockaddr_in6 target;
#endif
  socklen_t addrlen;
  int npackets;
  int ndots;
  int sockfd;
  int ret;

#ifdef CONFIG_EXAMPLES_UDPBLASTER_INIT
  /* Initialize the network */

  netest_initialize();
#endif

#ifdef CONFIG_EXAMPLES_UDPBLASTER_IPv4
  host.sin_family             = AF_INET;
  host.sin_port               = HTONS(UDPBLASTER_HOST_PORTNO);
  host.sin_addr.s_addr        = HTONL(CONFIG_EXAMPLES_UDPBLASTER_HOSTIP);

  sockfd                      = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

  target.sin_family             = AF_INET;
  target.sin_port               = HTONS(UDPBLASTER_TARGET_PORTNO);
  target.sin_addr.s_addr        = HTONL(CONFIG_EXAMPLES_UDPBLASTER_TARGETIP);
  addrlen                       = sizeof(struct sockaddr_in);

  if (bind(sockfd, (struct sockaddr *)&target, addrlen) < 0)
    {
      printf("server: ERROR bind failure: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

#else
  host.sin6_family              = AF_INET6;
  host.sin6_port                = HTONS(UDPBLASTER_HOST_PORTNO);
  host.sin6_addr.s6_addr16[0]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_1);
  host.sin6_addr.s6_addr16[1]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_2);
  host.sin6_addr.s6_addr16[2]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_3);
  host.sin6_addr.s6_addr16[3]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_4);
  host.sin6_addr.s6_addr16[4]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_5);
  host.sin6_addr.s6_addr16[5]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_6);
  host.sin6_addr.s6_addr16[6]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_7);
  host.sin6_addr.s6_addr16[7]   =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_HOSTIPv6_8);

  sockfd                        = socket(PF_INET6, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

  target.sin6_family            = AF_INET6;
  target.sin6_port              = HTONS(UDPBLASTER_TARGET_PORTNO);
  target.sin6_addr.s6_addr16[0] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_1);
  target.sin6_addr.s6_addr16[1] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_2);
  target.sin6_addr.s6_addr16[2] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_3);
  target.sin6_addr.s6_addr16[3] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_4);
  target.sin6_addr.s6_addr16[4] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_5);
  target.sin6_addr.s6_addr16[5] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_6);
  target.sin6_addr.s6_addr16[6] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_7);
  target.sin6_addr.s6_addr16[7] =
    HTONS(CONFIG_EXAMPLES_UDPBLASTER_TARGETIPv6_8);
  addrlen                       = sizeof(struct sockaddr_in6);

  if (bind(sockfd, (struct sockaddr *)&target, addrlen) < 0)
    {
      fprintf(stderr, "ERROR bind failure: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }
#endif

  npackets = 0;
  ndots    = 0;

  for (; ; )
    {
#ifdef CONFIG_EXAMPLES_UDPBLASTER_POLLOUT
      struct pollfd fds[1];

      memset(fds, 0, 1 * sizeof(struct pollfd));
      fds[0].fd     = sockfd;
      fds[0].events = POLLOUT | POLLHUP;

      /* Wait until we can send data or until the connection is lost */

      ret = poll(fds, 1, -1);
      if (ret < 0)
        {
          printf("client: ERROR poll failed: %d\n", errno);
          goto errout_with_socket;
        }

      if ((fds[0].revents & POLLHUP) != 0)
        {
          printf("client: WARNING poll returned POLLHUP\n");
          ret = EXIT_SUCCESS;
          goto errout_with_socket;
        }
#endif

      ret = sendto(sockfd, g_udpblaster_text, UDPBLASTER_SENDSIZE, 0,
                   (struct sockaddr *)&host, addrlen);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: sendto() failed: %d\n", errno);
          ret = EXIT_FAILURE;
          goto errout_with_socket;
        }

      if (++npackets >= 10000)
        {
          putchar('.');
          npackets = 0;

          if (++ndots >= 50)
            {
              putchar('\n');
              ndots = 0;
            }
        }
    }

errout_with_socket:
  close(sockfd);
  return ret;
}
