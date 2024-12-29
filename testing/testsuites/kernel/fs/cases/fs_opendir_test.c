/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_opendir_test.c
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
#include <dirent.h>
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
 * Name: test_nuttx_fs_opendir01
 ****************************************************************************/

void test_nuttx_fs_opendir01(FAR void **state)
{
  DIR *dir;
  struct dirent *ptr;
  int ret;

  ret = mkdir("testopendir1", 0777);
  assert_int_equal(ret, 0);

  ret = mkdir("testopendir1/dir123", 0777);
  assert_int_equal(ret, 0);

  /* do opendir */

  dir = opendir("testopendir1");
  assert_non_null(dir);

  while ((ptr = readdir(dir)) != NULL)
    {
      if (strncmp(ptr->d_name, ".", 1) == 0)
        continue;
      if (strncmp(ptr->d_name, "..", 2) == 0)
        continue;
      if (strncmp(ptr->d_name, "dir123", 6) != 0)
        {
          closedir(dir);
          assert_true(0);
        }
    }

  /* close dir */

  assert_int_equal(closedir(dir), 0);
}

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_NUM 1000

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_opendir02
 ****************************************************************************/

void test_nuttx_fs_opendir02(FAR void **state)
{
  int ret;
  DIR *dir;
  struct dirent *ptr;

  /* mkdir for test */

  ret = mkdir("testopendir2", 0777);
  assert_int_equal(ret, 0);

  /* mkdir for test */

  ret = mkdir("testopendir2/dir_test2", 0777);
  assert_int_equal(ret, 0);

  for (int i = 0; i < TEST_NUM; i++)
    {
      /* open fir for test */

      dir = opendir("testopendir2");
      assert_true(dir != NULL);
      while ((ptr = readdir(dir)) != NULL)
        {
          if (strcmp(ptr->d_name, ".") == 0 ||
              strcmp(ptr->d_name, "..") == 0)
            {
              continue;
            }
          else if (strcmp(ptr->d_name, "dir_test2") == 0)
            {
              break;
            }
        }

      /* close dir */

      assert_int_equal(closedir(dir), 0);
    }
}
