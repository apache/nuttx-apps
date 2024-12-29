/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/fpathconf_test.c
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
 * Name: test_nuttx_syscall_fpathconf01
 ****************************************************************************/

void test_nuttx_syscall_fpathconf01(FAR void **state)
{
  int fd;
  int i;
  int ret;
  char filename[64] = "";

  struct pathconf_args
  {
    char name[16];
    int value;
  }

  test_cases[] =
    {
        {
          "_PC_MAX_CANON", _PC_MAX_CANON
        },

        {
          "_PC_MAX_INPUT", _PC_MAX_INPUT
        },

        {
          "_PC_LINK_MAX", _PC_LINK_MAX
        },

        {
          "_PC_NAME_MAX", _PC_NAME_MAX
        },

        {
          "_PC_PATH_MAX", _PC_PATH_MAX
        },

        {
          "_PC_PIPE_BUF", _PC_PIPE_BUF
        },
    };

  /* setup */

  sprintf(filename, "%s_file", __func__);
  fd = open(filename, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  for (i = 0; i < 6; i++)
    {
      ret = fpathconf(fd, test_cases[i].value);
      assert_int_not_equal(ret, -1);
    }

  if (fd > 0)
    {
      assert_int_equal(close(fd), 0);
    }

  assert_int_equal(unlink(filename), 0);
}
