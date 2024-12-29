/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/write_test.c
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
#include <inttypes.h>
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
 * Name: test_nuttx_syscall_write01
 ****************************************************************************/

void test_nuttx_syscall_write01(FAR void **state)
{
  ssize_t ret;
  char filename[64];
  int fd;
  int i;
  int badcount = 0;
  char buf[BUFSIZ];

  sprintf(filename, "%s_file", __func__);

  /* setup */

  fd = open(filename, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  /* do test */

  assert_non_null(memset(buf, 'w', BUFSIZ));

  assert_int_equal(lseek(fd, 0, SEEK_SET), 0);

  for (i = BUFSIZ; i > 0; i--)
    {
      ret = write(fd, buf, i);
      if (ret == -1)
        {
          fail_msg("FAIL, write failed !\n");
          break;
        }

      if (ret != i)
        {
          badcount++;
          syslog(LOG_INFO,
                 "INFO, write() returned %" PRId64 ", expected %d\n",
                 (int64_t)ret, i);
        }
    }

  if (badcount != 0)
    {
      fail_msg("FAIL, write() failed to return proper count\n");
    }

  if (fd > 0)
    {
      assert_int_equal(close(fd), 0);
    }

  assert_int_equal(unlink(filename), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_write02
 ****************************************************************************/

void test_nuttx_syscall_write02(FAR void **state)
{
  int fd;
  int ret;
  char filename[64];
  sprintf(filename, "%s_file", __func__);
  fd = open(filename, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  ret = write(fd, NULL, 3);
  assert_true(ret < 0);

  if (fd > 0)
    {
      assert_int_equal(close(fd), 0);
    }

  assert_int_equal(unlink(filename), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_write03
 ****************************************************************************/

void test_nuttx_syscall_write03(FAR void **state)
{
  int fd;
  ssize_t ret;
  char filename[64];
  int i;
  int badcount = 0;
  char buf[100];

  sprintf(filename, "%s_file.wav", __func__);

  /* setup */

  fd = open(filename, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  /* do test */

  assert_non_null(memset(buf, 'w', 100));

  assert_int_equal(lseek(fd, 0, SEEK_SET), 0);

  for (i = 100; i > 0; i--)
    {
      ret = write(fd, buf, i);
      if (ret == -1)
        {
          fail_msg("FAIL, write failed !\n");
          break;
        }

      if (ret != i)
        {
          badcount++;
          syslog(LOG_INFO,
                 "INFO, write() returned %" PRId64 ", expected %d\n",
                 (int64_t)ret, i);
        }
    }

  if (badcount != 0)
    {
      fail_msg("FAIL, write() failed to return proper count\n");
    }

  if (fd > 0)
    {
      assert_int_equal(close(fd), 0);
    }

  assert_int_equal(unlink(filename), 0);
}
