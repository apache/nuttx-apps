/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_sendfile_test.c
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
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <errno.h>
#include <cmocka.h>
#include "fstest.h"

#define O_FILE "outputFile"
#define I_FILE1 "inputFile1"
#define I_FILE2 "inputFile2"

static void set_test(void)
{
  int fd;
  fd = open(O_FILE, O_CREAT | O_RDWR, 0777);
  if (fd == -1)
    {
      syslog(LOG_ERR, "Unable to open file %s, errno %d\n", O_FILE,
             errno);
      assert_true(1 == 0);
    }
  write(fd, "ABCDEFGHIJ", 10);
  close(fd);
}

/****************************************************************************
 * Name: sendfile
 * Example description:
 *  1.Test copy the entire file
 * Expect results: TEST PASSED
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void test_nuttx_fs_sendfile01(FAR void **state)
{
  int ret;
  int fd_o1;
  int fd_o2;
  int fd_i;
  int test_flag = 0;
  struct stat f_stat;
  set_test();

  /* open file readonly */

  fd_i = open(O_FILE, O_RDONLY);
  if (fd_i == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  /* open file O_RDWR */

  fd_o1 = open(I_FILE1, O_CREAT | O_RDWR, 0777);
  if (fd_o1 == -1)
    {
      close(fd_i);
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  if (fstat(fd_i, &f_stat) < 0)
    {
      syslog(LOG_ERR, "fstat fail !, errno %d\n", errno);
      close(fd_i);
      close(fd_o1);
      assert_true(1 == 0);
    }

  /* sendfile , copy the entire file */

  ret = sendfile(fd_o1, fd_i, NULL, f_stat.st_size);

  if (ret != f_stat.st_size)
    {
      syslog(LOG_ERR, "ret != f_stat.st_size\n");
      test_flag = 1;
    }

  lseek(fd_i, 0, SEEK_SET);

  fd_o2 = open(I_FILE2, O_CREAT | O_RDWR, 0777);
  if (fd_o2 == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      close(fd_i);
      close(fd_o1);
      assert_true(1 == 0);
    }

  /* sendfile , Copy part of the file */

  ret = sendfile(fd_o2, fd_i, NULL, 5);

  if (ret != 5)
    {
      syslog(LOG_ERR, "ret != 5\n");
      test_flag = 1;
    }

  close(fd_o1);
  close(fd_o2);
  close(fd_i);
  assert_true(test_flag == 0);
}

/****************************************************************************
 * Name: sendfile
 * Example description:
 *  1.Test copy the entire file
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_sendfile02(FAR void **state)
{
  int fd_o;
  int fd_i;
  off_t offset;
  struct stat f_stat;
  set_test();

  /* open file readonly */

  fd_i = open(O_FILE, O_RDONLY);
  if (fd_i == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  /* open file O_RDWR */

  fd_o = open(I_FILE1, O_CREAT | O_RDWR, 0777);
  if (fd_o == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      close(fd_i);
      assert_true(1 == 0);
    }

  if (fstat(fd_i, &f_stat) < 0)
    {
      syslog(LOG_ERR, "fstat fail ! errno %d\n", errno);
      close(fd_i);
      close(fd_o);
      assert_true(1 == 0);
    }

  offset = 5;

  /* sendfile */

  sendfile(fd_o, fd_i, &offset, f_stat.st_size - offset);

  close(fd_o);
  close(fd_i);

  /* Checks whether the file pointer is offset to the specified position
   */

  assert_int_equal(offset, f_stat.st_size);
}
