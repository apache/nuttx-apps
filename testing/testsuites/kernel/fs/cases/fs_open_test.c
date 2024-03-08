/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_open_test.c
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
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTFILE "testOpenFile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_open01
 ****************************************************************************/

void test_nuttx_fs_open01(FAR void **state)
{
  int fd;
  int ret;
  char s[] = "test data!";
  char buffer[50] = {
      0
    };

  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  fd = open(TESTFILE, O_WRONLY | O_CREAT, 0700);
  assert_true(fd > 0);

  /* do write */

  int ret2 = write(fd, s, sizeof(s));
  close(fd);
  assert_int_in_range(ret2, 1, sizeof(s));

  /* open file */

  fd = open(TESTFILE, O_RDONLY);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* do read */

  ret = read(fd, buffer, sizeof(buffer));
  assert_true(ret > 0);
}
