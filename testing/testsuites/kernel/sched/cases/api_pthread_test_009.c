/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_pthread_test_009.c
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
#include <stdio.h>
#include <syslog.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SchedTest.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t schedtask09_sem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedpthread09threadroutine
 ****************************************************************************/

static void *schedpthread09threadroutine(void *arg)
{
  int i;
  int res;
  for (i = 0; i < 10; i++)
    {
      res = sem_wait(&schedtask09_sem);
      assert_int_equal(res, OK);
      (*((int *)arg))++;
      res = sem_post(&schedtask09_sem);
      assert_int_equal(res, OK);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_pthread09
 ****************************************************************************/

void test_nuttx_sched_pthread09(FAR void **state)
{
  int res;
  pthread_t pthread_id[10];
  int run_flag = 0;

  res = sem_init(&schedtask09_sem, 0, 1);
  assert_int_equal(res, OK);

  int i;
  for (i = 0; i < 10; i++)
    {
      res =
          pthread_create(&pthread_id[i], NULL,
                         (void *)schedpthread09threadroutine, &run_flag);
      assert_int_equal(res, OK);
    }

  int j;
  for (j = 0; j < 10; j++)
    pthread_join(pthread_id[j], NULL);

  res = sem_destroy(&schedtask09_sem);
  assert_int_equal(res, OK);

  assert_int_equal(run_flag, 100);
}
