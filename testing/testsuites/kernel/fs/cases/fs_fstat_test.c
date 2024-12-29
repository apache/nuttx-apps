/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_fstat_test.c
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
#include <sys/socket.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE_1 "fstat_test_file1"
#define TEST_FILE_2 "fstat_test_file2"
#define BUF_SIZE 512

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fstat01
 ****************************************************************************/

void test_nuttx_fs_fstat01(FAR void **state)
{
  struct stat file_s;
  int fd;
  int ret;
  char *buf;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TEST_FILE_1, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* malloc memory */

  buf = malloc(BUF_SIZE);
  assert_non_null(buf);
  test_state->ptr = buf;

  /* set memory */

  memset(buf, 'A', BUF_SIZE);

  /* get the file size before write */

  ret = fstat(fd, &file_s);
  assert_int_equal(ret, 0);

  /* do write */

  ret = write(fd, buf, BUF_SIZE);
  assert_int_in_range(ret, 1, BUF_SIZE);

  /* get file size again */

  ret = fstat(fd, &file_s);
  assert_int_equal(ret, 0);

  /* check file_s.st_size */

  ret = (file_s.st_size == BUF_SIZE) ? 1 : 0;
  assert_int_equal(ret, 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fstat02
 ****************************************************************************/

void test_nuttx_fs_fstat02(FAR void **state)
{
  int fd;
  int ret;
  struct stat file_s;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TEST_FILE_2, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* get the file size before write */

  ret = fstat(fd, &file_s);
  assert_int_equal(ret, 0);

  /* close file */

  assert_int_equal(close(fd), 0);

  /* #if defined(CONFIG_NET)
   *    // creat socket
   *    fd = socket(AF_INET, SOCK_STREAM, 0);
   *    assert_int_not_equal(fd, -1);

   *    // get file size again
   *    ret = fstat(fd, &file_s);
   *    assert_int_equal(ret, 0);

   *    assert_true(S_ISSOCK(file_s.st_mode));
   *    assert_int_equal(close(fd), 0);
   * #endif
   */
}
