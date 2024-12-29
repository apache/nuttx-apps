/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_dup2_test.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTFILE "testDup2File1"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_dup201
 ****************************************************************************/

void test_nuttx_fs_dup201(FAR void **state)
{
  char buf[16] =
  {
    0
  };

  off_t currpos;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  int fd1 = open(TESTFILE, O_RDWR | O_CREAT, 0777);
  assert_true(fd1 > 0);
  test_state->fd1 = fd1;

  /* open file */

  int fd2 = open(TESTFILE, O_RDWR | O_CREAT, 0777);
  assert_true(fd2 > 0);
  test_state->fd2 = fd2;

  /* do dup2 */

  int ret = dup2(fd1, fd2);
  assert_int_not_equal(ret, -1);

  char *buf1 = "hello ";
  char *buf2 = "world!";

  /* do write */

  int ret2;
  ret2 = write(fd1, buf1, strlen(buf1));
  assert_int_in_range(ret2, 1, strlen(buf1));
  ret2 = write(fd2, buf2, strlen(buf2));
  assert_int_in_range(ret2, 1, strlen(buf2));

  /* refresh to storage */

  assert_int_equal(fsync(fd1), 0);

  /* reset file pos use fd2 */

  lseek(fd2, 0, SEEK_SET);

  /* check if file pos is shared */

  currpos = lseek(fd1, 0, SEEK_CUR);
  assert_int_equal(currpos, 0);

  /* read file */

  ret = read(fd1, buf, 12);
  assert_int_equal(ret, 12);

  /* check buf */

  ret = strncmp(buf, "hello world!", 12);
  assert_int_equal(ret, 0);
}
