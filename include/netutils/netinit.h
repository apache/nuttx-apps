/****************************************************************************
 * apps/include/netutils/netinit.h
 *
 *   Copyright (C) 2007-2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
