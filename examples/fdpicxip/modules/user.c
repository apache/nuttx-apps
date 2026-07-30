/****************************************************************************
 * apps/examples/fdpicxip/modules/user.c
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

/* Uses libcounter.so.  Two instances run concurrently: if the library's
 * data were shared between them the totals would interleave.
 *
 * libcounter is also a *leaf* library -- it calls nothing outside itself,
 * so it has no PLT and therefore no DT_PLTGOT.  The loader has to fall back
 * to PT_DYNAMIC vaddr + memsz to find its GOT.  If that fallback is wrong
 * the library cannot reach its own globals, which is exactly what the total
 * below would expose.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern int counter_bump(int by);
extern int counter_total(void);

int main(int argc, char *argv[])
{
  int seed = (argc > 1) ? atoi(argv[1]) : 0;
  int i;

  for (i = 0; i < 3; i++)
    {
      counter_bump(seed);
      usleep(100000);
    }

  syslog(LOG_INFO, "[user %d] library total = %d (expected %d) -- %s\n",
         seed, counter_total(), seed * 3,
         counter_total() == seed * 3 ? "PASS" : "FAIL");

  /* Reported through the exit status as well as the log, so a test can
   * assert on it rather than a human reading the console.
   */

  return counter_total() == seed * 3 ? EXIT_SUCCESS : EXIT_FAILURE;
}
