/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/getitimer_test.c
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
#include <nuttx/clock.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/time.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#define ITIMER_REAL 0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF 2
#define CLOCK_MONOTONIC_COARSE 6
#define SEC 100
#define USEC 10000
struct timeval tv;
struct itimerval value;
struct timespec time_res;
long jiffy;

/****************************************************************************
 * Name: TestNuttxAccept01
 ****************************************************************************/

void set_settitimer_value(int sec, int usec)
{
  value.it_value.tv_sec = sec;
  value.it_value.tv_usec = usec;
  value.it_interval.tv_sec = sec;
  value.it_interval.tv_usec = usec;
}

void test_nuttx_syscall_veritygetitimer(int which, char *dec)
{
  if (which == ITIMER_REAL)
    {
      assert_int_not_equal(gettimeofday(&tv, NULL), -1);
    }

  int ret = getitimer(which, &value);
  assert_int_equal(ret, 0);
  assert_int_equal(value.it_value.tv_sec, 0);
  assert_int_equal(value.it_value.tv_usec, 0);
  assert_int_equal(value.it_interval.tv_sec, 0);
  assert_int_equal(value.it_interval.tv_usec, 0);
  set_settitimer_value(SEC, USEC);
  int ret2 = setitimer(which, &value, NULL);
  assert_int_equal(ret2, 0);
  set_settitimer_value(0, 0);
  int ret3 = getitimer(which, &value);
  assert_int_equal(ret3, 0);

  assert_int_equal(value.it_interval.tv_sec, SEC);
  assert_int_equal(value.it_interval.tv_usec, USEC);

  /* syslog(LOG_INFO,"value.it_value.tv_sec=%lld,
   * value.it_value.tv_usec=%ld", value.it_value.tv_sec,
   * value.it_value.tv_usec);
   */

  long margin = (which == ITIMER_REAL) ? (2 * USEC_PER_TICK) : jiffy;
  if (value.it_value.tv_sec == SEC)
    {
      assert_in_range(value.it_value.tv_usec, 0, USEC + margin);
    }

  else
    {
      assert_in_range(value.it_value.tv_sec, 0, SEC);
    }

  /* syslog(LOG_INFO,"timer value is within the expected range"); */

  if (which == ITIMER_REAL)
    {
      assert_int_not_equal(gettimeofday(&tv, NULL), -1);
    }

  set_settitimer_value(0, 0);
  int ret4 = setitimer(which, &value, NULL);
  assert_int_equal(ret4, 0);
}

void test_nuttx_syscall_getitimer01(FAR void **state)
{
  struct tcase
  {
    int which;
    char *des;
  }

  tcases[] =
    {
        {
          ITIMER_REAL, "ITIMER_REAL"
        },

      /* {ITIMER_VIRTUAL, "ITIMER_VIRTUAL"},
       * {ITIMER_PROF,    "ITIMER_PROF"},
       */
    };

  clock_getres(CLOCK_MONOTONIC_COARSE, &time_res);
  jiffy = (time_res.tv_nsec + 999) / 1000;

  test_nuttx_syscall_veritygetitimer(tcases[0].which, tcases[0].des);
}
