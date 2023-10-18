/****************************************************************************
 * apps/testing/ostest/smp_call.c
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

#include <assert.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>

#include <nuttx/sched.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int smp_call_func(void *arg)
{
  FAR sem_t *psem = arg;
  sem_post(psem);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void smp_call_test(void)
{
  cpu_set_t cpuset;
  sem_t sem;
  int cpucnt;
  int cpu;
  int value;
  int status;

  printf("smp_call_test: Test start\n");

  sem_init(&sem, 0, 0);

  for (cpu = 0; cpu < CONFIG_SMP_NCPUS; cpu++)
    {
      printf("smp_call_test: Call cpu %d, nowait\n", cpu);

      nxsched_smp_call_single(cpu, smp_call_func, &sem, false);

      status = sem_wait(&sem);
      if (status != 0)
        {
          printf("smp_call_test: Check smp call error\n");
          ASSERT(false);
        }

      printf("smp_call_test: Call cpu %d, wait\n", cpu);

      nxsched_smp_call_single(cpu, smp_call_func, &sem, true);

      sem_getvalue(&sem, &value);
      if (value != 1)
        {
          printf("smp_call_test: Check smp call wait error\n");
          ASSERT(false);
        }

      nxsem_reset(&sem, 0);
    }

  printf("smp_call_test: Call multi cpu, nowait\n");

  sched_getaffinity(0, sizeof(cpu_set_t), &cpuset);
  cpucnt = CPU_COUNT(&cpuset);

  nxsched_smp_call(cpuset, smp_call_func, &sem, false);

  for (cpu = 0; cpu < cpucnt; cpu++)
    {
      status = sem_wait(&sem);
      if (status != 0)
        {
          printf("smp_call_test: Check smp call error\n");
          ASSERT(false);
        }
    }

  printf("smp_call_test: Call multi cpu, wait\n");

  nxsched_smp_call(cpuset, smp_call_func, &sem, true);

  sem_getvalue(&sem, &value);
  if (value != cpucnt)
    {
      printf("smp_call_test: Check smp call wait error\n");
      ASSERT(false);
    }

  sem_destroy(&sem);

  printf("smp_call_test: Test success\n");
}
