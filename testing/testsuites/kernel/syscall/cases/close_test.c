/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/close_test.c
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
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
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
 * Name: test_nuttx_syscall_close01
 ****************************************************************************/

void test_nuttx_syscall_close01(FAR void **state)
{
  int fd;
  int newfd;
  int pipefd[2] =
  {
    0
  };

  char filename[128] =
  {
    0
  };

  int ret;

  sprintf(filename, "%s_dir", __func__);

  fd = creat(filename, 0777);

  newfd = dup(fd);

  ret = pipe(pipefd);
  assert_int_equal(ret, 0);

  for (int i = 0; i < 2; i++)
    {
      if (pipefd[i] > 0)
        {
          ret = close(pipefd[i]);
          assert_int_equal(ret, 0);
        }
    }

  if (fd > 0)
    close(fd);
  if (newfd > 0)
    close(newfd);
  remove(filename);
}

/****************************************************************************
 * Name: test_nuttx_syscall_close02
 ****************************************************************************/

void test_nuttx_syscall_close02(FAR void **state)
{
#ifndef CONFIG_FDSAN
  int ret;
  int badfd1 = -1;
  int badfd2 = 99999999;

  ret = close(badfd1);
  assert_int_equal(ret, ERROR);

  /* Check that an invalid file descriptor returns EBADF
   */

  assert_int_equal(errno, EBADF);

  ret = close(badfd2);
  assert_int_equal(ret, ERROR);

  /* Check that an invalid file descriptor returns EBADF
   */

  assert_int_equal(errno, EBADF);
#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_close03
 ****************************************************************************/

void test_nuttx_syscall_close03(FAR void **state)
{
  char filename[128] =
  {
    0
  };

  int fd;
  int ret;

  for (int i = 0; i < 100; i++)
    {
      memset(filename, 0, sizeof(filename));
      sprintf(filename, "%s_dir%d", __func__, i + 1);

      fd = open(filename, O_RDWR | O_CREAT, 0700);
      if (fd < 0)
        {
          fail_msg("open test file fail !\n");
        }

      ret = close(fd);
      assert_int_equal(ret, 0);

      unlink(filename);
    }
}
