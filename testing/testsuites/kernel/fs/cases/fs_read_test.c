/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_read_test.c
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
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
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

#define TESTFILE "testRead01File1"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_read01
 ****************************************************************************/

void test_nuttx_fs_read01(FAR void **state)
{
  int fd;
  int size;
  char s[] = "Test!";
  char buffer[80];
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  fd = open(TESTFILE, O_WRONLY | O_CREAT, 0777);
  assert_int_in_range(fd, 0, 255);
  test_state->fd1 = fd;

  size = write(fd, s, sizeof(s));
  assert_int_equal(size, sizeof(s));

  close(fd);
  fd = open(TESTFILE, O_RDONLY, 0777);
  assert_int_in_range(fd, 0, 255);
  test_state->fd1 = fd;
  size = read(fd, buffer, sizeof(buffer));
  assert_int_equal(size, sizeof(s));
}
