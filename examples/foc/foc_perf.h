/****************************************************************************
 * apps/examples/foc/foc_perf.h
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
 * Pre-processor Definitions
 ****************************************************************************/

#define PRINTF_PERF(format, ...) printf(format, ##__VA_ARGS__)

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

struct foc_perf_s
{
  bool     max_changed;
  uint32_t max;
  uint32_t now;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int foc_perf_init(struct foc_perf_s *p);
void foc_perf_start(struct foc_perf_s *p);
void foc_perf_end(struct foc_perf_s *p);

#endif /* __APPS_EXAMPLES_FOC_FOC_PERF_H */
