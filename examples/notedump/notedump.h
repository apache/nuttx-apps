/****************************************************************************
 * apps/examples/notedump/notedump.h
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

#ifndef __APPS_EXAMPLES_NOTEDUMP_H
#define __APPS_EXAMPLES_NOTEDUMP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifdef EXAMPLES_UDP_HOST
#else
#  include <debug.h>
#endif

#include <arpa/inet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_UDP_IPv6
#  define AF_INETX AF_INET6
#  define PF_INETX PF_INET6
#else
#  define AF_INETX AF_INET
#  define PF_INETX PF_INET
#endif

#ifndef CONFIG_EXAMPLES_NOTEDUMP_SERVER_PORTNO
#  define CONFIG_EXAMPLES_NOTEDUMP_SERVER_PORTNO 6666
#endif

#ifndef CONFIG_EXAMPLES_NOTEDUMP_METRIC_FREQ
#  define CONFIG_EXAMPLES_NOTEDUMP_METRIC_FREQ 10
#endif

#endif /* __APPS_EXAMPLES_NOTEDUMP_H */
