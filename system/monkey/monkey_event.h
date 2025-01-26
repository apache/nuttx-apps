/****************************************************************************
 * apps/system/monkey/monkey_event.h
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

#ifndef __APPS_SYSTEM_MONKEY_EVENT_H
#define __APPS_SYSTEM_MONKEY_EVENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include "monkey_type.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct monkey_event_param_s
{
  enum monkey_event_e event;
  int duration;
  int x1;
  int y1;
  int x2;
  int y2;
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
 * Name: monkey_event_gen
 ****************************************************************************/

void monkey_event_gen(FAR struct monkey_s *monkey,
                      FAR struct monkey_event_param_s *param);

/****************************************************************************
 * Name: monkey_event_exec
 ****************************************************************************/

bool monkey_event_exec(FAR struct monkey_s *monkey,
                       FAR struct monkey_dev_s *dev,
                       FAR const struct monkey_event_param_s *param);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MONKEY_EVENT_H */
