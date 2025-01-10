/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/dup2_test.c
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
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_dup201
 ****************************************************************************/

void test_nuttx_syscall_dup201(FAR void **state)
{
#ifndef CONFIG_FDSAN
  int ret;
  int goodfd = 5;
  int badfd = -1;
  int mystdout = 0;

  struct test_case_t
  {
    int *ofd;
    int *nfd;
    int error;
  }

  TC[] =
    {
      /* First fd argument is less than 0 - EBADF */

        {
          NULL, NULL, EBADF
        },

      /* Second fd argument is less than 0 - EBADF */

        {
          NULL, NULL, EBADF
        },
    };

  TC[0].ofd = &badfd;
  TC[0].nfd = &goodfd;

  TC[1].ofd = &mystdout;
  TC[1].nfd = &badfd;

  for (int i = 0; i < 2; i++)
    {
      ret = dup2(*TC[i].ofd, *TC[i].nfd);
      assert_int_equal(ret, -1);
      assert_int_equal(errno, TC[i].error);
    }

#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_dup202
 ****************************************************************************/

void test_nuttx_syscall_dup202(FAR void **state)
{
  char testfile[64] = "";
  int ret;
  int i;
  int ofd;
  int duprdo = 10;
  int dupwro = 20;
  int duprdwr = 30;
  struct stat oldbuf;
  struct stat newbuf;

  struct test_case_t
  {
    int *nfd;
    mode_t mode;
  }

  TC[] =
    {
        {
          NULL, (S_IRUSR | S_IRGRP | S_IROTH)
        },

        {
          NULL, (S_IWUSR | S_IWGRP | S_IWOTH)
        },

        {
          NULL, (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH)
        },
    };

  TC[0].nfd = &duprdo;
  TC[1].nfd = &dupwro;
  TC[2].nfd = &duprdwr;

  sprintf(testfile, "%s_file", __func__);

  for (i = 0; i < 3; i++)
    {
      ofd = creat(testfile, TC[i].mode);
      assert_true(ofd > 0);

      ret = dup2(ofd, *TC[i].nfd);
      assert_int_not_equal(ret, -1);
      if (ret == -1)
        {
          continue;
        }

      assert_int_equal(fstat(ofd, &oldbuf), 0);
      assert_int_equal(fstat(*(TC[i].nfd), &newbuf), 0);
      assert_int_equal(oldbuf.st_mode, newbuf.st_mode);

      close(*TC[i].nfd);
      close(ofd);
      close(ret);

      assert_int_equal(unlink(testfile), 0);
    }
}
