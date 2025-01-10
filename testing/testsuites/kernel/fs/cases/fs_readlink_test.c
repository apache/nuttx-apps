/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_readlink_test.c
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE "readlink_test_file"
#define PATH_MAX_SIZE 64

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_readlink01
 ****************************************************************************/

void test_nuttx_fs_readlink01(FAR void **state)
{
  int ret;
  int fd;

  /* test symlink */

  char path[PATH_MAX_SIZE] =
  {
    0
  };

  char buf[PATH_MAX_SIZE] =
  {
    0
  };

  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* creat file */

  fd = creat(TEST_FILE, 0700);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  getcwd(path, sizeof(path));
  strcat(path, "/");
  strcat(path, TEST_FILE);

  /* creating a soft connection */

  ret = symlink(path, "/file_link");
  assert_int_equal(ret, 0);

  /* read link */

  ret = readlink("/file_link", buf, PATH_MAX_SIZE);
  assert_true(ret == strlen(path));

  /* delete test file */

  assert_int_equal(unlink("/file_link"), 0);
}
