/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_rewinddir_test.c
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
#include <time.h>
#include <errno.h>
#include <dirent.h>
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

#define TEST_PARENT_DIR "parent_dir"
#define TEST_CHILD_DIR1 "parent_dir/child_dir1"
#define TEST_CHILD_DIR2 "parent_dir/child_dir2"
#define TEST_CHILD_DIR3 "parent_dir/child_dir3"
#define TEST_CHILD_DIR4 "parent_dir/child_dir4"
#define TEST_CHILD_DIR5 "parent_dir/child_dir5"

#define TEST_CHILD_FILE1 "parent_dir/child_file1"
#define TEST_CHILD_FILE2 "parent_dir/child_file2"
#define TEST_CHILD_FILE3 "parent_dir/child_file3"

#define WRITE_BUF_SIZE 1024

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_cteatfile
 ****************************************************************************/

static void test_nuttx_fs_cteatfile(char *filename, size_t write_size)
{
  int fd;
  char w_buffer[WRITE_BUF_SIZE] =
  {
    0
  };

  ssize_t size = 0;

  /* open file */

  fd = open(filename, O_CREAT | O_RDWR, 0777);
  assert_true(fd > 0);

  /* set memory */

  memset(w_buffer, 0x61, WRITE_BUF_SIZE);

  do
    {
      if (write_size <= WRITE_BUF_SIZE)
        {
          /* do write */

          size = write(fd, w_buffer, write_size);
        }
      else
        {
          /* do write */

          size = write(fd, w_buffer, WRITE_BUF_SIZE);
        }

      write_size = write_size - size;
    }
  while (write_size > (size_t)0);

  /* close file */

  close(fd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_rewinddir01
 ****************************************************************************/

void test_nuttx_fs_rewinddir01(FAR void **state)
{
  DIR *dir;
  struct dirent *ptr;
  int count = 0;
  int r_count = 0;
  int test_flag = -1;

  unsigned long long size;
  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of the partition is less than
   * 50K
   */

  if (size < 51200)
    {
      syslog(LOG_WARNING, "Partitioned free space not enough !\n");
      syslog(LOG_WARNING, "Test case (%s) exits early !\n", __func__);
    }

  else
    {
      /* make directory */

      assert_int_equal(mkdir(TEST_PARENT_DIR, S_IRWXU), 0);
      assert_int_equal(mkdir(TEST_CHILD_DIR1, S_IRWXU), 0);
      assert_int_equal(mkdir(TEST_CHILD_DIR2, S_IRWXU), 0);
      assert_int_equal(mkdir(TEST_CHILD_DIR3, S_IRWXU), 0);
      assert_int_equal(mkdir(TEST_CHILD_DIR4, S_IRWXU), 0);
      assert_int_equal(mkdir(TEST_CHILD_DIR5, S_IRWXU), 0);

      /* create */

      test_nuttx_fs_cteatfile(TEST_CHILD_FILE1, 10);
      test_nuttx_fs_cteatfile(TEST_CHILD_FILE2, 10);
      test_nuttx_fs_cteatfile(TEST_CHILD_FILE3, 10);

      /* open directory */

      dir = opendir(TEST_PARENT_DIR);

      while ((ptr = readdir(dir)) != NULL)
        {
          count++;
        }

      rewinddir(dir);

      while ((ptr = readdir(dir)) != NULL)
        {
          r_count++;
        }

      /* close directory */

      assert_int_equal(closedir(dir), 0);

      if (count == r_count && count != 0)
        {
          test_flag = 0;
        }

      /* remove directory */

      rmdir(TEST_CHILD_DIR1);
      rmdir(TEST_CHILD_DIR2);
      rmdir(TEST_CHILD_DIR3);
      rmdir(TEST_CHILD_DIR4);
      rmdir(TEST_CHILD_DIR5);

      unlink(TEST_CHILD_FILE1);
      unlink(TEST_CHILD_FILE2);
      unlink(TEST_CHILD_FILE3);

      rmdir(TEST_PARENT_DIR);

      assert_int_equal(test_flag, 0);
    }
}
