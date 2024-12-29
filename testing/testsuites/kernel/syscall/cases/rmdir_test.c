/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/rmdir_test.c
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
#include <sys/types.h>
#include <sys/stat.h>
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
 * Name: test_nuttx_syscall_rmdir01
 ****************************************************************************/

void test_nuttx_syscall_rmdir01(FAR void **state)
{
  int ret;
  struct stat buf;
  char testdir[64] = "";

  sprintf(testdir, "%s_dir", __func__);

  ret = mkdir(testdir, 0777);
  assert_int_equal(ret, 0);

  ret = rmdir(testdir);
  assert_int_equal(ret, 0);

  assert_int_not_equal(stat(testdir, &buf), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_rmdir02
 ****************************************************************************/

void test_nuttx_syscall_rmdir02(FAR void **state)
{
  int ret;
  int fd;

  /* Description:
   *   1) attempt to rmdir() non-empty directory -> ENOTEMPTY
   *   2) attempt to rmdir() non-existing directory -> ENOENT
   *   3) attempt to rmdir() a file -> ENOTDIR
   */

  struct testcase
  {
    char dir[32];
    int exp_errno;
  }

  tcases[] =
    {
        {
          "Rmdir02_testdir", ENOTEMPTY
        },

        {
          "nosuchdir/testdir2", ENOENT
        },

        {
          "Rmdir02_testfile2", ENOTDIR
        },
    };

  ret = mkdir("Rmdir02_testdir", (S_IRWXU | S_IRWXG | S_IRWXO));
  assert_int_equal(ret, 0);
  ret = mkdir("Rmdir02_testdir/test1", (S_IRWXU | S_IRWXG | S_IRWXO));
  assert_int_equal(ret, 0);
  fd = open("Rmdir02_testfile2", O_CREAT | O_RDWR);

  if (fd > 0)
    close(fd);
  for (int i = 0; i < 3; i++)
    {
      ret = rmdir(tcases[i].dir);
      assert_int_not_equal(ret, 0);
      if (ret != -1)
        {
          continue;
        }
    }

  assert_int_equal(rmdir("Rmdir02_testdir/test1"), 0);
  assert_int_equal(rmdir("Rmdir02_testdir"), 0);
  assert_int_equal(unlink("Rmdir02_testfile2"), 0);
}
