/****************************************************************************
 * apps/include/industry/foc/foc_log.h
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
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_INDUSTRY_FOC_FOC_LOG_H
#define __APPS_INCLUDE_INDUSTRY_FOC_FOC_LOG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <inttypes.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_DEBUG
#  define FOCLIBLOG(format, ...)  printf(format, ##__VA_ARGS__)
#else
#  define FOCLIBLOG(format, ...)
#endif

#ifdef CONFIG_INDUSTRY_FOC_ERROR
#  define FOCLIBERR(format, ...)  printf(format, ##__VA_ARGS__)
#else
#  define FOCLIBERR(format, ...)
#endif

#ifdef CONFIG_INDUSTRY_FOC_WARN
#  define FOCLIBWARN(format, ...) printf(format, ##__VA_ARGS__)
#else
#  define FOCLIBWARN(format, ...)
#endif

#endif /* __APPS_INCLUDE_INDUSTRY_FOC_FOC_LOG_H */
