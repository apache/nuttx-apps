/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/nansleep_test.c
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
#include <time.h>
#include <syslog.h>
#include <errno.h>
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
 * Name: test_nuttx_syscall_nansleep01
 ****************************************************************************/

void test_nuttx_syscall_nansleep01(FAR void **state)
{
  time_t ret;
  struct timespec t =
  {
    .tv_sec = 2, .tv_nsec = 9999
  };

  ret = nanosleep(&t, NULL);
  assert_int_equal(ret, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_nansleep02
 ****************************************************************************/

void test_nuttx_syscall_nansleep02(FAR void **state)
{
  int ret;
  struct timespec tcases[] =
  {
      {.tv_sec = 0, .tv_nsec = (long)1000000000},
      {.tv_sec = 1, .tv_nsec = (long)-100},
  };

  int n = 0;
  for (n = 0; n < 2; n++)
    {
      ret = nanosleep(&tcases[n], NULL);
      if (ret != -1)
        {
          syslog(LOG_ERR,
                 "test no.%d,  FAIL, nanosleep() returned %d, expected "
                 "-1\n",
                 n + 1, ret);
          fail_msg("test fail !\n");
        }

      if (errno != EINVAL)
        {
          syslog(LOG_ERR,
                 "test no.%d,  FAIL, nanosleep() failed,expected "
                 "EINVAL, got\n",
                 n + 1);
          fail_msg("test fail !\n");
        }
    }
}
