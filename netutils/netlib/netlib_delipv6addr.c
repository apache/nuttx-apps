/****************************************************************************
 * apps/netutils/netlib/netlib_delipv6addr.c
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
#ifdef CONFIG_NETDEV_MULTIPLE_IPv6

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_del_ipv6addr
 *
 * Description:
 *   Remove an IPv6 address from the network driver
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   ipaddr   The address to delete
 *   preflen  The prefix length of the address
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_del_ipv6addr(FAR const char *ifname,
                        FAR const struct in6_addr *addr, uint8_t preflen)
{
  int ret = ERROR;

  if (ifname && addr)
    {
      int sockfd = socket(AF_INET6, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct ifreq req;
          struct in6_ifreq ifr6;

          /* Add the device name to the request */

          strlcpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Get interface index */

          ret = ioctl(sockfd, SIOCGIFINDEX,
                     ((unsigned long)(uintptr_t)&req));
          if (ret == OK)
            {
              /* Delete address from the interface. */

              ifr6.ifr6_ifindex = req.ifr_ifindex;
              ifr6.ifr6_prefixlen = preflen;
              memcpy(&ifr6.ifr6_addr, addr, sizeof(struct in6_addr));

              ret = ioctl(sockfd, SIOCDIFADDR,
                          ((unsigned long)(uintptr_t)&ifr6));
            }

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NETDEV_MULTIPLE_IPv6 */
