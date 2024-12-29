/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/read_test.c
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
#include <errno.h>
#include <syslog.h>
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
 * Name: test_nuttx_syscall_read01
 ****************************************************************************/

void test_nuttx_syscall_read01(FAR void **state)
{
  int fd;
  char buf[512];
  char filename[64] = "";

  memset(buf, '*', 512);
  sprintf(filename, "%s_file", __func__);

  fd = open(filename, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);
  assert_int_equal(write(fd, buf, 512), (ssize_t)512);

  lseek(fd, 0, SEEK_SET);

  assert_int_equal(read(fd, buf, 512), (ssize_t)512);

  if (fd > 0)
    assert_int_equal(close(fd), 0);

  assert_int_equal(unlink(filename), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_read02
 ****************************************************************************/

void test_nuttx_syscall_read02(FAR void **state)
{
#ifndef CONFIG_FDSAN

  /* Define an invalid file descriptor */

  int badfd = 99999999;
  int ret;
  char buf[64];
  size_t count = 1;

  /* Read with an invalid file descriptor */

  ret = read(badfd, buf, count);

  /* Read fail */

  assert_int_equal(ret, -1);

  /* Check whether the error code meets the expectation */

  assert_int_equal(errno, EBADF);
#endif
}

/****************************************************************************
 * Name: test_nuttx_syscall_read03
 ****************************************************************************/

void test_nuttx_syscall_read03(FAR void **state)
{
  char fname[255];
  char palfa[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int fild;

  int lc;
  int ret;
  int rfild;
  char prbuf[BUFSIZ];

  sprintf(fname, "%s_tfile", __func__);

  fild = creat(fname, 0777);
  if (fild > 0)
    {
      assert_int_equal(write(fild, palfa, 27), 27);
      close(fild);
    }

  for (lc = 0; lc < 2; lc++)
    {
      if ((rfild = open(fname, O_RDONLY)) == -1)
        {
          syslog(LOG_ERR, "can't open for reading\n");
          continue;
        }

      ret = read(rfild, prbuf, BUFSIZ);

      if (ret == -1)
        {
          fail_msg("test fail !\n");
          syslog(LOG_ERR, "FAIL, call failed unexpectedly\n");
          close(rfild);
          continue;
        }

      if (ret != 27)
        {
          fail_msg("test fail !\n");
          syslog(LOG_ERR,
                 "FAIL, Bad read count - got %d - "
                 "expected %d\n",
                 ret, 27);
          close(rfild);
          continue;
        }

      if (memcmp(palfa, prbuf, 27) != 0)
        {
          fail_msg("test fail !\n");
          syslog(LOG_ERR,
                 "FAIL, read buffer not equal "
                 "to write buffer\n");
          close(rfild);
          continue;
        }

      close(rfild);
    }

  assert_int_equal(unlink(fname), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_read04
 ****************************************************************************/

void test_nuttx_syscall_read04(FAR void **state)
{
  int ret;

  /* Define an invalid file descriptor */

  int badfd = -1;
  char buf[64];
  size_t count = 1;

  /* Read with an invalid file descriptor */

  ret = read(badfd, buf, count);

  /* Read succeeded */

  if (ret != -1)
    {
      fail_msg("FAIL, read() succeeded unexpectedly");
    }

  /* Check whether the error code meets the expectation */

  if (errno != EBADF)
    {
      syslog(LOG_ERR,
             "FAIL, read() failed unexpectedly, expected "
             "EBADF:%d,actually:%d\n",
             EBADF, errno);
      fail_msg("read() failed unexpectedly");
    }
}
