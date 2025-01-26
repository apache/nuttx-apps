/****************************************************************************
 * apps/system/monkey/monkey_utils.h
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

#ifndef __APPS_SYSTEM_MONKEY_MONKEY_UTILS_H
#define __APPS_SYSTEM_MONKEY_MONKEY_UTILS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>
#include "monkey_type.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_random
 ****************************************************************************/

int monkey_random(int min, int max);

/****************************************************************************
 * Name: monkey_tick_get
 ****************************************************************************/

uint32_t monkey_tick_get(void);

/****************************************************************************
 * Name: monkey_tick_elaps
 ****************************************************************************/

uint32_t monkey_tick_elaps(uint32_t act_time, uint32_t prev_tick);

/****************************************************************************
 * Name: monkey_get_localtime_str
 ****************************************************************************/

void monkey_get_localtime_str(FAR char *str_buf, size_t buf_size);

/****************************************************************************
 * Name: monkey_dir_check
 ****************************************************************************/

bool monkey_dir_check(FAR const char *dir_path);

/****************************************************************************
 * Name: monkey_map
 ****************************************************************************/

int monkey_map(int x, int min_in, int max_in, int min_out, int max_out);

/****************************************************************************
 * Name: monkey_dev_type2name
 ****************************************************************************/

FAR const char *monkey_dev_type2name(enum monkey_dev_type_e type);

/****************************************************************************
 * Name: monkey_dev_name2type
 ****************************************************************************/

enum monkey_dev_type_e monkey_dev_name2type(FAR const char *name);

/****************************************************************************
 * Name: monkey_event_type2name
 ****************************************************************************/

FAR const char *monkey_event_type2name(enum monkey_event_e event);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MONKEY_MONKEY_UTILS_H */
