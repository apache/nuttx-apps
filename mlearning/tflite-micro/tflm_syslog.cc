/****************************************************************************
 * apps/mlearning/tflite-micro/tflm_syslog.cc
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

#include "tensorflow/lite/micro/micro_log.h"

#include <syslog.h>
#include <stdio.h>

#if !defined(TF_LITE_STRIP_ERROR_STRINGS)
#include "tensorflow/lite/micro/debug_log.h"
#endif

#if !defined(TF_LITE_STRIP_ERROR_STRINGS)

void VMicroPrintf(FAR const char *format, va_list ap)
{
  vsyslog(CONFIG_TFLITEMICRO_SYSLOG_LEVEL, format, ap);
}

void MicroPrintf(FAR const char *format, ...)
{
  va_list ap;

  /* Let vsyslog do the work */

  va_start(ap, format);
  vsyslog(CONFIG_TFLITEMICRO_SYSLOG_LEVEL, format, ap);
  va_end(ap);
}

int MicroSnprintf(FAR char *buffer, size_t buf_size, FAR const char *format, ...)
{
  va_list ap;
  int ret;

  va_start(ap, format);
  ret = vsnprintf(buffer, buf_size, format, ap);
  va_end(ap);

  return ret;
}

int MicroVsnprintf(FAR char *buffer, size_t buf_size,
                   FAR const char *format, va_list vlist)
{
  return vsnprintf(buffer, buf_size, format, vlist);
}

#endif  // !defined(TF_LITE_STRIP_ERROR_STRINGS)
