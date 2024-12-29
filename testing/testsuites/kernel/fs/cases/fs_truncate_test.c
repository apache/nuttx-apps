/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_truncate_test.c
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
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTFILE "truncateTestFile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_truncate01
 ****************************************************************************/

void test_nuttx_fs_truncate01(FAR void **state)
{
  int fd;
  int ret;
  int ret2;
  char buf[80];
  const char *s1 = "0123456789";
  const char *s2 = "abcde";
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TESTFILE, O_CREAT | O_WRONLY | O_TRUNC, 0700);
#ifdef CONFIG_FDCHECK
  assert_true(fd > 0);
#else
  assert_int_in_range(fd, 0, 512);
#endif
  test_state->fd1 = fd;

  /* do write */

  ret2 = write(fd, s1, strlen(s1));
  assert_int_in_range(ret2, 1, strlen(s1));

  /* refresh to storage */

  ret = fsync(fd);
  assert_int_equal(ret, 0);

  /* do ftruncate */

  ret = ftruncate(fd, 5);
  assert_int_equal(ret, 0);

  /* do lseek */

  off_t pos = lseek(fd, 0, SEEK_END);
  assert_int_equal(pos, 5);

  /* do write */

  ret2 = write(fd, s2, strlen(s2));
  assert_int_in_range(ret2, 1, strlen(s2));

  close(fd);

  /* open file */

  fd = open(TESTFILE, O_RDONLY);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* read file */

  ret = read(fd, buf, 26);
  if (ret >= 0)
    {
      buf[ret] = 0;
    }

  /* check buf */

  assert_true(strncmp(buf, "01234abcde", 10) == 0);
  close(fd);
}
