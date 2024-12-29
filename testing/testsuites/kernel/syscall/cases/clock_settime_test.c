/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/clock_settime_test.c
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
#include <errno.h>
#include <time.h>
#include <syslog.h>
#include <stdlib.h>
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
 * Name: test_nuttx_syscall_clocksettime01
 ****************************************************************************/

void test_nuttx_syscall_clocksettime01(FAR void **state)
{
  int ret;
  struct timespec ts;
  time_t begin_time;
  time_t end_time;

  /* do clock_gettime */

  ret = clock_gettime(CLOCK_REALTIME, &ts);
  assert_int_equal(ret, 0);

  begin_time = ts.tv_sec;
  ts.tv_sec = ts.tv_sec + 10;
  ret = clock_settime(CLOCK_REALTIME, &ts);
  assert_int_equal(ret, 0);

  ret = clock_gettime(CLOCK_REALTIME, &ts);
  assert_int_equal(ret, 0);

  end_time = ts.tv_sec;
  assert_true((end_time - begin_time) >= 10);
}
