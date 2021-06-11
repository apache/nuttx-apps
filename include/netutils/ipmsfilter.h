/****************************************************************************
 * apps/include/netutils/ipmsfilter.h
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

#ifndef __APPS_INCLUDE_NETUTILS_IPMSFILTER_H
#define __APPS_INCLUDE_NETUTILS_IPMSFILTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <netinet/in.h>

#ifdef CONFIG_NET_IGMP

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
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
               uint32_t fmode);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_NET_IGMP */
#endif /* __APPS_INCLUDE_NETUTILS_IPMSFILTER_H */
