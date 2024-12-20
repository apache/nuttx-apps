/****************************************************************************
 * apps/lte/alt1250/alt1250_dbg.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_DBG_H
#define __APPS_LTE_ALT1250_ALT1250_DBG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_LTE_ALT1250_DEBUG_MSG
#  define err_alt1250(v, ...) nerr(v, ##__VA_ARGS__)
#  define dbg_alt1250(v, ...) ninfo(v, ##__VA_ARGS__)
#else
#  define err_alt1250(v, ...)
#  define dbg_alt1250(v, ...)
#endif

#endif /* __APPS_LTE_ALT1250_ALT1250_DBG_H */
