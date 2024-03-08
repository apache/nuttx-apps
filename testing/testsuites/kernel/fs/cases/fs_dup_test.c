/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_dup_test.c
 * Copyright (C) 2020 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define TESTFILENAME "testDupFile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_dup01
 ****************************************************************************/

void test_nuttx_fs_dup01(FAR void **state)
{
  int fd;
  int newfd;
  int rval;
  char buf_fd[5] = "hello";
  char buf_new_fd[8] = "littleFS";
  char read_buf[20] = "";
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TESTFILENAME, O_RDWR | O_CREAT | O_APPEND, 0700);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* do write */

  assert_int_in_range(write(fd, buf_fd, sizeof(buf_fd)), 1, sizeof(buf_fd));

  /* refresh to storage */

  assert_int_equal(fsync(fd), 0);

  /* do dup */

  newfd = dup(fd);
  close(fd);
  assert_int_not_equal(newfd, -1);
  test_state->fd2 = newfd;

  /* check if file pos is shared */

  off_t currpos = lseek(newfd, 0, SEEK_CUR);
  assert_int_equal(currpos, 5);

  /* write newfd after dup */

  rval = write(newfd, buf_new_fd, sizeof(buf_new_fd));
  assert_int_in_range(rval, 1, sizeof(buf_new_fd));

  /* refresh to storage */

  assert_int_equal(fsync(newfd), 0);

  /* reset file pos use newfd */

  off_t ret = lseek(newfd, 0, SEEK_SET);
  assert_int_equal(ret, 0);

  /* do double check */

  rval = read(newfd, read_buf, 20);
  assert_int_in_range(rval, 1, 20);

  /* check read_buf */

  assert_int_equal(strncmp(read_buf, "hellolittleFS", 13), 0);
}
