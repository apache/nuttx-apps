/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cases/api_pthread_test_002.c
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
 * Name: schedpthread02threadroutine
 ****************************************************************************/

static void *schedpthread02threadroutine(void *arg)
{
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_sched_pthread02
 ****************************************************************************/

void test_nuttx_sched_pthread02(FAR void **state)
{
  int res;
  pthread_t p_t;
  pthread_attr_t attr;
  size_t statck_size;
  struct sched_param param;
  struct sched_param o_param;

  /* Initializes thread attributes object (attr) */

  res = pthread_attr_init(&attr);
  assert_int_equal(res, OK);

  res = pthread_attr_setstacksize(&attr, PTHREAD_STACK_SIZE);
  assert_int_equal(res, OK);

  res = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  assert_int_equal(res, OK);

  param.sched_priority = TASK_PRIORITY + 1;

  res = pthread_attr_setschedparam(&attr, &param);
  assert_int_equal(res, OK);

  /* create thread */

  pthread_create(&p_t, &attr, schedpthread02threadroutine, NULL);

  /* Wait for the child thread finish */

  pthread_join(p_t, NULL);

  /* get schedparam */

  res = pthread_attr_getschedparam(&attr, &o_param);
  assert_int_equal(res, OK);

  /* get stack size */

  res = pthread_attr_getstacksize(&attr, &statck_size);
  assert_int_equal(res, OK);
}
