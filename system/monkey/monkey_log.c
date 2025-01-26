/****************************************************************************
 * apps/system/monkey/monkey_log.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <syslog.h>
#include <nuttx/streams.h>
#include "monkey_log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static enum monkey_log_level_type_e g_log_level = MONKEY_LOG_LEVEL_NOTICE;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_log_printf
 ****************************************************************************/

void monkey_log_printf(enum monkey_log_level_type_e level,
                       FAR const char *func,
                       FAR const char *fmt,
                       ...)
{
  struct va_format vaf;
  va_list ap;

  static const int priority[MONKEY_LOG_LEVEL_LAST] =
    {
      LOG_INFO, LOG_NOTICE, LOG_WARNING, LOG_ERR
    };

  if (level < g_log_level)
    {
      return;
    }

  va_start(ap, fmt);
  vaf.fmt = fmt;
  vaf.va  = &ap;
  syslog(priority[level], "[monkey] %s: %pV\n", func, &vaf);
  va_end(ap);
}

/****************************************************************************
 * Name: monkey_log_set_level
 ****************************************************************************/

void monkey_log_set_level(enum monkey_log_level_type_e level)
{
  if (level >= MONKEY_LOG_LEVEL_LAST)
    {
      MONKEY_LOG_WARN("error level: %d", level);
      return;
    }

  g_log_level = level;
}

/****************************************************************************
 * Name: monkey_log_get_level
 ****************************************************************************/

enum monkey_log_level_type_e monkey_log_get_level(void)
{
  return g_log_level;
}
