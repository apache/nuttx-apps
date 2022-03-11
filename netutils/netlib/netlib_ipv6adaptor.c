/****************************************************************************
 * apps/netutils/netlib/netlib_ipv6adaptor.c
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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <net/if.h>
#include <nuttx/net/ip.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NET_IPv6

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: _netlib_ipv6adaptor
 *
 * Description:
 *   Given the destination address, destipaddr, return the IP address
 *   assigned to the network adaptor that connects the sub-net that
 *   includes destipaddr.
 *
 *   NOTE:  This does not account for any routing information that may
 *   appear in the routing table.  A complete solution could involve three
 *   steps:
 *
 *   1. Call netlib_ipv6adaptor() to find the address of the network
 *      adaptor for the destination address.
 *   2. If this fails, then look up the router address in the routing table
 *      that can forward to the destination address, then
 *   3. Call netlib_ipv6adaptor() to find the address of the network
 *      adaptor for that router address.
 *
 * Input Parameters:
 *   destipaddr - The destination IPv6 address
 *   srcipaddr  - The location to return that adaptor address that serves
 *                the sub-net that includes the destination address.
 *
 * Returned Value:
 *   Zero (OK) is returned on success with srcipaddr valid.  A negated
 *   errno value is returned on any failure and in this case the srcipaddr
 *   is not valid.
 *
 ****************************************************************************/

static int _netlib_ipv6adaptor(FAR const struct in6_addr *destipaddr,
                               FAR struct in6_addr *srcipaddr)
{
  FAR struct lifreq *lifr;
  struct lifconf lifc;
  size_t allocsize;
  size_t buflen;
  int nintf;
  int ret;
  int sd;
  int i;

  /* REVISIT:  Should check the routing table to see if there is a route
   * for the destipaddr before finding the device that provides the sub-net.
   */

  /* Find the size of the buffer we need to hold the interface descriptions */

  lifc.lifc_req = NULL;
  lifc.lifc_len = 0;

  sd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sd < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: socket() failed: %d\n", ret);
      return ret;
    }

  ret = ioctl(sd, SIOCGLIFCONF, (unsigned long)((uintptr_t)&lifc));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: ioctl(SIOCGLIFCONF) failed: %d\n", ret);
      goto errout_with_sd;
    }

  /* Allocate the buffer to hold the interface descriptions */

  allocsize = lifc.lifc_len;
  lifr      = (FAR struct lifreq *)malloc(allocsize);
  if (lifr == NULL)
    {
      fprintf(stderr, "ERROR: Failed to allocate LIFC buffer\n");
      ret = -ENOMEM;
      goto errout_with_sd;
    }

  /* Get the interface descriptions */

  lifc.lifc_req = lifr;

  ret = ioctl(sd, SIOCGLIFCONF, (unsigned long)((uintptr_t)&lifc));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: ioctl(SIOCGLIFCONF) failed: %d\n", ret);
      goto errout_with_ifr;
    }

  /* Find an interface that supports the subnet needed by the provided
   * IPv6 address. Be careful!  The size of the returned buffer could
   * be different if the network device configuration changed in between
   * the two calls!
   */

  buflen = lifc.lifc_len;
  if (buflen > allocsize)
    {
      /* Something has changed.  Limit to the original size (OR do the
       * allocation and IOCTL again).
       */

      buflen = allocsize;
    }

  nintf = buflen / sizeof(struct lifreq);
  ret   = -EHOSTUNREACH;

  for (i = 0; i < nintf; i++)
    {
      FAR struct lifreq *req = &lifr[i];
      FAR struct sockaddr_in6 *addr =
        (FAR struct sockaddr_in6 *)&req->lifr_addr;
      struct lifreq maskreq;
      FAR struct sockaddr_in6 *netmask;
      int status;

      /* Get the network mask */

      strlcpy(maskreq.lifr_name, lifr->lifr_name, IFNAMSIZ);

      status = ioctl(sd, SIOCGLIFNETMASK,
                     (unsigned long)((uintptr_t)&maskreq));
      if (status < 0)
        {
          ret = -errno;
          fprintf(stderr, "ERROR: ioctl(SIOCGLIFNETMASK) failed: %d\n",
                  ret);
          goto errout_with_ifr;
        }

      netmask = (FAR struct sockaddr_in6 *)&maskreq.lifr_addr;

      /* Does this device provide the sub-net? */

      if (net_ipv6addr_maskcmp(addr->sin6_addr.s6_addr16,
                               destipaddr->s6_addr16,
                               netmask->sin6_addr.s6_addr16))
        {
          /* Yes.. return the IP address assigned to this adaptor */

          net_ipv6addr_copy(srcipaddr->s6_addr16,
                            addr->sin6_addr.s6_addr16);
          ret = OK;
          break;
        }
    }

errout_with_ifr:
  free(lifr);

errout_with_sd:
  close(sd);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ipv6adaptor
 *
 * Description:
 *   Given the destination address, destipaddr, return the IP address
 *   assigned to the network adaptor that connects the sub-net that
 *   includes destipaddr.
 *
 *   If routing table support is enabled, then this logic will account for
 *   the case where the destination address is not locally accessible.  In
 *   this case, it will return the IP address of the network adaptor that
 *   provides the correct router to handle that destination address.
 *
 * Input Parameters:
 *   destipaddr - The destination IPv6 address
 *   srcipaddr  - The location to return that adaptor address that serves
 *                the sub-net that includes the destination address.
 *
 * Returned Value:
 *   Zero (OK) is returned on success with srcipaddr valid.  A negated
 *   errno value is returned on any failure and in this case the srcipaddr
 *   is not valid.
 *
 ****************************************************************************/

int netlib_ipv6adaptor(FAR const struct in6_addr *destipaddr,
                       FAR struct in6_addr *srcipaddr)
{
  int ret;

  DEBUGASSERT(destipaddr != NULL && srcipaddr != NULL);

  ret = _netlib_ipv6adaptor(destipaddr, srcipaddr);

#ifdef HAVE_ROUTE_PROCFS
  if (ret < 0)
    {
      struct in6_addr router;

      /* If the first adaptor look-up on the the destination IP address
       * failed,  then the IP address cannot be sent on any of the
       * currently up network devices configured with an IPv6 address.
       *
       * But perhaps the destination address is on a sub-net that is
       * accessible on a router that can be reached through a local
       * network adaptor?
       */

      ret = netlib_ipv6router(destipaddr, &router);
      if (ret >= 0)
        {
          /* Yes... try again using the router address as the destination
           * address.
           */

          ret = _netlib_ipv6adaptor(&router, srcipaddr);
        }
    }
#endif

  return ret;
}

#endif /* CONFIG_NET_IPv6 */
