/****************************************************************************
 * apps/system/trace/trace.h
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

#ifndef __APPS_SYSTEM_TRACE_TRACE_H
#define __APPS_SYSTEM_TRACE_TRACE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>

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

typedef enum
{
  TRACE_TYPE_LTTNG_KERNEL = 0,  /* Common Trace Format : Linux Kernel Trace */
  TRACE_TYPE_GENERIC_CTF  = 1,  /* Common Trace Format : Generic CTF Trace */
  TRACE_TYPE_LTTNG_UST    = 2,  /* Common Trace Format : LTTng UST Trace */
  TRACE_TYPE_CUSTOM_TEXT  = 3,  /* Custom Text :         TmfGeneric */
  TRACE_TYPE_CUSTOM_XML   = 4,  /* Custom XML :          Custom XML Log */
  TRACE_TYPE_ANDROID      = 5,  /* Custom Format :       Android ATrace */
} trace_dump_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_DRIVER_NOTERAM

/****************************************************************************
 * Name: trace_dump
 *
 * Description:
 *   Read notes and dump trace results.
 *
 ****************************************************************************/

int trace_dump(trace_dump_t type, FAR FILE *out);

/****************************************************************************
 * Name: trace_dump_clear
 *
 * Description:
 *   Clear all contents of the buffer
 *
 ****************************************************************************/

void trace_dump_clear(void);

/****************************************************************************
 * Name: trace_dump_get_overwrite
 *
 * Description:
 *   Get overwrite mode
 *
 ****************************************************************************/

bool trace_dump_get_overwrite(void);

/****************************************************************************
 * Name: trace_dump_set_overwrite
 *
 * Description:
 *   Set overwrite mode
 *
 ****************************************************************************/

void trace_dump_set_overwrite(bool mode);

#else /* CONFIG_DRIVER_NOTERAM */

#define trace_dump(type,out)
#define trace_dump_clear()
#define trace_dump_get_overwrite()      0
#define trace_dump_set_overwrite(mode)  (void)(mode)

#endif /* CONFIG_DRIVER_NOTERAM */

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_TRACE_TRACE_H */
