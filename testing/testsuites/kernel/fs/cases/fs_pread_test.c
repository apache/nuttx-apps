/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_pread_test.c
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
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE "pread_file"
#define BUF_SIZE 4

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_pread01
 ****************************************************************************/

void test_nuttx_fs_pread01(FAR void **state)
{
  int fd;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* malloc memory */

  char *buf = malloc(BUF_SIZE * sizeof(char));
  assert_non_null(buf);
  test_state->ptr = buf;

  /* open file */

  fd = open(TEST_FILE, O_RDWR | O_CREAT, 0777);
  assert_int_not_equal(fd, -1);
  test_state->fd1 = fd;

  /* do write */

  int ret = write(fd, "ABCD", BUF_SIZE);
  assert_int_in_range(ret, 1, 4);

  /* reset file pos use fd */

  lseek(fd, 0, SEEK_SET);

  /* set memory */

  memset(buf, '\0', BUF_SIZE);

  /* do pread */

  pread(fd, buf, 2, 2);

  assert_int_equal(strncmp(buf, "CD", 2), 0);

  /* set memory */

  memset(buf, '\0', BUF_SIZE);

  /* the pread do not change file pointer */

  pread(fd, buf, 3, 1);
  assert_int_equal(strncmp(buf, "BCD", 3), 0);
}
