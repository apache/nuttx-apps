/****************************************************************************
 * apps/lte/alt1250/alt1250_netdev.c
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

#include <string.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netdev.h>

#include "alt1250_netdev.h"

#define ALT1250_SUBNET_MASK 0xFFFFFF00 /* 255.255.255.0 this is dummy */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: alt1250_netdev_register
 ****************************************************************************/

void alt1250_netdev_register(FAR struct alt1250_s *dev)
{
  netdev_register(&dev->net_dev, NET_LL_ETHERNET);
}

/****************************************************************************
 * name: alt1250_netdev_unregister
 ****************************************************************************/

void alt1250_netdev_unregister(FAR struct alt1250_s *dev)
{
  netdev_unregister(&dev->net_dev);
}

/****************************************************************************
 * name: alt1250_netdev_ifdown
 ****************************************************************************/

void alt1250_netdev_ifdown(FAR struct alt1250_s *dev)
{
  dev->net_dev.d_flags = ~IFF_UP;
#ifdef CONFIG_NET_IPv4
  memset(&dev->net_dev.d_ipaddr, 0, sizeof(dev->net_dev.d_ipaddr));
  memset(&dev->net_dev.d_draddr, 0, sizeof(dev->net_dev.d_draddr));
  memset(&dev->net_dev.d_netmask, 0, sizeof(dev->net_dev.d_netmask));
#endif
#ifdef CONFIG_NET_IPv6
  memset(&dev->net_dev.d_ipv6addr, 0, sizeof(dev->net_dev.d_ipv6addr));
#endif
}

/****************************************************************************
 * name: alt1250_netdev_ifup
 ****************************************************************************/

void alt1250_netdev_ifup(FAR struct alt1250_s *dev, FAR lte_pdn_t *pdn)
{
  int i;

  dev->net_dev.d_flags = IFF_UP;

  for (i = 0; i < pdn->ipaddr_num; i++)
    {
#ifdef CONFIG_NET_IPv4
      if (LTE_IPTYPE_V4 == pdn->address[i].ip_type)
        {
          inet_pton(AF_INET,
                    (FAR const char *)pdn->address[i].address,
                    (FAR void *)&dev->net_dev.d_ipaddr);
          inet_pton(AF_INET,
                    (FAR const char *)pdn->address[i].address,
                    (FAR void *)&dev->net_dev.d_draddr);

          /* The following parameters are dummy values because
           * they cannot be obtained from alt1250.
           */

          dev->net_dev.d_draddr = htonl((ntohl(dev->net_dev.d_draddr) &
                                         ALT1250_SUBNET_MASK) | 1);
          dev->net_dev.d_netmask = htonl(ALT1250_SUBNET_MASK);
        }
#endif

#ifdef CONFIG_NET_IPv6
      if (LTE_IPTYPE_V6 == pdn->address[i].ip_type)
        {
          inet_pton(AF_INET6,
                    (FAR const char *)pdn->address[i].address,
                    (FAR void *)&dev->net_dev.d_ipv6addr);
        }
#endif
    }
}
