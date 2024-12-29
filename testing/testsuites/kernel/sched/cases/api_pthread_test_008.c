/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_pthread_test_008.c
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

static sem_t schedtask08_sem;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedpthread08threadroutine
 ****************************************************************************/

static void *schedpthread08threadroutine(void *arg)
{
  int i;
  for (i = 0; i < 5; i++)
    {
      sem_wait(&schedtask08_sem);
      (*((int *)arg))++;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_pthread08
 ****************************************************************************/

void test_nuttx_sched_pthread08(FAR void **state)
{
  int res;
  pthread_t pthread_id;
  int run_flag = 0;

  res = sem_init(&schedtask08_sem, 0, 0);
  assert_int_equal(res, OK);

  /* create pthread */

  res = pthread_create(&pthread_id, NULL,
                       (void *)schedpthread08threadroutine, &run_flag);
  assert_int_equal(res, OK);

  while (1)
    {
      sleep(2);
      res = sem_post(&schedtask08_sem);
      assert_int_equal(res, OK);
      if (run_flag == 5)
        break;
    }

  res = sem_destroy(&schedtask08_sem);
  assert_int_equal(res, OK);
  assert_int_equal(run_flag, 5);
}
