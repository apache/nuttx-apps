/****************************************************************************
 * apps/testing/testsuites/kernel/time/cases/clock_test_timer03.c
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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include "TimeTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SIG SIGALRM
#define CLOCKID CLOCK_REALTIME

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_clock_test_timer03
 ****************************************************************************/

void test_nuttx_clock_test_timer03(FAR void **state)
{
  int ret = 0;
  int sig = 0;
  int failed = 0;
  timer_t timerid;
  sigset_t set;
  sigset_t oldset;
  struct sigevent sev;

  ret = sigemptyset(&set);
  assert_int_equal(ret, 0);

  ret = sigaddset(&set, SIG);
  assert_int_equal(ret, 0);

  ret = sigprocmask(SIG_BLOCK, &set, &oldset);
  assert_int_equal(ret, 0);

  /* Create the timer */

  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = &timerid;
  ret = timer_create(CLOCKID, &sev, &timerid);
  syslog(LOG_INFO, "timer_create %p: %d", timerid, ret);
  assert_int_equal(ret, 0);

  struct timespec testcases[] =
      {
          {0, 30000000},
          {1, 0},
          {1, 5000},
      };

  struct timespec zero =
      {
        0, 0
      };

  for (int i = 0; i < sizeof(testcases) / sizeof(testcases[0]); ++i)
    {
      struct timespec start;
      struct timespec end;
      struct itimerspec its;
      int64_t expected, escaped;

      its.it_interval = zero;
      its.it_value = testcases[i];

      ret = clock_gettime(CLOCKID, &start);
      assert_int_equal(ret, 0);

      ret = timer_settime(timerid, 0, &its, NULL);
      assert_int_equal(ret, 0);

      ret = sigwait(&set, &sig);
      assert_int_equal(ret, 0);

      ret = clock_gettime(CLOCKID, &end);
      assert_int_equal(ret, 0);

      expected =
          its.it_value.tv_sec * (int64_t)(1e9) + its.it_value.tv_nsec;
      escaped = end.tv_sec * (int64_t)(1e9) + end.tv_nsec -
                start.tv_sec * (int64_t)(1e9) - start.tv_nsec;

      failed += (escaped < expected ||
                 (escaped - expected) >= 20000000); /* 20000000, 2 ticks. */
      syslog(LOG_INFO, "expected = %" PRId64 " escaped = %" PRId64
             "failed = %d", expected, escaped, failed);
    }

  ret = timer_delete(timerid);
  assert_int_equal(ret, 0);

  ret = sigprocmask(SIG_SETMASK, &oldset, NULL);
  assert_int_equal(ret, 0);

  assert_int_equal(failed, 0);
}
