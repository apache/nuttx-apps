/****************************************************************************
 * apps/netutils/netlib/netlib_ipv4router.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#if defined(CONFIG_NET_IPv4) && defined(HAVE_ROUTE_PROCFS)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ipv4router
 *
 * Description:
 *   Given a destination address that is not on a local network, query the
 *   IPv4 routing table, and return the IPv4 address of the routing that
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

int netlib_ipv4router(FAR const struct in_addr *destipaddr,
                      FAR struct in_addr *router)
{
  struct netlib_ipv4_route_s route;
  FILE *stream;
  in_addr_t hdest;
  ssize_t nbytes;
  int ret = -ENOENT;

  /* Open the routing table file */

  stream = netlib_open_ipv4route();
  if (stream == NULL)
    {
      return -errno;
    }

  /* Convert the destination IP address to host byte order for the
   * comparison.
   */

  hdest = ntohl(destipaddr->s_addr);

  /* Find the routing table entry that provides the router for this
   * sub-net.
   */

  for (; ; )
    {
      /* Read the route (in host byte order) */

      nbytes = netlib_read_ipv4route(stream, &route);
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

      if (net_ipv4addr_maskcmp(route.prefix, hdest, route.netmask))
        {
          /* Found it! Return the router address in network byte order */

          router->s_addr = htonl(route.router);
          ret = OK;
          break;
        }
    }

  netlib_close_ipv4route(stream);
  return ret;
}

#endif /* CONFIG_NET_IPv4 && HAVE_ROUTE_PROCFS */
