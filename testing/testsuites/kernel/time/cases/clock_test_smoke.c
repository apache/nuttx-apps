/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_smoke.c
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
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "TimeTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_clock_test_smoke01
 ****************************************************************************/

void test_nuttx_clock_test_smoke01(FAR void **state)
{
  clockid_t clk = CLOCK_REALTIME;
  struct timespec res =
  {
    0, 0
  };

  struct timespec setts =
  {
    0, 0
  };

  struct timespec oldtp =
  {
    0, 0
  };

  struct timespec ts =
  {
    0, 0
  };

  int ret;
  int passflag = 0;

  /* get clock resolution */

  ret = clock_getres(clk, &res);
  assert_int_equal(ret, 0);

  /* get clock realtime */

  ret = clock_gettime(clk, &oldtp);
  syslog(LOG_INFO,
         "the clock current time: %lld second, %ld nanosecond\n",
         (long long)oldtp.tv_sec, oldtp.tv_nsec);
  assert_int_equal(ret, 0);

  /* set clock realtime */

  setts.tv_sec = oldtp.tv_sec + 1;
  setts.tv_nsec = oldtp.tv_nsec;
  syslog(LOG_INFO,
         "the clock setting time: %lld second, %ld nanosecond\n",
         (long long)setts.tv_sec, setts.tv_nsec);
  ret = clock_settime(CLOCK_REALTIME, &setts);
  assert_int_equal(ret, 0);

  ret = clock_gettime(clk, &ts);
  syslog(LOG_INFO,
         "obtaining the current time after setting: %lld second, %ld "
         "nanosecond\n",
         (long long)ts.tv_sec, ts.tv_nsec);

  passflag =
      (ts.tv_sec >= setts.tv_sec) &&
      (ts.tv_sec <=
       setts.tv_sec + 1); /* 1, means obtaining time's errno is 1 second. */
  assert_int_equal(ret, 0);
  assert_int_equal(passflag, 1);
}
