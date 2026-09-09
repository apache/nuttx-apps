/****************************************************************************
 * apps/system/xrcedds/include/uxr/client/config.h
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
 ****************************************************************************/

#ifndef __APPS_SYSTEM_XRCEDDS_INCLUDE_UXR_CLIENT_CONFIG_H
#define __APPS_SYSTEM_XRCEDDS_INCLUDE_UXR_CLIENT_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#define UXR_CLIENT_VERSION_MAJOR 3
#define UXR_CLIENT_VERSION_MINOR 0
#define UXR_CLIENT_VERSION_MICRO 1
#define UXR_CLIENT_VERSION_STR "3.0.1"

#define UCLIENT_PROFILE_UDP
#define UCLIENT_PLATFORM_POSIX
#define UCLIENT_TWEAK_XRCE_WRITE_LIMIT

#ifdef CONFIG_SYSTEM_XRCEDDS_STREAM_FRAMING
#  define UCLIENT_PROFILE_STREAM_FRAMING
#endif

#define UXR_CONFIG_MAX_OUTPUT_BEST_EFFORT_STREAMS \
  CONFIG_SYSTEM_XRCEDDS_MAX_OUTPUT_BEST_EFFORT_STREAMS
#define UXR_CONFIG_MAX_OUTPUT_RELIABLE_STREAMS \
  CONFIG_SYSTEM_XRCEDDS_MAX_OUTPUT_RELIABLE_STREAMS
#define UXR_CONFIG_MAX_INPUT_BEST_EFFORT_STREAMS \
  CONFIG_SYSTEM_XRCEDDS_MAX_INPUT_BEST_EFFORT_STREAMS
#define UXR_CONFIG_MAX_INPUT_RELIABLE_STREAMS \
  CONFIG_SYSTEM_XRCEDDS_MAX_INPUT_RELIABLE_STREAMS

#define UXR_CONFIG_MAX_SESSION_CONNECTION_ATTEMPTS 10
#define UXR_CONFIG_MIN_SESSION_CONNECTION_INTERVAL 1000
#define UXR_CONFIG_MIN_HEARTBEAT_TIME_INTERVAL 100

#ifdef UCLIENT_PROFILE_UDP
#  define UXR_CONFIG_UDP_TRANSPORT_MTU CONFIG_SYSTEM_XRCEDDS_MTU
#endif

#endif /* __APPS_SYSTEM_XRCEDDS_INCLUDE_UXR_CLIENT_CONFIG_H */
