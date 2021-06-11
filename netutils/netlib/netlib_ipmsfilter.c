/****************************************************************************
 * apps/netutils/netlib/netlib_ipmsfilter.c
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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>
#include <sys/sockio.h>

#include "netutils/netlib.h"
#include "netutils/ipmsfilter.h"

#ifdef CONFIG_NET_IGMP

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipmsfilter
 *
 * Description:
 *   Add or remove an IP address from a multicast filter set.
 *
 * Parameters:
 *   interface  The local address of the local interface to use.
 *   multiaddr  Multicast group address to add/remove (network byte order)
 *   fmode      MCAST_INCLUDE: Add multicast address
 *              MCAST_EXCLUDE: Remove multicast address
 *
 * Return:
 *   0 on success; Negated errno on failure
 *
 ****************************************************************************/

int ipmsfilter(FAR const struct in_addr *interface,
               FAR const struct in_addr *multiaddr,
               uint32_t fmode)
{
  int ret = ERROR;

  if (interface != NULL && multiaddr != NULL)
    {
      ninfo("interface: %08" PRIx32 " muliaddr: %08" PRIx32
            " fmode: %" PRId32 "\n",
            (uint32_t)interface->s_addr,
            (uint32_t)multiaddr->s_addr, fmode);

      /* Get a socket (only so that we get access to the INET subsystem) */

      int sockfd = socket(PF_INET, NETLIB_SOCK_TYPE, 0);
      if (sockfd >= 0)
        {
          struct ip_msfilter imsf;

          /* Put the multicast group address into the request */

          imsf.imsf_multiaddr.s_addr = multiaddr->s_addr;

          /* Put the address of the local interface into the request */

          imsf.imsf_interface.s_addr = interface->s_addr;

          /* Put the filter mode into the request */

          imsf.imsf_fmode = fmode;

          /* No source address */

          imsf.imsf_numsrc = 0;

          /* Perform the ioctl to set the MAC address */

          ret = ioctl(sockfd, SIOCSIPMSFILTER, (unsigned long)&imsf);
          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_IGM */
