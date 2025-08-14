/****************************************************************************
 * apps/system/nxinit/init.h
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

#ifndef __APPS_SYSTEM_NXINIT_INIT_H
#define __APPS_SYSTEM_NXINIT_INIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
#define init_debug(...) syslog(LOG_DEBUG, ##__VA_ARGS__)
#define init_dump_args(argc, argv) \
          { \
            int _i; \
            for (_i = 0; _i < (argc); _i++) \
              { \
                init_debug("  argv[%d] '%s'", _i, (argv)[_i]); \
              } \
          }
#else
#define init_debug(...)
#define init_dump_args(...)
#endif

#ifdef CONFIG_SYSTEM_NXINIT_INFO
#define init_info(...) syslog(LOG_INFO, ##__VA_ARGS__)
#else
#define init_info(...)
#endif

#ifdef CONFIG_SYSTEM_NXINIT_WARN
#define init_warn(...) syslog(LOG_WARNING, ##__VA_ARGS__)
#else
#define init_warn(...)
#endif

#ifdef CONFIG_SYSTEM_NXINIT_ERR
#define init_err(f, ...) syslog(LOG_ERR, "Error " f, ##__VA_ARGS__)
#else
#define init_err(...)
#endif

#define init_log(p, ...) \
        do \
          { \
            if ((p) <= LOG_ERR) \
              { \
                init_err(__VA_ARGS__); \
              } \
            else if ((p) <= LOG_WARNING) \
              { \
                init_warn(__VA_ARGS__); \
              } \
            else if ((p) <= LOG_INFO) \
              { \
                init_info(__VA_ARGS__); \
              } \
            else \
              { \
                init_debug(__VA_ARGS__); \
              } \
          } \
        while (0)

#endif /* __APPS_SYSTEM_NXINIT_INIT_H */
