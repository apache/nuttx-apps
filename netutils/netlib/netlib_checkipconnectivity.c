/****************************************************************************
 * apps/netutils/netlib/netlib_checkipconnectivity.c
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
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <nuttx/debug.h>
#include <ifaddrs.h>

#include <arpa/inet.h>
#include <nuttx/net/ip.h>

#include "netutils/netlib.h"
#include "netutils/icmp_ping.h"
#include "netutils/icmpv6_ping.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define PING_IPADDR_DATALEN  56

#ifdef CONFIG_NETUTILS_PING

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ping_ipaddr_callback
 *
 * Description:
 *   ping ipaddr callback
 *
 * Parameters:
 *   result  The result of ping
 *
 ****************************************************************************/

static void ping_ipaddr_callback(FAR const struct ping_result_s *result)
{
  FAR int *count = result->info->priv;

  if (result->code == ICMP_I_FINISH)
    {
      *count = result->nreplies;
    }
}

#endif /* CONFIG_NETUTILS_PING */

#ifdef CONFIG_NETUTILS_PING6

/****************************************************************************
 * Name: ping6_ipaddr_callback
 *
 * Description:
 *   ping6 ipaddr callback
 *
 * Parameters:
 *   result  The result of ping6
 *
 ****************************************************************************/

static void ping6_ipaddr_callback(FAR const struct ping6_result_s *result)
{
  FAR int *count = result->info->priv;

  if (result->code == ICMPv6_I_FINISH)
    {
      *count = result->nreplies;
    }
}

#endif /* CONFIG_NETUTILS_PING6 */

/****************************************************************************
 * Name: ping_gateway
 *
 * Description:
 *   Ping the default gateway of each network interface (IPv4 and IPv6).
 *
 * Parameters:
 *   info   Pointer to the IPv4 ping info structure
 *   info6  Pointer to the IPv6 ping info structure
 *
 ****************************************************************************/

#ifdef CONFIG_NETDEV_IFINDEX
static void ping_gateway(FAR struct ping_info_s *info,
                         FAR struct ping6_info_s *info6)
{
  FAR int *replies = info->priv;
  FAR struct ifaddrs *ifap;
  FAR struct ifaddrs *ifa;

  if (getifaddrs(&ifap) < 0)
    {
      return;
    }

  for (ifa = ifap; ifa != NULL && *replies == 0; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL)
        {
          continue;
        }

      /* Skip interfaces that are not up and running */

      if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) !=
          (IFF_UP | IFF_RUNNING) || (ifa->ifa_flags & IFF_LOOPBACK) != 0)
        {
          continue;
        }

      if (ifa->ifa_addr->sa_family == AF_INET)
        {
          FAR struct sockaddr_in *sin;
          char destip[INET_ADDRSTRLEN];

          /* Skip interfaces with INADDR_ANY address and loopback flag */

          sin = (FAR struct sockaddr_in *)ifa->ifa_addr;
          if (sin->sin_addr.s_addr == INADDR_ANY)
            {
              continue;
            }

          /* No ARP means point-to-point or tunnel, treat as reachable */

          if ((ifa->ifa_flags & IFF_NOARP) != 0)
            {
              (*replies)++;
              break;
            }

          sin = (FAR struct sockaddr_in *)ifa->ifa_dstaddr;
          if (sin == NULL ||
              sin->sin_family != AF_INET ||
              sin->sin_addr.s_addr == INADDR_ANY)
            {
              continue;
            }

          inet_ntop(AF_INET, &sin->sin_addr, destip, sizeof(destip));
          ninfo("ping gateway %s on %s\n", destip, ifa->ifa_name);
          info->hostname = destip;
#ifdef CONFIG_NETUTILS_PING
          icmp_ping(info);
#else
          (*replies)++;
#endif
        }
      else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
          FAR struct sockaddr_in6 *sin6;
          char destip[INET6_ADDRSTRLEN];

          /* Skip interfaces with unspecified address and loopback flag */

          sin6 = (FAR struct sockaddr_in6 *)ifa->ifa_addr;
          if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
            {
              continue;
            }

          /* No ARP means point-to-point or tunnel, treat as reachable */

          if ((ifa->ifa_flags & IFF_NOARP) != 0)
            {
              (*replies)++;
              break;
            }

          sin6 = (FAR struct sockaddr_in6 *)ifa->ifa_dstaddr;
          if (sin6 == NULL ||
              sin6->sin6_family != AF_INET6 ||
              IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
            {
              continue;
            }

          inet_ntop(AF_INET6, &sin6->sin6_addr, destip, sizeof(destip));
          ninfo("ping6 gateway %s on %s\n", destip, ifa->ifa_name);
          info6->hostname = destip;
#ifdef CONFIG_NETUTILS_PING6
          icmp6_ping(info6);
#else
          (*replies)++;
#endif
        }
    }

  freeifaddrs(ifap);
}
#endif /* CONFIG_NETDEV_IFINDEX */

/****************************************************************************
 * Name: ping_route
 *
 * Description:
 *   Ping the router addresses from the IPv4 routing table.
 *
 * Parameters:
 *   info  Pointer to the ping info structure
 *
 ****************************************************************************/

