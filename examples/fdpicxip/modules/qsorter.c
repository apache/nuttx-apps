/****************************************************************************
 * apps/examples/fdpicxip/modules/qsorter.c
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

/* Demonstrates the two things worth knowing about NuttX FDPIC modules.
 *
 * 1. qsort() runs in the firmware and calls back into compare(), which
 *    lives here.  That works because the firmware reserves the FDPIC
 *    register, so this module's data base survives the call in, and
 *    because libc resolves the callback descriptor on the way back out.
 *
 * 2. g_data is module data.  Every running instance gets a private copy,
 *    while the code itself is shared and executed in place from flash.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define N 8

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_data[N];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int compare(const void *a, const void *b)
{
  return *(const int *)a - *(const int *)b;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int seed = (argc > 1) ? atoi(argv[1]) : 0;
  int i;

  for (i = 0; i < N; i++)
    {
      g_data[i] = (N - i) * 10 + seed;
    }

  /* Overlap with any other instance, so shared data would show up as
   * corruption rather than passing by luck.
   */

  usleep(300000);

  qsort(g_data, N, sizeof(int), compare);

  syslog(LOG_INFO, "[inst %d] %d %d %d %d %d %d %d %d\n", seed,
         g_data[0], g_data[1], g_data[2], g_data[3],
         g_data[4], g_data[5], g_data[6], g_data[7]);
  return 0;
}
