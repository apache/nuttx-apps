/****************************************************************************
 * apps/system/monkey/monkey.h
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

#ifndef __APPS_SYSTEM_MONKEY_MONKEY_H
#define __APPS_SYSTEM_MONKEY_MONKEY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include "monkey_type.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: monkey_create
 ****************************************************************************/

FAR struct monkey_s *monkey_create(int dev_type_mask);

/****************************************************************************
 * Name: monkey_update
 ****************************************************************************/

int monkey_update(FAR struct monkey_s *monkey);

/****************************************************************************
 * Name: monkey_delete
 ****************************************************************************/

void monkey_delete(FAR struct monkey_s *monkey);

/****************************************************************************
 * Name: monkey_config_default_init
 ****************************************************************************/

void monkey_config_default_init(FAR struct monkey_config_s *config);

/****************************************************************************
 * Name: monkey_set_config
 ****************************************************************************/

void monkey_set_config(FAR struct monkey_s *monkey,
                       FAR const struct monkey_config_s *config);

/****************************************************************************
 * Name: monkey_set_mode
 ****************************************************************************/

void monkey_set_mode(FAR struct monkey_s *monkey, enum monkey_mode_e mode);

/****************************************************************************
 * Name: monkey_set_period
 ****************************************************************************/

void monkey_set_period(FAR struct monkey_s *monkey, uint32_t period);

/****************************************************************************
 * Name: monkey_set_recorder_path
 ****************************************************************************/

bool monkey_set_recorder_path(FAR struct monkey_s *monkey,
                              FAR const char *path);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MONKEY_MONKEY_H */
