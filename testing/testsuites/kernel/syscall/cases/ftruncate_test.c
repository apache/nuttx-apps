/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/ftruncate_test.c
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: checkandreportftruncatetest
 ****************************************************************************/

static int checkandreportftruncatetest(int fd, off_t offset, char data,
                                       off_t trunc_len)
{
  int i;
  int file_length;
  int ret;
  char buf[1024];
  struct stat stat_buf;
  off_t ret_offset;
  ssize_t rval;

  memset(buf, '*', sizeof(buf));
  ret = fstat(fd, &stat_buf);
  assert_int_equal(ret, 0);
  file_length = stat_buf.st_size;
  if (file_length != trunc_len)
    {
      syslog(LOG_ERR, "FAIL, ftruncate() got incorrected size: %d\n",
             file_length);
      return -1;
    }

  ret_offset = lseek(fd, offset, SEEK_SET);
  assert_int_in_range(ret_offset, 0, offset);
  rval = read(fd, buf, sizeof(buf));
  assert_int_in_range(rval, 0, 1024);

  for (i = 0; i < 256; i++)
    {
      if (buf[i] != data)
        {
          syslog(
              LOG_ERR,
              "FAIL, ftruncate() got incorrect data %d, expected %d\n",
              buf[i], data);
          return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_ftruncate01
 ****************************************************************************/

void test_nuttx_syscall_ftruncate01(FAR void **state)
{
  int fd;
  int ret;
  char fname[256] = "";

  sprintf(fname, "Ftruncate01_%d", gettid());

  assert_int_equal(cmtestfillfile(fname, 'a', 1024, 1), 0);

  fd = open(fname, O_RDWR);

  /* ftruncate to 256 */

  ret = ftruncate(fd, 256);
  assert_int_equal(ret, 0);

  ret = checkandreportftruncatetest(fd, 0, 'a', 256);
  assert_int_equal(ret, 0);

  ret = ftruncate(fd, 512);
  assert_int_equal(ret, 0);

  ret = checkandreportftruncatetest(fd, 256, 0, 512);
  assert_int_equal(ret, 0);

  assert_int_equal(close(fd), 0);
  assert_int_equal(unlink(fname), 0);
}
