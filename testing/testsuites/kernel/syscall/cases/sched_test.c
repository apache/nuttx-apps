/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/sched_test.c
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
#include <dirent.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
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
 * Name: test_nuttx_syscall_sched01
 ****************************************************************************/

void test_nuttx_syscall_sched01(FAR void **state)
{
  struct test_case_t
  {
    int prio;
    int policy;
  }

  TC[] =
    {
      /* set scheduling policy to SCHED_RR */

        {
          1, SCHED_RR
        },

      /* set scheduling policy to SCHED_FIFO */

        {
          1, SCHED_FIFO
        }
    };

  int i;
  int rec;
  struct sched_param param;

  i = 0;
#if CONFIG_RR_INTERVAL > 0
  param.sched_priority = TC[i].prio;

  assert_int_not_equal(sched_setscheduler(0, TC[i].policy, &param), -1);

  rec = sched_getscheduler(0);
  assert_int_not_equal(rec, -1);
  assert_int_equal(rec, TC[i].policy);
#endif

  i = 1;
  param.sched_priority = TC[i].prio;
  assert_int_not_equal(sched_setscheduler(0, TC[i].policy, &param), -1);

  rec = sched_getscheduler(0);
  assert_int_not_equal(rec, -1);
  assert_int_equal(rec, TC[i].policy);
}

/****************************************************************************
 * Name: test_nuttx_syscall_sched02
 ****************************************************************************/

void test_nuttx_syscall_sched02(FAR void **state)
{
#if CONFIG_RR_INTERVAL > 0
  int ret;
  struct timespec tp;
  struct sched_param p =
  {
    1
  };

  /* Change scheduling policy to SCHED_RR */

  assert_int_not_equal(sched_setscheduler(0, SCHED_RR, &p), -1);
  ret = sched_rr_get_interval(0, &tp);
  assert_int_equal(ret, 0);
#else
  assert_true(1);
#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_sched03
 ****************************************************************************/

void test_nuttx_syscall_sched03(FAR void **state)
{
  struct timespec tp;
  struct sched_param p =
  {
    1
  };

  int ret;

  /* Change scheduling policy to SCHED_FIFO */

  assert_int_not_equal(sched_setscheduler(0, SCHED_FIFO, &p), -1);

  tp.tv_sec = 99;
  tp.tv_nsec = 99;

  ret = sched_rr_get_interval(0, &tp);

  assert_int_equal(ret, 0);
  assert_int_equal(tp.tv_sec, 0);
  assert_int_equal(tp.tv_nsec, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_sched04
 ****************************************************************************/

void test_nuttx_syscall_sched04(FAR void **state)
{
  struct timespec tp;

  static pid_t inval_pid = -1;

  int i;
  int ret;

  struct test_cases_t
  {
    pid_t *pid;
    struct timespec *tp;
    int exp_errno;
  }

  test_cases[] =
    {
        {
          &inval_pid, &tp, EINVAL
        }
    };

  int TST_TOTAL = sizeof(test_cases) / sizeof(test_cases[0]);

#if CONFIG_RR_INTERVAL > 0
  struct sched_param p =
  {
    1
  };

  /* Change scheduling policy to SCHED_RR */

  assert_int_not_equal(sched_setscheduler(0, SCHED_RR, &p), -1);
#endif
  for (i = 0; i < TST_TOTAL; ++i)
    {
      /* Call sched_rr_get_interval(2)
       */

      ret =
          sched_rr_get_interval(*(test_cases[i].pid), test_cases[i].tp);
      int ret_error = errno;

      assert_int_equal(ret, -1);
      assert_int_equal(ret_error, test_cases[i].exp_errno);
    }
}
