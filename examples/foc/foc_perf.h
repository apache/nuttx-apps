/****************************************************************************
 * apps/examples/foc/foc_perf.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_PERF_H
#define __APPS_EXAMPLES_FOC_FOC_PERF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* The diagram below illustrates the operation of a simple FOC ocntroller
 * performance measurement tool:
 *
 *     | notify      | notify      | notify
 *     v             v             v
 *        ctrl          ctrl
 *      |------|      |------|      |---
 *      | BUSY | wait | BUSY | wait |
 *  ____|      |------|      |------|
 *
 *      ^      ^      ^
 *      | exec |      |
 *      |<---->|      |
 *      |             |
 *      |     per     |
 *      |<----------->|
 *
 *
 *  exec   - FOC control loop execution time
 *  per    - FOC control loop period
 *  notify - notification event from FOC device,
 *           called with CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ frequency
 */

struct foc_perf_s
{
  bool     exec_max_changed;    /* Max execution time changed */
  bool     per_max_changed;     /* Max period changed */
  uint32_t exec_max;            /* Control loop execution max */
  uint32_t per_max;             /* Control loop period max */
  uint32_t exec;                /* Temporary storage */
  uint32_t per;                 /* Temporary storage */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_perf_init(struct foc_perf_s *p);
void foc_perf_start(struct foc_perf_s *p);
void foc_perf_end(struct foc_perf_s *p);
void foc_perf_live(struct foc_perf_s *p);
void foc_perf_exit(struct foc_perf_s *p);

#endif /* __APPS_EXAMPLES_FOC_FOC_PERF_H */
