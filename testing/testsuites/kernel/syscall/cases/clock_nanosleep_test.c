/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/clock_nanosleep_test.c
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
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_clocknanosleep01
 ****************************************************************************/

void test_nuttx_syscall_clocknanosleep01(FAR void **state)
{
  int ret;
  clockid_t type = CLOCK_REALTIME;
  struct timespec t;
  clock_t start_time, end_time;
  t.tv_sec = 2;
  t.tv_nsec = 0;
  start_time = clock();
  ret = clock_nanosleep(type, 0, &t, NULL);
  assert_int_equal(ret, 0);
  end_time = clock();
  assert_int_equal((time_t)(end_time - start_time) / CLOCKS_PER_SEC,
                   t.tv_sec);
}

/****************************************************************************
 * Name: test_nuttx_syscall_clocknanosleep02
 ****************************************************************************/

void test_nuttx_syscall_clocknanosleep02(FAR void **state)
{
  int ret;
  struct timespec ts;
  clockid_t tcase[] =
    {
        CLOCK_MONOTONIC,
        CLOCK_REALTIME,
    };

  ret = clock_gettime(tcase[0], &ts);
  assert_int_equal(ret, 0);

  ts.tv_sec = 4;
  ts.tv_nsec = 1000;

  ret = clock_nanosleep(tcase[0], TIMER_ABSTIME, &ts, NULL);
  assert_int_equal(ret, 0);

  ret = clock_gettime(tcase[1], &ts);
  assert_int_equal(ret, 0);

  ts.tv_sec = 4;
  ts.tv_nsec = 1000;

  ret = clock_nanosleep(tcase[1], TIMER_ABSTIME, &ts, NULL);
  assert_int_equal(ret, 0);
}
