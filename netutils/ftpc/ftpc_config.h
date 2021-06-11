/****************************************************************************
 * apps/netutils/ftpc/ftpc_config.h
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

#ifndef __APPS_NETUTILS_FTPC_FTPC_CONFIG_H
#define __APPS_NETUTILS_FTPC_FTPC_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This is a mindless little wrapper around include/nuttx/config.h.  Every
 * file in the ftpc directory includes this file at the very beginning of
 * of the file (instead of include/nuttx/config.h).  The only purpose of
 * this file is to muck with some of the settings to support some debug
 * features.
 *
 * The FPT client uses common networking debug macros (ndbg and ninfo).
 * This can be overwhelming if there is a lot of networking debug output
 * as well.  But by defining CONFIG_DEBUG_FTPC, this file will force
 * networking debug ON only for the files within this directory.
 */

#if !defined(CONFIG_DEBUG_NET) && defined(CONFIG_DEBUG_FTPC)
#  undef  CONFIG_DEBUG_NET
#  define CONFIG_DEBUG_NET 1
#  undef  CONFIG_DEBUG_NET_ERROR
#  define CONFIG_DEBUG_NET_ERROR 1
#  undef  CONFIG_DEBUG_NET_WARN
#  define CONFIG_DEBUG_NET_WARN 1
#  undef  CONFIG_DEBUG_NET_INFO
#  define CONFIG_DEBUG_NET_INFO 1
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

#endif /* __APPS_NETUTILS_FTPC_FTPC_CONFIG_H */
