/****************************************************************************
 * apps/system/monkey/monkey_log.h
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

#ifndef __APPS_SYSTEM_MONKEY_MONKEY_LOG_H
#define __APPS_SYSTEM_MONKEY_MONKEY_LOG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MONKEY_LOG_INFO(format, ...)          \
  monkey_log_printf(MONKEY_LOG_LEVEL_INFO,    \
  __func__,                                   \
  format,                                     \
  ##__VA_ARGS__)
#define MONKEY_LOG_NOTICE(format, ...)        \
  monkey_log_printf(MONKEY_LOG_LEVEL_NOTICE,  \
  __func__,                                   \
  format,                                     \
  ##__VA_ARGS__)
#define MONKEY_LOG_WARN(format, ...)          \
  monkey_log_printf(MONKEY_LOG_LEVEL_WARN,    \
  __func__,                                   \
  format,                                     \
  ##__VA_ARGS__)
#define MONKEY_LOG_ERROR(format, ...)         \
  monkey_log_printf(MONKEY_LOG_LEVEL_ERROR,   \
  __func__,                                   \
  format,                                     \
  ##__VA_ARGS__)

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum monkey_log_level_type_e
{
  MONKEY_LOG_LEVEL_INFO,
  MONKEY_LOG_LEVEL_NOTICE,
  MONKEY_LOG_LEVEL_WARN,
  MONKEY_LOG_LEVEL_ERROR,
  MONKEY_LOG_LEVEL_LAST
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_log_printf
 ****************************************************************************/

void monkey_log_printf(enum monkey_log_level_type_e level,
                       FAR const char *func,
                       FAR const char *fmt,
                       ...);

/****************************************************************************
 * Name: monkey_log_set_level
 ****************************************************************************/

void monkey_log_set_level(enum monkey_log_level_type_e level);

/****************************************************************************
 * Name: monkey_log_get_level
 ****************************************************************************/

enum monkey_log_level_type_e monkey_log_get_level(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MONKEY_MONKEY_LOG_H */
