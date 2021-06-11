/****************************************************************************
 * apps/netutils/netlib/netlib_ipv6router.c
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

#include <stdio.h>
#include <errno.h>

#include <arpa/inet.h>
#include <nuttx/net/ip.h>

#include "netutils/netlib.h"

#if defined(CONFIG_NET_IPv6) && defined(HAVE_ROUTE_PROCFS)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ipv6router
 *
 * Description:
 *   Given a destination address that is no on a local network, query the
 *   IPv6 routing table, and return the IPv6 address of the routing that
 *   will provide access to the correct sub-net.
 *
 * Input Parameters:
 *   destipaddr - The destination address to use in the look-up (in network
 *                byte order).
 *   router     - The location to return that the IP address of the router
 *                (in network byte order)
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int netlib_ipv6router(FAR const struct in6_addr *destipaddr,
                      FAR struct in6_addr *router)
{
  struct netlib_ipv6_route_s route;
  FILE *stream;
  uint16_t hdest[8];
  ssize_t nbytes;
  int ret = -ENOENT;

  /* Open the routing table file */

  stream = netlib_open_ipv6route();
  if (stream == NULL)
    {
      return -errno;
    }

  /* Convert the destination IP address to host byte order for the
   * comparison.
   */

  hdest[0] = ntohs(destipaddr->s6_addr16[0]);
  hdest[1] = ntohs(destipaddr->s6_addr16[1]);
  hdest[2] = ntohs(destipaddr->s6_addr16[2]);
  hdest[3] = ntohs(destipaddr->s6_addr16[3]);
  hdest[4] = ntohs(destipaddr->s6_addr16[4]);
  hdest[5] = ntohs(destipaddr->s6_addr16[5]);
  hdest[6] = ntohs(destipaddr->s6_addr16[6]);
  hdest[7] = ntohs(destipaddr->s6_addr16[7]);

  /* Find the routing table entry that provides the router for this
   * sub-net.
   */

  for (; ; )
    {
      /* Read the route (in host byte order) */

      nbytes = netlib_read_ipv6route(stream, &route);
      if (nbytes == 0)
        {
          /* End of file with no match */

          break;
        }
      else if (nbytes < 0)
        {
          /* Read error */

          ret = (int)nbytes;
          break;
        }

      /* Compare the prefix and the destination address under the mask */

      if (net_ipv6addr_maskcmp(route.prefix, hdest, route.netmask))
        {
          /* Found it! Return the router address in network byte order */

          router->s6_addr16[0] = htons(route.router[0]);
          router->s6_addr16[1] = htons(route.router[1]);
          router->s6_addr16[2] = htons(route.router[2]);
          router->s6_addr16[3] = htons(route.router[3]);
          router->s6_addr16[4] = htons(route.router[4]);
          router->s6_addr16[5] = htons(route.router[5]);
          router->s6_addr16[6] = htons(route.router[6]);
          router->s6_addr16[7] = htons(route.router[7]);
          ret = OK;
          break;
        }
    }

  netlib_close_ipv6route(stream);
  return ret;
}

#endif /* CONFIG_NET_IPv6 && HAVE_ROUTE_PROCFS */
