/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/timer_delete_test.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 *The ASF licenses this file to you under the Apache License, Version 2.0
 *(the "License"); you may not use this file except in compliance with
 *the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *implied.  See the License for the specific language governing
 *permissions and limitations under the License.
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
 * Name: test_nuttx_syscall_timerdelete01
 ****************************************************************************/

void test_nuttx_syscall_timerdelete01(FAR void **state)
{
  int ret;
  timer_t timer_id;
  clock_t clock_list[] =
    {
        CLOCK_REALTIME,
        CLOCK_MONOTONIC,
        CLOCK_BOOTTIME,
    };

  for (int i = 0; i < 3; i++)
    {
      ret = timer_create(clock_list[i], NULL, &timer_id);
      assert_int_equal(ret, 0);

      ret = timer_delete(timer_id);
      assert_int_equal(ret, 0);
    }
}
