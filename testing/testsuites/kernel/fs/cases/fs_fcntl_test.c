/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_fcntl_test.c
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
#include <fcntl.h>
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define BUFSIZE 512
#define TEST_FILE_1 "fcntl01_testfile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fcntl01
 ****************************************************************************/

void test_nuttx_fs_fcntl01(FAR void **state)
{
  char *buf = NULL;
  int oldfd;
  int newfd;
  int ret;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  oldfd = open(TEST_FILE_1, O_CREAT | O_RDWR, 0700);
  assert_true(oldfd > 0);
  test_state->fd1 = oldfd;

  /* do fcntl */

  newfd = fcntl(oldfd, F_DUPFD, 0);
  assert_true(newfd > 0);
  test_state->fd2 = newfd;

  /* malloc memory */

  buf = (char *)malloc(BUFSIZ);
  assert_non_null(buf);
  test_state->ptr = buf;

  /* set memory */

  memset(buf, 'A', BUFSIZ);

  /* do write */

  ret = write(newfd, buf, BUFSIZ);
  assert_int_in_range(ret, 1, BUFSIZ);
}

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE_2 "fcntl02_testfile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fcntl02
 ****************************************************************************/

void test_nuttx_fs_fcntl02(FAR void **state)
{
  int fd;
  int ret;
  int v;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TEST_FILE_2, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  /* do fcntl */

  v = fcntl(fd, F_GETFD);
  assert_int_in_range(v, 0, 255);
  test_state->fd1 = fd;

  v |= FD_CLOEXEC;

  /* do fcntl */

  ret = fcntl(fd, F_SETFD, v);
  assert_int_in_range(ret, 0, 255);
  test_state->fd2 = ret;
  ret = (v == fcntl(fd, F_GETFD) ? 1 : 0);
  assert_int_equal(ret, 1);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fsfcntlsetlilestatus
 ****************************************************************************/

static int fsfcntlsetlilestatus(int mode, int fd)
{
  int flags;
  int ret;
  flags = fcntl(fd, F_GETFL);
  if (flags == -1)
    {
      syslog(LOG_ERR, "get file status fail !\n");
      return -1;
    }

  flags |= mode;
  ret = fcntl(fd, F_SETFL, flags);
  if (ret < 0)
    {
      syslog(LOG_ERR, "set file status fail !\n");
    }

  return 0;
}

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE_3 "fcntl03_testfile_1"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fcntl03
 ****************************************************************************/

void test_nuttx_fs_fcntl03(FAR void **state)
{
  int fd;
  int ret;
  int size;
  int ret2;
  char path[32] =
  {
    0
  };

  char buf[10] =
  {
    0
  };

  getcwd(path, sizeof(path));

  /* open file */

  fd = open(TEST_FILE_3, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);

  /* set memory */

  memset(buf, 'A', 10);

  /* do write */

  ret2 = write(fd, buf, 10);
  assert_int_in_range(ret2, 1, 10);

  /* close file */

  assert_int_equal(close(fd), 0);

  sleep(1);

  /* open the file again */

  fd = open(TEST_FILE_3, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);

  /* F_SETFL */

  ret = fsfcntlsetlilestatus(O_APPEND, fd);
  assert_int_equal(ret, 0);

  /* set memory */

  memset(buf, 'B', 10);

  /* do write */

  ret2 = write(fd, buf, 10);
  assert_int_in_range(ret2, 1, 10);

  /* get fd size */

  size = lseek(fd, 0, SEEK_END);

  /* close file */

  close(fd);

  /* check size */

  assert_int_equal(size, 20);
}
