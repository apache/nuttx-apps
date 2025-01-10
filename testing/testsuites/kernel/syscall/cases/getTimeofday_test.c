/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/getTimeofday_test.c
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
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int done;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: breakout
 ****************************************************************************/

static void breakout(int sig)
{
  done = sig;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_gettimeofday01
 ****************************************************************************/

void test_nuttx_syscall_gettimeofday01(FAR void **state)
{
  int rtime = 3;
  struct timeval tv1;
  struct timeval tv2;
  unsigned long long cnt = 0;

  signal(SIGALRM, breakout);

  done = 0;

  alarm(rtime);

  if (gettimeofday(&tv1, NULL))
    fail_msg("FAIL, gettimeofday() failed\n");

  while (!done)
    {
      if (gettimeofday(&tv2, NULL))
        fail_msg("FAIL, gettimeofday() failed\n");

      if (tv2.tv_sec < tv1.tv_sec ||
          (tv2.tv_sec == tv1.tv_sec && tv2.tv_usec < tv1.tv_usec))
        {
          fail_msg("test fail !");
        }

      sleep(1);
      tv1 = tv2;
      cnt++;
    }
}
