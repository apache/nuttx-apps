/****************************************************************************
 * apps/examples/smf/hsm_psicc2_thread.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
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

#ifndef __APPS_EXAMPLES_SMF_HSM_PSICC2_THREAD_H
#define __APPS_EXAMPLES_SMF_HSM_PSICC2_THREAD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct hsm_psicc2_event
{
  uint32_t event_id;
};

enum demo_events
{
  EVENT_A,
  EVENT_B,
  EVENT_C,
  EVENT_D,
  EVENT_E,
  EVENT_F,
  EVENT_G,
  EVENT_H,
  EVENT_I,
  EVENT_TERMINATE
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int hsm_psicc2_thread_start(void);
int hsm_psicc2_post_event(uint32_t event_id);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_EXAMPLES_SMF_HSM_PSICC2_THREAD_H */
