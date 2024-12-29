/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/clock_gettime_test.c
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
#include <string.h>
#include <stdio.h>
#include <time.h>
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
 * Name: test_nuttx_syscall_clockgettime01
 ****************************************************************************/

void test_nuttx_syscall_clockgettime01(FAR void **state)
{
  int ret;
  clockid_t type;
  struct test_case
  {
    clockid_t clktype;
    int allow_inval;
  }

  tc[] =
    {
        {
            CLOCK_REALTIME,
        },

        {
            CLOCK_MONOTONIC,
        },

        {
          CLOCK_BOOTTIME, 1
        },
    };

  struct timespec spec;

  for (int i = 0; i < 3; i++)
    {
      type = tc[i].clktype;
      switch (type)
        {
        case CLOCK_REALTIME:

          /* syslog(LOG_INFO, "clock_gettime test CLOCK_REALTIME\n"); */

          break;
        case CLOCK_MONOTONIC:

          /* syslog(LOG_INFO, "clock_gettime test CLOCK_MONOTONIC\n"); */

          break;
        case CLOCK_BOOTTIME:

          /* syslog(LOG_INFO, "clock_gettime test CLOCK_BOOTTIME\n"); */

          break;
        default:
          break;
        }

      ret = clock_gettime(type, &spec);
      assert_int_equal(ret, 0);
    }
}
