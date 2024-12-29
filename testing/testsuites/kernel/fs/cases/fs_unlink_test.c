/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_unlink_test.c
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
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXSIZE 1024
#define test_file "test_unlink_file01"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_unlink01
 ****************************************************************************/

void test_nuttx_fs_unlink01(FAR void **state)
{
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  int fd = open(test_file, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  char buf[MAXSIZE] =
  {
    0
  };

  /* set memory */

  memset(buf, 65, MAXSIZE);

  /* do write */

  int size = write(fd, buf, MAXSIZE);
  assert_int_in_range(size, 1, MAXSIZE);

  close(fd);

  /* delete test file */

  int ret = unlink(test_file);
  assert_int_equal(ret, 0);

  /* check if the file was deleted successfully */

  fd = open(test_file, O_RDONLY);
  assert_int_equal(fd, -1);
}
