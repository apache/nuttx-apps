/****************************************************************************
 * examples/netloop/netloop.h
 *
 *   Copyright (C) 2015, 2020 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_EXAMPLES_NETLOOP_NETLOOP_H
#define __APPS_EXAMPLES_NETLOOP_NETLOOP_H

/****************************************************************************
 * Compilation Switches
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Here are all of the configuration settings that must be met to have TCP/IP
 * poll/select support.  This kind of looks like overkill.
 *
 * CONFIG_NET                  - Network support must be enabled
 * CONFIG_NET_TCP              - Only support on TCP (because read-ahead
 *                               buffering s not yet support for UDP)
 */


#ifndef CONFIG_NET
#  error Network socket support not enabled
#endif

#if !defined(CONFIG_NET_TCP) || !defined(CONFIG_NET_TCPBACKLOG) || \
    !defined(CONFIG_NET_TCP_WRITE_BUFFERS)
#  error TCP not configured correctly
#endif

#if !defined(CONFIG_NET_IPv4)
#  error This test only works with IPv4
#endif

#define LISTENER_DELAY    3      /* 3 seconds */
#define LISTENER_PORT     5471
#define LO_ADDRESS        0x7f000001

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void *lo_listener(pthread_addr_t pvarg);

#endif /* __APPS_EXAMPLES_NETLOOP_NETLOOP_H */
