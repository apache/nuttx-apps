/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/getcwd_test.c
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
#include <syslog.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
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
 * Name: test_nuttx_syscall_getcwd01
 ****************************************************************************/

void test_nuttx_syscall_getcwd01(FAR void **state)
{
  char buffer[5] = "/";
  char *res;
  int ret;
  struct t_case
  {
    char *buf;
    size_t size;
    int exp_err;
  }

  tcases[] =
    {
        {
          buffer, 0, EINVAL
        },

        {
          buffer, 1, ERANGE
        },

        {
          NULL, 1, ERANGE
        },
    };

  for (int i = 0; i < 3; i++)
    {
      errno = 0;
      res = getcwd(tcases[i].buf, tcases[i].size);
      assert_null(res);
      ret = errno;
      assert_int_equal(ret, tcases[i].exp_err);
    }
}

/****************************************************************************
 * Name: test_nuttx_syscall_getcwd02
 ****************************************************************************/

void test_nuttx_syscall_getcwd02(FAR void **state)
{
  char *datadir = MOUNT_DIR;
  char exp_buf[PATH_MAX] = "";
  char buffer[PATH_MAX] = "";
  char *res = NULL;

  struct t_case
  {
    char *buf;
    size_t size;
  }

  tcases[] =
    {
        {
          NULL, 0
        },

        {
          NULL, 0
        },

        {
          NULL, PATH_MAX
        },
    };

  tcases[0].buf = buffer;
  tcases[0].size = sizeof(buffer);

  assert_int_equal(chdir(datadir), 0);
  assert_non_null(realpath(datadir, exp_buf));

  for (int i = 0; i < 3; i++)
    {
      errno = 0;
      res = getcwd(tcases[i].buf, tcases[i].size);

      assert_non_null(res);
      assert_string_equal(exp_buf, res);

      if (!tcases[i].buf)
        free(res);
    }
}
