/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_pthread_test_006.c
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
 * Name: schedpthread06threadroutine
 ****************************************************************************/

static void *schedpthread06threadroutine(void *arg)
{
  int pid;

  /* call pthread_self() */

  pid = pthread_self();
  assert_true(pid > 0);
  if (pid > 0)
    {
      *((int *)arg) = 0;
    }

  else
    {
      *((int *)arg) = 1;
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_pthread06
 ****************************************************************************/

void test_nuttx_sched_pthread06(FAR void **state)
{
  pthread_t p_t;
  int run_flag = 0;
  int res;

  /* create thread */

  res =
      pthread_create(&p_t, NULL, schedpthread06threadroutine, &run_flag);
  assert_int_equal(res, OK);

  pthread_join(p_t, NULL);

  assert_int_equal(run_flag, 0);
}
