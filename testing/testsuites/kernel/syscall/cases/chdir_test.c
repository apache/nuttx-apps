/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/chdir_test.c
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
 * Name: test_nuttx_syscall_chdir01
 ****************************************************************************/

void test_nuttx_syscall_chdir01(FAR void **state)
{
  char testdir[64] =
  {
    0
  };

  int ret;

  sprintf(testdir, "%s_dir", __func__);
  rmdir(testdir);
  assert_int_equal(mkdir(testdir, S_IRWXU), 0);

  ret = chdir(testdir);
  assert_int_equal(ret, 0);
  rmdir(testdir);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_chdir02
 ****************************************************************************/

void test_nuttx_syscall_chdir02(FAR void **state)
{
  int ret;
  char dirs[64] =
  {
    0
  };

  getcwd(dirs, sizeof(dirs));

  /* Switch the directory 100 times */

  for (int lc = 0; lc < 100; lc++)
    {
      if (lc % 2 == 0)
        {
          ret = chdir("/");
          assert_int_equal(ret, 0);
        }

      else
        {
          ret = chdir(dirs);
          assert_int_equal(ret, 0);
        }
    }
}
