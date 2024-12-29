/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/pathconf_test.c
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
#include <stdlib.h>
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
 * Name: test_nuttx_syscall_pathconf01
 ****************************************************************************/

void test_nuttx_syscall_pathconf01(FAR void **state)
{
  long ret;
  char *path;

  struct pathconf_args
  {
    char define_tag[16];
    int value;
  }

  args[] =
    {
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

  path = strdup(MOUNT_DIR);
  assert_non_null(path);

  for (int i = 0; i < 4; i++)
    {
      ret = pathconf(path, args[i].value);
      assert_int_not_equal(ret, -1);
    }

  free(path);
}
