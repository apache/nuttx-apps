/****************************************************************************
 * apps/examples/foc/foc_debug.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_DEBUG_H
#define __APPS_EXAMPLES_FOC_FOC_DEBUG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <inttypes.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Print support */

#if CONFIG_EXAMPLES_FOC_VERBOSE == 0
#  define PRINTF(...)
#  define PRINTFV(...)
#elif CONFIG_EXAMPLES_FOC_VERBOSE == 1
#  define PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#  define PRINTFV(...)
#else
#  define PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#  define PRINTFV(format, ...) printf(format, ##__VA_ARGS__)
#endif

/* If print is enabled we need float print support */

#if CONFIG_EXAMPLES_FOC_VERBOSE > 1
#  ifdef CONFIG_INDUSTRY_FOC_FLOAT
#    ifndef CONFIG_LIBC_FLOATINGPOINT
#      error "CONFIG_LIBC_FLOATINGPOINT must be set!"
#    endif
#  endif
#endif

#endif /* __APPS_EXAMPLES_FOC_FOC_DEBUG_H */
