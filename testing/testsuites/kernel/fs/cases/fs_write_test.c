/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_write_test.c
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

#define TESTFILE "testWriteFile"
#define MAXLEN 1024

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_write01
 ****************************************************************************/

void test_nuttx_fs_write01(FAR void **state)
{
  int out;
  int rval;
  char buffer[1024];
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* open file */

  out = open(TESTFILE, O_WRONLY | O_CREAT, 0700);
  assert_true(out > 0);
  test_state->fd1 = out;

  /* set memory */

  memset(buffer, '*', MAXLEN);

  /* do write */

  rval = write(out, buffer, MAXLEN);
  assert_int_in_range(rval, 1, MAXLEN);
}

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTFILENAME "loopTestFile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_write02
 ****************************************************************************/

void test_nuttx_fs_write02(FAR void **state)
{
  int rval;
  FILE *fp;
  long offset;
  char content[15] = "asdfgtgtrf";
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* fopen file */

  fp = fopen(TESTFILENAME, "a+");
  assert_non_null(fp);
  test_state->fd1 = fileno(fp);

  /* do fwrite */

  rval = fwrite(content, 1, 10, fp);
  assert_int_in_range(rval, 1, 10);

  /* refresh to storage */

  fsync(fileno(fp));

  /* do ffkush */

  fflush(fp);

  /* get ftell */

  offset = ftell(fp);
  assert_true(offset == 10);
  fclose(fp);
}

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TESTFILENAME3 "loopTestFile"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_write03
 ****************************************************************************/

void test_nuttx_fs_write03(FAR void **state)
{
  int rval;
  FILE *fp;
  long offset;
  char content[15] = "asdfgtgtrf";
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* fopen file */

  fp = fopen(TESTFILENAME3, "a+");
  assert_non_null(fp);
  test_state->fd1 = fileno(fp);

  /* do fwrite */

  rval = fwrite(content, 1, 10, fp);
  assert_int_in_range(rval, 1, 10);

  /* refresh to storage */

  fsync(fileno(fp));

  /* get ftell */

  offset = ftell(fp);
  assert_true(offset == 10);
  fclose(fp);
}
