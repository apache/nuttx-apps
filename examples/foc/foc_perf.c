/****************************************************************************
 * apps/examples/foc/foc_perf.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <nuttx/clock.h>

#include "foc_perf.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PRINTF_PERF(format, ...) printf(format, ##__VA_ARGS__)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_perf_exec
 ****************************************************************************/

static bool foc_perf_exec(struct foc_perf_s *p, FAR uint32_t *exec)
{
  bool tmp = p->exec_max_changed;

  *exec = p->exec_max;
  p->exec_max_changed = false;

  return tmp;
}

/****************************************************************************
 * Name: foc_perf_per
 ****************************************************************************/

static bool foc_perf_per(struct foc_perf_s *p, FAR uint32_t *per)
{
  bool tmp = p->per_max_changed;

  *per = p->per_max;
  p->per_max_changed = false;

  return tmp;
}

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
  p->exec = perf_gettime();
}

/****************************************************************************
 * Name: foc_perf_end
 ****************************************************************************/

void foc_perf_end(struct foc_perf_s *p)
{
  uint32_t now = perf_gettime();
  uint32_t tmp = 0;

  if (p->per > 0)
    {
      tmp = now - p->per;
    }

  p->per = now;
  p->exec = now - p->exec;

  p->exec_max_changed = false;
  p->per_max_changed = false;

  if (p->exec > p->exec_max)
    {
      p->exec_max = p->exec;
      p->exec_max_changed = true;
    }

  if (tmp > p->per_max)
    {
      p->per_max = tmp;
      p->per_max_changed = true;
    }
}

/****************************************************************************
 * Name: foc_perf_live
 ****************************************************************************/

void foc_perf_live(struct foc_perf_s *p)
{
  uint32_t perf = 0;

  if (foc_perf_exec(p, &perf))
    {
      PRINTF_PERF("exec ticks=%" PRId32 "\n", perf);
    }

  if (foc_perf_per(p, &perf))
    {
      PRINTF_PERF("per ticks=%" PRId32 "\n", perf);
    }
}

/****************************************************************************
 * Name: foc_perf_exit
 ****************************************************************************/

void foc_perf_exit(struct foc_perf_s *p)
{
  struct timespec ts;
  uint32_t max = 0;

  PRINTF_PERF("===============================\n");

  foc_perf_exec(p, &max);
  perf_convert(max, &ts);
  PRINTF_PERF("exec ticks=%" PRId32 "\n", max);
  PRINTF_PERF("  nsec=%" PRId32 "\n", ts.tv_nsec);

  foc_perf_per(p, &max);
  perf_convert(max, &ts);
  PRINTF_PERF("per ticks=%" PRId32 "\n", max);
  PRINTF_PERF("  nsec=%" PRId32 "\n", ts.tv_nsec);

  PRINTF_PERF("===============================\n");
}
