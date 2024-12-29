/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/creat_test.c
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
 * Name: test_nuttx_syscall_creat01
 ****************************************************************************/

void test_nuttx_syscall_creat01(FAR void **state)
{
  char filename[50] = "";
  int fd;
  int res;
  struct tcase
  {
    int mode;
  }

  tcases[] =
    {
        {
          0644
        },

        {
          0444
        },
    };

  sprintf(filename, "Creat01%d", gettid());

  struct stat buf;
  char c;
  char w_buf[2] = "A";

  for (int i = 0; i < 2; i++)
    {
      fd = creat(filename, tcases[0].mode);
      if (fd > 0)
        {
          res = stat(filename, &buf);
          assert_int_equal(res, 0);
          assert_int_equal(buf.st_size, 0);
          assert_int_equal(write(fd, w_buf, 1), 1);
          assert_int_equal(read(fd, &c, 1), -1);
          close(fd);
        }
    }

  res = unlink(filename);
  assert_int_equal(res, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_creat02
 ****************************************************************************/

void test_nuttx_syscall_creat02(FAR void **state)
{
  char filename[50] = "";
  int fd;
  int res;
  struct stat statbuf;

  sprintf(filename, "Creat02%d", gettid());

  fd = creat(filename, 444);
  if (fd > 0)
    {
      res = fstat(fd, &statbuf);
      assert_int_equal(res, 0);
      assert_int_equal((statbuf.st_mode & S_ISVTX), 0);

      close(fd);
      res = unlink(filename);
      assert_int_equal(res, 0);
    }
  else
    {
      fail_msg("open test file fail !\n");
    }
}
