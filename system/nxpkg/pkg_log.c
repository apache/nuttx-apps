/****************************************************************************
 * apps/system/nxpkg/pkg_log.c
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
#include <stdio.h>
#include <syslog.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pkg_vlog(int priority, FAR const char *fmt, va_list ap)
{
  char message[256];
  int ret;

  ret = vsnprintf(message, sizeof(message), fmt, ap);
  if (ret < 0)
    {
      return;
    }

  syslog(priority, "nxpkg: %s", message);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pkg_error(FAR const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  pkg_vlog(LOG_ERR, fmt, ap);
  va_end(ap);
}

void pkg_info(FAR const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  pkg_vlog(LOG_INFO, fmt, ap);
  va_end(ap);
}
