/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_pthread_test_007.c
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: schedpthread07threadroutine
 ****************************************************************************/

static void *schedpthread07threadroutine(void *arg)
{
  int i;
  pthread_mutex_t schedpthreadtest07_mutex = PTHREAD_MUTEX_INITIALIZER;
  for (i = 0; i < 100; i++)
    {
      pthread_mutex_lock(&schedpthreadtest07_mutex);
      (*((int *)arg))++;
      pthread_mutex_unlock(&schedpthreadtest07_mutex);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_pthread07
 ****************************************************************************/

void test_nuttx_sched_pthread07(FAR void **state)
{
  int res;
  pthread_t pt_1, pt_2, pt_3;
  int run_flag = 0;

  res = pthread_create(&pt_1, NULL, (void *)schedpthread07threadroutine,
                       &run_flag);
  assert_int_equal(res, OK);
  res = pthread_create(&pt_2, NULL, (void *)schedpthread07threadroutine,
                       &run_flag);
  assert_int_equal(res, OK);
  res = pthread_create(&pt_3, NULL, (void *)schedpthread07threadroutine,
                       &run_flag);
  assert_int_equal(res, OK);

  pthread_join(pt_1, NULL);
  pthread_join(pt_2, NULL);
  pthread_join(pt_3, NULL);
  sleep(5);

  assert_int_equal(run_flag, 300);
}
