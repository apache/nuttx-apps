/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_pthread_test_003.c
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
 * Name: schedpthread03threadroutine
 ****************************************************************************/

static void *schedpthread03threadroutine(void *arg)
{
  int i = 0;
  for (i = 0; i <= 5; i++)
    {
      if (i == 3)
        {
          pthread_exit(0);
        }
    }

  /* This part of the code will not be executed */

  *((int *)arg) = 1;
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_pthread03
 ****************************************************************************/

void test_nuttx_sched_pthread03(FAR void **state)
{
  pthread_t pid_1;
  int ret;
  int run_flag = 0;

  /* create test thread */

  ret = pthread_create(&pid_1, NULL, (void *)schedpthread03threadroutine,
                       &run_flag);
  assert_int_equal(ret, 0);

  pthread_join(pid_1, NULL);

  assert_int_not_equal(run_flag, 1);
}
