/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/timer_create_test.c
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
 * Name: test_nuttx_syscall_timercreate01
 ****************************************************************************/

void test_nuttx_syscall_timercreate01(FAR void **state)
{
  int ret;
  struct sigevent evp;
  clock_t clock = CLOCK_MONOTONIC;
  timer_t created_timer_id;

  memset(&evp, 0, sizeof(evp));

  evp.sigev_signo = SIGALRM;
  evp.sigev_notify = SIGEV_SIGNAL;

  ret = timer_create(clock, &evp, &created_timer_id);
  assert_int_equal(ret, 0);

  ret = timer_delete(created_timer_id);
  assert_int_equal(ret, 0);
}