#if defined(HAVE_ROUTE_PROCFS) && defined(CONFIG_NETUTILS_PING)
static void ping_route(FAR struct ping_info_s *info)
{
  FAR int *replies = info->priv;
  FAR FILE *stream;
  struct netlib_ipv4_route_s route;

  stream = netlib_open_ipv4route();
  if (stream == NULL)
    {
      return;
    }

  while (*replies == 0 && netlib_read_ipv4route(stream, &route) > 0)
    {
      char destip[INET_ADDRSTRLEN];

      if (route.router == INADDR_ANY)
        {
          continue;
        }

      inet_ntop(AF_INET, &route.router, destip, sizeof(destip));
      ninfo("ping route gateway %s\n", destip);
      info->hostname = destip;
      icmp_ping(info);
    }

  netlib_close_ipv4route(stream);
}
#endif /* HAVE_ROUTE_PROCFS && CONFIG_NETUTILS_PING */

/****************************************************************************
 * Name: ping6_route
 *
 * Description:
 *   Ping the router addresses from the IPv6 routing table.
 *
 * Parameters:
 *   info6  Pointer to the ping6 info structure
 *
 ****************************************************************************/

#if defined(HAVE_ROUTE_PROCFS) && defined(CONFIG_NETUTILS_PING6)
static void ping6_route(FAR struct ping6_info_s *info6)
{
  FAR int *replies = info6->priv;
  FAR FILE *stream;
  struct netlib_ipv6_route_s route;

  stream = netlib_open_ipv6route();
  if (stream == NULL)
    {
      return;
    }

  while (*replies == 0 && netlib_read_ipv6route(stream, &route) > 0)
    {
      char destip[INET6_ADDRSTRLEN];
      struct in6_addr router;

      memcpy(&router, route.router, sizeof(router));
      if (IN6_IS_ADDR_UNSPECIFIED(&router))
        {
          continue;
        }

      inet_ntop(AF_INET6, &router, destip, sizeof(destip));
      ninfo("ping6 route gateway %s\n", destip);
      info6->hostname = destip;
      icmp6_ping(info6);
    }

  netlib_close_ipv6route(stream);
}
#endif /* HAVE_ROUTE_PROCFS && CONFIG_NETUTILS_PING6 */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_check_ipconnectivity
 *
 * Description:
 *   Check network connectivity by pinging a remote IP address.
 *   If ip is NULL, ping the gateway of each network interface,
 *   and optionally the routers from the routing table. If ping
 *   is disabled, just check the status of the IP network card.
 *
 * Parameters:
 *   ip       The ipv4 address to check, or NULL to ping gateways
 *   timeout  The max timeout of each ping
 *   retry    The retry times of ping
 *
 * Return:
 *   nums of remote reply of ping; a negative or ZERO on failure
 *
 ****************************************************************************/

int netlib_check_ipconnectivity(FAR const char *ip, int timeout, int retry)
{
  int replies = 0;
#if defined(CONFIG_NETDEV_IFINDEX) || defined(CONFIG_NETUTILS_PING)
  struct ping_info_s info;
#endif
#if defined(CONFIG_NETDEV_IFINDEX) || defined(CONFIG_NETUTILS_PING6)
  struct ping6_info_s info6;
#endif

#ifdef CONFIG_NETUTILS_PING
  info.count    = retry;
  info.datalen  = PING_IPADDR_DATALEN;
  info.delay    = 0;
  info.timeout  = timeout;
  info.callback = ping_ipaddr_callback;
  info.hostname = ip;
#  ifdef CONFIG_NET_BINDTODEVICE
  info.devname  = NULL;
#  endif
#endif
#if defined(CONFIG_NETDEV_IFINDEX) || defined(CONFIG_NETUTILS_PING)
  info.priv     = &replies;
#endif

#ifdef CONFIG_NETUTILS_PING6
  info6.count    = retry;
  info6.datalen  = PING_IPADDR_DATALEN;
  info6.delay    = 0;
  info6.timeout  = timeout;
  info6.hostname = ip;
  info6.callback = ping6_ipaddr_callback;
#  ifdef CONFIG_NET_BINDTODEVICE
  info6.devname  = NULL;
#  endif
#endif
#if defined(CONFIG_NETDEV_IFINDEX) || defined(CONFIG_NETUTILS_PING6)
  info6.priv     = &replies;
#endif

  if (ip != NULL)
    {
      struct in6_addr addr6;

      ninfo("ping ipaddr test %s\n", ip);
      if (inet_pton(AF_INET6, ip, &addr6) == 1)
        {
#ifdef CONFIG_NETUTILS_PING6
          icmp6_ping(&info6);
          return replies;
#else
          /* Fallback to network card status check */
#endif
        }
      else
        {
#ifdef CONFIG_NETUTILS_PING
          icmp_ping(&info);
          return replies;
#else
          /* Fallback to network card status check */
#endif
        }
    }

#ifdef CONFIG_NETDEV_IFINDEX
  ping_gateway(&info, &info6);
#endif

#if defined(HAVE_ROUTE_PROCFS) && defined(CONFIG_NETUTILS_PING)
  if (replies == 0)
    {
      ping_route(&info);
    }
#endif

#if defined(HAVE_ROUTE_PROCFS) && defined(CONFIG_NETUTILS_PING6)
  if (replies == 0)
    {
      ping6_route(&info6);
    }
#endif

#if !defined(CONFIG_NETDEV_IFINDEX) && !defined(HAVE_ROUTE_PROCFS)
  replies = 1;
#endif

  return replies;
}
