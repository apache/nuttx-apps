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

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pkg_vlog(FAR FILE *stream, FAR const char *level,
                     FAR const char *fmt, va_list ap)
{
  fprintf(stream, "nxpkg: %s: ", level);
  vfprintf(stream, fmt, ap);
  fputc('\n', stream);
  fflush(stream);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pkg_error(FAR const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  pkg_vlog(stderr, "error", fmt, ap);
  va_end(ap);
}

void pkg_info(FAR const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  pkg_vlog(stdout, "info", fmt, ap);
  va_end(ap);
}
