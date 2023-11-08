/****************************************************************************
 * apps/examples/foc/foc_perf.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <nuttx/clock.h>

#include "foc_perf.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_perf_init
 ****************************************************************************/

int foc_perf_init(struct foc_perf_s *p)
{
  memset(p, 0, sizeof(struct foc_perf_s));

  return OK;
}

/****************************************************************************
 * Name: foc_perf_start
 ****************************************************************************/

void foc_perf_start(struct foc_perf_s *p)
{
  p->now = perf_gettime();
}

/****************************************************************************
 * Name: foc_perf_end
 ****************************************************************************/

void foc_perf_end(struct foc_perf_s *p)
{
  p->now = perf_gettime() - p->now;

  p->max_changed = false;

  if (p->now > p->max)
    {
      p->max = p->now;
      p->max_changed = true;
    }
}
