/****************************************************************************
 * apps/include/netutils/netinit.h
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

#ifndef __APPS_INCLUDE_NETUTILS_NETINIT_H
#define __APPS_INCLUDE_NETUTILS_NETINIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_NETUTILS_NETINIT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* Networking support.  Make sure that all non-boolean configuration
 * settings have some value.
 */

#ifndef CONFIG_NETINIT_IPADDR
#  define CONFIG_NETINIT_IPADDR    0x0a000002
#endif

#ifndef CONFIG_NETINIT_DRIPADDR
#  define CONFIG_NETINIT_DRIPADDR  0x0a000001
#endif

#ifndef CONFIG_NETINIT_NETMASK
#  define CONFIG_NETINIT_NETMASK   0xffffff00
#endif

#ifndef CONFIG_NETINIT_DNSIPADDR
#  define CONFIG_NETINIT_DNSIPADDR CONFIG_NETINIT_DRIPADDR
#endif

#ifndef CONFIG_NETINIT_MACADDR
#  define CONFIG_NETINIT_MACADDR   0x00e0deadbeef
#endif

#if !defined(CONFIG_NETINIT_THREAD) || !defined(CONFIG_ARCH_PHY_INTERRUPT) || \
    !defined(CONFIG_NETDEV_PHY_IOCTL) || !defined(CONFIG_NET_UDP)
#  undef CONFIG_NETINIT_MONITOR
#endif

#ifndef CONFIG_NETINIT_RETRYMSEC
#  define CONFIG_NETINIT_RETRYMSEC 2000
#endif

#ifndef CONFIG_NETINIT_SIGNO
#  define CONFIG_NETINIT_SIGNO 18
#endif

#ifndef CONFIG_NETINIT_THREAD_STACKSIZE
#  define CONFIG_NETINIT_THREAD_STACKSIZE 1568
#endif

#ifndef CONFIG_NETINIT_THREAD_PRIORITY
#  define CONFIG_NETINIT_THREAD_PRIORITY 100
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: netinit_bringup
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

int netinit_bringup(void);

/****************************************************************************
 * Name: netinit_associate
 ****************************************************************************/

#ifdef CONFIG_WIRELESS_WAPI
int netinit_associate(FAR const char *ifname);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /* CONFIG_NETUTILS_NETINIT */
#endif /* __APPS_INCLUDE_NETUTILS_NETINIT_H */
