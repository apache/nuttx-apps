/****************************************************************************
 * apps/testing/ostest/intstack.c
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
#include <stdio.h>

#include <nuttx/arch.h>
#include <nuttx/sched.h>

#include "ostest.h"

#if defined(CONFIG_SMP) && defined(CONFIG_BUILD_FLAT) && \
    defined(CONFIG_STACK_COLORATION) && CONFIG_ARCH_INTERRUPTSTACK > 3

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int intstack_func(FAR void *arg)
{
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: intstack_test
 *
 * Description:
 *   Each CPU takes an interrupt, then its interrupt stack must show some
 *   use and must still have some of its color.  A stack with no color
 *   reports that all of it is used.
 *
 ****************************************************************************/

void intstack_test(void)
{
  size_t used;
  int cpu;

  for (cpu = 0; cpu < CONFIG_SMP_NCPUS; cpu++)
    {
      nxsched_smp_call_single(cpu, intstack_func, NULL);

      used = up_check_intstack(cpu, 0);
      printf("intstack_test: CPU %d used %zu of %d\n", cpu, used,
             CONFIG_ARCH_INTERRUPTSTACK);
      ASSERT(used > 0);
      ASSERT(used < CONFIG_ARCH_INTERRUPTSTACK);
    }

  printf("intstack_test: Test success\n");
}
#endif
