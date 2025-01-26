/****************************************************************************
 * apps/system/monkey/monkey_dev.h
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

#ifndef __APPS_SYSTEM_MONKEY_MONKEY_DEV_H
#define __APPS_SYSTEM_MONKEY_MONKEY_DEV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "monkey_type.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct monkey_dev_s
{
  int fd;
  enum monkey_dev_type_e type;
  bool is_available;
};

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
 * Name: monkey_dev_create
 ****************************************************************************/

FAR struct monkey_dev_s *monkey_dev_create(FAR const char *dev_path,
                                           enum monkey_dev_type_e type);

/****************************************************************************
 * Name: monkey_dev_delete
 ****************************************************************************/

void monkey_dev_delete(FAR struct monkey_dev_s *dev);

/****************************************************************************
 * Name: monkey_dev_set_state
 ****************************************************************************/

void monkey_dev_set_state(FAR struct monkey_dev_s *dev,
                          FAR const struct monkey_dev_state_s *state);

/****************************************************************************
 * Name: monkey_dev_get_state
 ****************************************************************************/

bool monkey_dev_get_state(FAR struct monkey_dev_s *dev,
                          FAR struct monkey_dev_state_s *state);

/****************************************************************************
 * Name: monkey_dev_get_type
 ****************************************************************************/

enum monkey_dev_type_e monkey_dev_get_type(FAR struct monkey_dev_s *dev);

/****************************************************************************
 * Name: monkey_dev_get_available
 ****************************************************************************/

int monkey_dev_get_available(FAR struct monkey_dev_s *devs[], int dev_num);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MONKEY_MONKEY_DEV_H */
