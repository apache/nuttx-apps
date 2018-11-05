/****************************************************************************
 * examples/mld/mld_main.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include "netutils/netlib.h"

#include "mld.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_MLD_INIT
static const uint16_t g_host_addr[8] =
{
  HTONS(0xfe80),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0x00ff),
  HTONS(0xfe00),
  HTONS(CONFIG_EXAMPLES_MLD_IPADDR)
};

static const uint16_t g_dr_addr[8] =
{
  HTONS(0xfe80),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0x00ff),
  HTONS(0xfe00),
  HTONS(CONFIG_EXAMPLES_MLD_DRIPADDR)
};

static const uint16_t g_netmask[8] =
{
  HTONS(0xfffff),
  HTONS(0xfffff),
  HTONS(0xfffff),
  HTONS(0xfffff),
  HTONS(0xfffff),
  HTONS(0xfffff),
  HTONS(0xfffff),
  HTONS(0)
};
#endif

static const uint16_t g_grp_addr[8] =
{
  HTONS(0xfc00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(CONFIG_EXAMPLES_MLD_GRPADDR)
};

static const uint8_t g_garbage[] = "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mld_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int mld_main(int argc, char *argv[])
#endif
{
  struct sockaddr_in6 host;
#if 0
  struct sockaddr_in6 mcast;
#endif
  struct ipv6_mreq mrec;
  int sockfd;
  int ret;

#ifdef CONFIG_EXAMPLES_MLD_INIT
#ifdef CONFIG_EXAMPLES_MLD_NOMAC
  uint8_t mac[IFHWADDRLEN];
#endif

  printf("Configuring Ethernet...\n");

#ifdef CONFIG_EXAMPLES_MLD_NOMAC
  /* Many embedded network interfaces must have a software assigned MAC */

  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our (fixed) host address */

  netlib_set_ipv6addr("eth0", (FAR const struct in6_addr *)g_host_addr);

  /* Set up the default router address */

  netlib_set_dripv6addr("eth0", (FAR const struct in6_addr *)g_dr_addr);

  /* Setup the subnet mask */

  netlib_set_ipv6netmask("eth0", (FAR const struct in6_addr *)g_netmask);

  /* Bring the network up */

  netlib_ifup("eth0");
#endif /* CONFIG_EXAMPLES_MLD_INIT */

  memset(&host, 0, sizeof(struct sockaddr_in6));
  host.sin6_family  = AF_INET6;
  sockfd            = socket(PF_INET6, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

  /* Join the group */

  printf("Join group...\n");

  memcpy(mrec.ipv6mr_multiaddr.s6_addr16, g_grp_addr, sizeof(struct in6_addr));
  mrec.ipv6mr_interface = if_nametoindex("eth0");

  ret = setsockopt(sockfd, SOL_IPV6, IPV6_JOIN_GROUP, (FAR void *)&mrec,
                   sizeof(struct ipv6_mreq));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: setsockopt() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

  /* Wait a while */

  printf("Wait a bit...\n");
  sleep(5);

#if 0 /* REVISIT:  This is not right */
  /* Send a garbage packet */

  memset(&mcast, 0, sizeof(struct sockaddr_in6));
  mcast.sin6_family  = AF_INET6;
  mcast.sin6_port    = HTONS(0x4321);
  memcpy(mcast.sin6_addr.s6_addr16, g_grp_addr, sizeof(struct in6_addr));

  ret = sendto(sockfd, g_garbage, sizeof(g_garbage), 0,
               (FAR struct sockaddr *)&mcast, sizeof(struct sockaddr_in6));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sendto() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

  /* Wait a while */

  printf("Wait a bit...\n");
  sleep(5);
#endif

  /* Leave the group */

  printf("Leave group...\n");
  ret = setsockopt(sockfd, SOL_IPV6, IPV6_LEAVE_GROUP, (FAR void *)&mrec,
                   sizeof(struct ipv6_mreq));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: setsockopt() failed: %d\n", errno);
      ret = EXIT_FAILURE;
      goto errout_with_socket;
    }

  /* Wait a while */

  printf("Wait a bit...\n");
  sleep(5);
  printf("Exiting...\n");
  ret = EXIT_SUCCESS;

errout_with_socket:
  close(sockfd);
  return ret;
}
