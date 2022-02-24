/****************************************************************************
 * apps/netutils/netlib/netlib_getipv6addr.c
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
#ifdef CONFIG_NET_IPv6

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <netinet/in.h>
#include <net/if.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_get_ipv6addr
 *
 * Description:
 *   Get the network driver IPv6 address
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   ipaddr   The location to return the IP address
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_get_ipv6addr(FAR const char *ifname, FAR struct in6_addr *addr)
{
  int ret = ERROR;

  if (ifname && addr)
    {
      int sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          struct lifreq req;

          strlcpy(req.lifr_name, ifname, IFNAMSIZ);
          ret = ioctl(sockfd, SIOCGLIFADDR, (unsigned long)&req);
          if (!ret)
            {
              FAR struct sockaddr_in6 *req_addr;

              req_addr = (FAR struct sockaddr_in6 *)&req.lifr_addr;
              memcpy(addr, &req_addr->sin6_addr, sizeof(struct in6_addr));
            }

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_IPv6 */
