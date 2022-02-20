/****************************************************************************
 * apps/netutils/netlib/netlib_setdripv4addr.c
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
#ifdef CONFIG_NET_IPv4

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <net/if.h>

#ifdef CONFIG_NET_ROUTE
#  include <net/route.h>
#endif

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_set_dripv4addr
 *
 * Description:
 *   Set the default router IPv4 address
 *
 * Parameters:
 *   ifname   The name of the interface to use
 *   ipaddr   The address to set
 *
 * Return:
 *   0 on success; -1 on failure
 *
 ****************************************************************************/

int netlib_set_dripv4addr(FAR const char *ifname,
                          FAR const struct in_addr *addr)
{
  int ret = ERROR;

#ifdef CONFIG_NET_ROUTE
  struct sockaddr_in target;
  struct sockaddr_in netmask;
  struct sockaddr_in router;

  memset(&target, 0, sizeof(target));
  target.sin_family  = AF_INET;

  memset(&netmask, 0, sizeof(netmask));
  netmask.sin_family  = AF_INET;

  router.sin_addr    = *addr;
  router.sin_family  = AF_INET;
#endif

  if (ifname && addr)
    {
      int sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
      if (sockfd >= 0)
        {
          FAR struct sockaddr_in *inaddr;
          struct ifreq req;

          /* Add the device name to the request */

          strlcpy(req.ifr_name, ifname, IFNAMSIZ);

          /* Add the INET address to the request */

          inaddr             = (FAR struct sockaddr_in *)&req.ifr_addr;
          inaddr->sin_family = AF_INET;
          inaddr->sin_port   = 0;
          memcpy(&inaddr->sin_addr, addr, sizeof(struct in_addr));

          ret = ioctl(sockfd, SIOCSIFDSTADDR, (unsigned long)&req);

#ifdef CONFIG_NET_ROUTE
          if (OK == ret)
            {
              /* Delete the default route first */

              /* This call fails if no default route exists, but it's OK */

              delroute(sockfd,
                       (FAR struct sockaddr_storage *)&target,
                       (FAR struct sockaddr_storage *)&netmask);

              /* Then add the new default route */

              ret = addroute(sockfd,
                             (FAR struct sockaddr_storage *)&target,
                             (FAR struct sockaddr_storage *)&netmask,
                             (FAR struct sockaddr_storage *)&router);
            }
#endif

          close(sockfd);
        }
    }

  return ret;
}

#endif /* CONFIG_NET_IPv4 */
