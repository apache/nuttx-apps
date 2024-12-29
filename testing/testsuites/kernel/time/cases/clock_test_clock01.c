/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_clock01.c
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
 * Name: test_nuttx_clock_test_clock01
 ****************************************************************************/

void test_nuttx_clock_test_clock01(FAR void **state)
{
  clockid_t clk = CLOCK_REALTIME;
  struct timespec res;
  struct timespec tp;
  struct timespec oldtp;
  int ret;
  int passflag = 0;

  /* get clock resolution */

  ret = clock_getres(clk, &res);
  assert_int_equal(ret, 0);

  /* get current real time */

  ret = clock_gettime(clk, &oldtp);
  syslog(LOG_INFO, "The current real time: sec is %lld, nsec is %ld\n",
         (long long)oldtp.tv_sec, oldtp.tv_nsec);
  assert_int_equal(ret, 0);

  syslog(LOG_INFO, "sleep 2 seconds\n");
  sleep(2);

  /* 2, use for testing clock setting */

  tp.tv_sec = oldtp.tv_sec + 2;
  tp.tv_nsec = oldtp.tv_nsec;

  /* set real time */

  ret = clock_settime(clk, &tp);
  syslog(LOG_INFO, "Setting time: sec is %lld, nsec is %ld\n",
         (long long)tp.tv_sec, tp.tv_nsec);
  assert_int_equal(ret, 0);

  syslog(LOG_INFO, "get real time clock again\n");

  /* get current real time again */

  ret = clock_gettime(clk, &tp);
  syslog(LOG_INFO,
         "Obtaining the current time after setting: sec = %lld, nsec = "
         "%ld\n",
         (long long)tp.tv_sec, tp.tv_nsec);
  passflag = (tp.tv_sec >= 2 + oldtp.tv_sec) &&
             (tp.tv_sec <=
              2 + oldtp.tv_sec + 1); /* 2, use for testing clock setting */

  assert_int_equal(ret, 0);
  assert_int_equal(passflag, 1);
}
