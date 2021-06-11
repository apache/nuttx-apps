/****************************************************************************
 * apps/netutils/netlib/netlib_ipv4adaptor.c
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

#ifdef CONFIG_NET_IPv4

/****************************************************************************
 * Priver Functions
 ****************************************************************************/

/****************************************************************************
 * Name: _netlib_ipv4adaptor
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
 *   1. Call netlib_ipv4adaptor() to find the address of the network
 *      adaptor for the destination address.
 *   2. If this fails, then look up the router address in the routing table
 *      that can forward to the destination address, then
 *   3. Call netlib_ipv4adaptor() to find the address of the network
 *      adaptor for that router address.
 *
 * Input Parameters:
 *   destipaddr - The destination IPv4 address
 *   srcipaddr  - The location to return that adaptor address that serves
 *                the sub-net that includes the destination address.
 *
 * Returned Value:
 *   Zero (OK) is returned on success with srcipaddr valid.  A negated
 *   errno value is returned on any failure and in this case the srcipaddr
 *   is not valid.
 *
 ****************************************************************************/

static int _netlib_ipv4adaptor(in_addr_t destipaddr,
                               FAR in_addr_t *srcipaddr)
{
  FAR struct ifreq *ifr;
  struct ifconf ifc;
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

  ifc.ifc_req = NULL;
  ifc.ifc_len = 0;

  sd = socket(AF_INET, NETLIB_SOCK_TYPE, 0);
  if (sd < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: socket() failed: %d\n", ret);
      return ret;
    }

  ret = ioctl(sd, SIOCGIFCONF, (unsigned long)((uintptr_t)&ifc));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: ioctl(SIOCGIFCONF) failed: %d\n", ret);
      goto errout_with_sd;
    }

  /* Allocate the buffer to hold the interface descriptions */

  allocsize = ifc.ifc_len;
  ifr       = (FAR struct ifreq *)malloc(allocsize);
  if (ifr == NULL)
    {
      fprintf(stderr, "ERROR: Failed to allocate IFC buffer\n");
      ret = -ENOMEM;
      goto errout_with_sd;
    }

  /* Get the interface descriptions */

  ifc.ifc_req = ifr;

  ret = ioctl(sd, SIOCGIFCONF, (unsigned long)((uintptr_t)&ifc));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: ioctl(SIOCGIFCONF) failed: %d\n", ret);
      goto errout_with_ifr;
    }

  /* Find an interface that supports the subnet needed by the provided
   * IPv4 address. Be careful!  The size of the returned buffer could
   * be different if the network device configuration changed in between
   * the two calls!
   */

  buflen = ifc.ifc_len;
  if (buflen > allocsize)
    {
      /* Something has changed.  Limit to the original size (OR do the
       * allocation and IOCTL again).
       */

      buflen = allocsize;
    }

  nintf = buflen / sizeof(struct ifreq);
  ret   = -EHOSTUNREACH;

  for (i = 0; i < nintf; i++)
    {
      FAR struct ifreq *req = &ifr[i];
      FAR struct sockaddr_in *addr =
        (FAR struct sockaddr_in *)&req->ifr_addr;
      struct ifreq maskreq;
      FAR struct sockaddr_in *netmask;

      /* Get the network mask */

      strncpy(maskreq.ifr_name, ifr->ifr_name, IFNAMSIZ);

      ret = ioctl(sd, SIOCGIFNETMASK, (unsigned long)((uintptr_t)&maskreq));
      if (ret < 0)
        {
          ret = -errno;
          fprintf(stderr, "ERROR: ioctl(SIOCGIFNETMASK) failed: %d\n",
                  ret);
          goto errout_with_ifr;
        }

      netmask = (FAR struct sockaddr_in *)&maskreq.ifr_addr;

      /* Does this device provide the sub-net? */

      if (net_ipv4addr_maskcmp(addr->sin_addr.s_addr, destipaddr,
                               netmask->sin_addr.s_addr))
        {
          /* Yes.. return the IP address assigned to this adaptor */

          *srcipaddr = addr->sin_addr.s_addr;
          ret = OK;
          break;
        }
    }

errout_with_ifr:
  free(ifr);

errout_with_sd:
  close(sd);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ipv4adaptor
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
 *   destipaddr - The destination IPv4 address
 *   srcipaddr  - The location to return that adaptor address that serves
 *                the sub-net that includes the destination address.
 *
 * Returned Value:
 *   Zero (OK) is returned on success with srcipaddr valid.  A negated
 *   errno value is returned on any failure and in this case the srcipaddr
 *   is not valid.
 *
 ****************************************************************************/

int netlib_ipv4adaptor(in_addr_t destipaddr, FAR in_addr_t *srcipaddr)
{
  int ret;

  DEBUGASSERT(srcipaddr != NULL);
  ret = _netlib_ipv4adaptor(destipaddr, srcipaddr);

#ifdef HAVE_ROUTE_PROCFS
  if (ret < 0)
    {
      struct in_addr router;
      struct in_addr destinaddr;

      /* If the first adaptor look-up on the the destination IP address
       * failed, then the IP address cannot be sent on any of the currently
       * up network devices configured with an IPv4 address.
       *
       * But perhaps the destination address is on a sub-net that is
       * accessible on a router that can be reached through a local network
       * adaptor?
       */

      destinaddr.s_addr = destipaddr;
      ret = netlib_ipv4router(&destinaddr, &router);
      if (ret >= 0)
        {
          /* Yes... try again using the router address as the destination
           * address.
           */

          ret = _netlib_ipv4adaptor(router.s_addr, srcipaddr);
        }
    }
#endif

  return ret;
}

#endif /* CONFIG_NET_IPv4 */
