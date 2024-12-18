/****************************************************************************
 * apps/benchmarks/spinlock_bench/spinlock_bench.c
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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <nuttx/spinlock.h>
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_NUM CONFIG_SPINLOCK_MULTITHREAD
#define THREAD_NUM CONFIG_SPINLOCK_MULTITHREAD

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct thread_parmeter_s
{
  FAR int *result;
  FAR spinlock_t *lock;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *thread_spinlock(FAR void *parameter)
{
  FAR int *result = ((FAR struct thread_parmeter_s *)parameter)->result;
  FAR spinlock_t *lock = ((FAR struct thread_parmeter_s *)parameter)->lock;

  int i;

  for (i = 0; i < TEST_NUM; i++)
    {
      spin_lock(lock);
      (*result)++;
      spin_unlock(lock);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void main(void)
{
  spinlock_t lock = SP_UNLOCKED;
  int result = 0;
  pthread_t thread[THREAD_NUM];
  struct thread_parmeter_s para;
  clock_t start;
  clock_t end;

  int status;
  int i;

  para.result = &result;
  para.lock = &lock;

  start = perf_gettime();
  for (i = 0; i < THREAD_NUM; ++i)
    {
      status = pthread_create(&thread[i], NULL,
                              thread_spinlock, &para);
      if (status != 0)
        {
          printf("spinlock_test: ERROR pthread_create failed, status=%d\n",
                 status);
          ASSERT(false);
        }
    }

  for (i = 0; i < THREAD_NUM; ++i)
    {
      pthread_join(thread[i], NULL);
    }

  end = perf_gettime();
  assert(result == THREAD_NUM * TEST_NUM);

  printf("total_time: %lu\n", end - start);
}
