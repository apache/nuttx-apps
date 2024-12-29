/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_rename_test.c
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_rename01
 ****************************************************************************/

void test_nuttx_fs_rename01(FAR void **state)
{
  int fd;
  int status;
  int ret;
  char buffer[50];
  char filename1[] = "testRenameFile1";

  /* open file */

  fd = open(filename1, O_WRONLY | O_CREAT, 0700);
  assert_true(fd > 0);

  /* set memory */

  memset(buffer, '*', 50);

  /* do write */

  ret = write(fd, buffer, 50);
  assert_int_in_range(ret, 1, 50);

  /* close file befor rename */

  close(fd);

  /* do rename */

  status = rename(filename1, "newNameFile1");
  assert_int_equal(status, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_rename02
 ****************************************************************************/

void test_nuttx_fs_rename02(FAR void **state)
{
  int status;

  /* make directory */

  status = mkdir("testdir1", 0700);
  assert_int_equal(status, 0);

  /* rename directory */

  status = rename("testdir1", "newtestdir1");
  assert_int_equal(status, 0);
}
