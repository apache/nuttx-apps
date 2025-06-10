/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_file_get_test.c
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

#define TEST_FILE "fstat_test_file"
#define BUF_SIZE 512

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_file_get01
 ****************************************************************************/

void test_nuttx_fs_file_get01(FAR void **state)
{
  FAR struct file *filep;
  int ret;
  int fd;
  char *buf = NULL;
  FILE *fp;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* create a file for testing */

  fd = creat(TEST_FILE, 0700);
  assert_true(fd > 0);
  test_state->fd1 = fd;

  /* fdopen file */

  fp = fdopen(fd, "r+");
  assert_non_null(fp);

  /* get struct file */

  ret = file_get(fileno(fp), &filep);
  assert_int_equal(ret, 0);

  /* malloc memory */

  buf = malloc(BUF_SIZE);
  assert_non_null(buf);
  test_state->ptr = buf;

  /* set memory */

  memset(buf, 'A', BUF_SIZE);

  /* do write */

  ret = write(fileno(fp), buf, BUF_SIZE);
  assert_int_equal(ret, BUF_SIZE);

  /* do fflush */

  fflush(fp);

  /* do fsync */

  fsync(fileno(fp));

  /* put filep */

  file_put(filep);

  /* get struct file again */

  ret = file_get(fileno(fp), &filep);
  assert_int_equal(ret, 0);

  assert_int_equal(filep->f_pos, BUF_SIZE);

  test_state->fd2 = fileno(fp);

  /* put filep */

  file_put(filep);

  assert_int_equal(fclose(fp), 0);
}
