/****************************************************************************
 * apps/interpreters/berry/include/berry_conf.h
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

#ifndef __APPS_INTERPRETERS_BERRY_INCLUDE_BERRY_CONF_H
#define __APPS_INTERPRETERS_BERRY_INCLUDE_BERRY_CONF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BE_DEBUG                        0
#define BE_INTGER_TYPE                  2
#define BE_USE_SINGLE_FLOAT             0
#define BE_BYTES_MAX_SIZE               (32 * 1024)
#define BE_USE_PRECOMPILED_OBJECT       1
#define BE_DEBUG_SOURCE_FILE            1
#define BE_DEBUG_RUNTIME_INFO           1
#define BE_DEBUG_VAR_INFO               1
#define BE_USE_PERF_COUNTERS            1
#define BE_VM_OBSERVABILITY_SAMPLING    20
#define BE_STACK_TOTAL_MAX              20000
#define BE_STACK_FREE_MIN               10
#define BE_STACK_START                  50
#define BE_CONST_SEARCH_SIZE            50
#define BE_USE_STR_HASH_CACHE           0
#define BE_USE_FILE_SYSTEM              1
#define BE_USE_SCRIPT_COMPILER          1
#define BE_USE_BYTECODE_SAVER           1
#define BE_USE_BYTECODE_LOADER          1
#define BE_USE_SHARED_LIB               0
#define BE_USE_OVERLOAD_HASH            1
#define BE_USE_DEBUG_HOOK               0
#define BE_USE_DEBUG_GC                 0
#define BE_USE_DEBUG_STACK              0
#define BE_USE_MEM_ALIGNED              0

#define BE_USE_STRING_MODULE            1
#define BE_USE_JSON_MODULE              1
#define BE_USE_MATH_MODULE              1
#define BE_USE_TIME_MODULE              1
#define BE_USE_OS_MODULE                1
#define BE_USE_GLOBAL_MODULE            1
#define BE_USE_SYS_MODULE               1
#define BE_USE_DEBUG_MODULE             1
#define BE_USE_GC_MODULE                1
#define BE_USE_SOLIDIFY_MODULE          1
#define BE_USE_INTROSPECT_MODULE        1
#define BE_USE_STRICT_MODULE            1

#define BE_EXPLICIT_ABORT               abort
#define BE_EXPLICIT_EXIT                exit
#define BE_EXPLICIT_MALLOC              malloc
#define BE_EXPLICIT_FREE                free
#define BE_EXPLICIT_REALLOC             realloc

#define be_assert(expr)                 assert(expr)

#endif /* __APPS_INTERPRETERS_BERRY_INCLUDE_BERRY_CONF_H */
