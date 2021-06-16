/****************************************************************************
 * apps/examples/netloop/netloop.h
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
