/****************************************************************************
 * apps/examples/poll/poll_internal.h
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

#ifndef __APPS_EXAMPLES_POLL_POLL_INTERNAL_H
#define __APPS_EXAMPLES_POLL_POLL_INTERNAL_H

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

#if defined(CONFIG_NET) && defined(CONFIG_NET_TCP)
#  define HAVE_NETPOLL 1
#else
#  undef HAVE_NETPOLL
#endif

#define FIFO_PATH1 "/dev/fifo0"
#define FIFO_PATH2 "/dev/fifo1"

#define POLL_LISTENER_DELAY   2000   /* 2 seconds */
#define SELECT_LISTENER_DELAY 4      /* 4 seconds */
#define NET_LISTENER_DELAY    3      /* 3 seconds */
#define WRITER_DELAY          6      /* 6 seconds */

#define LISTENER_PORT         5471

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

extern void *poll_listener(pthread_addr_t pvarg);
extern void *select_listener(pthread_addr_t pvarg);

#ifdef HAVE_NETPOLL
extern void *net_listener(pthread_addr_t pvarg);
extern void *net_reader(pthread_addr_t pvarg);
#endif
#endif /* __APPS_EXAMPLES_POLL_POLL_INTERNAL_H */
