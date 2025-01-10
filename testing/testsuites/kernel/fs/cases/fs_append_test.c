/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_append_test.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <errno.h>
#include <cmocka.h>
#include "fstest.h"

#define TESTFILENAME "stream01Testfile"

/****************************************************************************
 * Name: stream
 * Example description:
 *    1. open a file with "a+".
 *    2. Write some strings to the file.
 *    3. Check if the file pointer is offset.
 * Test item: fopen() fseek() ftell()
 * Expect results: TEST PASSED
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void test_nuttx_fs_append01(FAR void **state)
{
  FILE *fd;
  int pos;
  char readstring[80];
  size_t ret;
  fd = fopen(TESTFILENAME, "a+");
  if (fd == NULL)
    {
      syslog(LOG_ERR, "Unable to open file %s, errno %d\n", TESTFILENAME,
             errno);
      assert_true(1 == 0);
    }

  fprintf(fd, "This is a test of the append.\n");
  pos = ftell(fd);
  if (fseek(fd, 0, SEEK_END) < 0)
    {
      syslog(LOG_ERR, "fseek fail, errno %d\n", errno);
      fclose(fd);
      assert_true(1 == 0);
    }

  if (ftell(fd) != pos)
    {
      syslog(LOG_ERR, "Error opening for append ... data not at EOF\n");
      fclose(fd);
      assert_true(1 == 0);
    }

  if (fseek(fd, -30, SEEK_END) < 0)
    {
      syslog(LOG_ERR, "fseek fail, errno %d\n", errno);
      fclose(fd);
      assert_true(1 == 0);
    }

  readstring[30] = '\0';
  ret = fread(readstring, 1, 30, fd);
  readstring[ret] = 0;
  fclose(fd);
  if (strcmp(readstring, "This is a test of the append.\n") != 0)
    {
      syslog(LOG_ERR, "strcmp fail\n");
      assert_true(1 == 0);
    }
}
