/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/timer_gettime_test.c
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
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <syslog.h>
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
 * Name: test_nuttx_syscall_timergettime01
 ****************************************************************************/

void test_nuttx_syscall_timergettime01(FAR void **state)
{
  int lc;
  int ret;
  struct sigevent ev;
  struct itimerspec spec;
  timer_t timer;

  ev.sigev_value = (union sigval)0;
  ev.sigev_signo = SIGALRM;
  ev.sigev_notify = SIGEV_SIGNAL;

  ret = timer_create(CLOCK_REALTIME, &ev, &timer);
  assert_int_equal(ret, 0);

  for (lc = 0; lc < 1; ++lc)
    {
      ret = timer_gettime(timer, &spec);
      assert_int_equal(ret, 0);

      ret = timer_gettime(timer, NULL);
      assert_int_equal(ret, -1);
      assert_int_equal(errno, EINVAL);
    }
}
