/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_rmdir_test.c
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
#include "nsh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PARENTDIR1 "parentDirName"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_fmdir01
 ****************************************************************************/

void test_nuttx_fs_fmdir01(FAR void **state)
{
  int status;
  int fd;
  char str[5];
  char test_file_name[20] = {
    0
  };

  char test_dir_name[20] = {
    0
  };

  char absolute_directory[100] = {
    0
  };

  char current_path[100] = {
    0
  };

  char parent_directory[PATH_MAX] = {
    0
  };

  unsigned long long size;
  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of
   * the partition is less than 160K
   */

  if (size < 163840)
    {
      syslog(LOG_WARNING, "Partitioned free space not enough !\n");
      syslog(LOG_WARNING, "Test case (%s) exits early !\n", __func__);
    }
  else
    {
      /* create directory */

      status = mkdir(PARENTDIR1, 0700);
      assert_int_equal(status, 0);

      /* get test path */

      getcwd(current_path, sizeof(current_path));

      strcpy(absolute_directory, current_path);
      strcat(current_path, "/");
      strcat(current_path, PARENTDIR1);

      strcpy(parent_directory, current_path);

      chdir(current_path);

      /* get test path */

      getcwd(current_path, sizeof(current_path));

      /* create 10 2-level subfolders */

      for (int i = 0; i < 10; i++)
        {
          itoa(i, str, 10);
          status = mkdir(str, 0700);
          assert_int_equal(status, 0);
        }

      /* switch to directory 5 */

      itoa(5, str, 10);

      /* enter sub-directory */

      strcat(current_path, "/");
      strcat(current_path, str);
      chdir(current_path);

      /* get test path */

      getcwd(current_path, sizeof(current_path));

      /* make directory */

      status = mkdir("test_3_dir_1", 0700);
      assert_int_equal(status, 0);

      /* make directory */

      status = mkdir("test_3_dir_2", 0700);
      assert_int_equal(status, 0);

      /* switch to directory 8 */

      itoa(8, str, 10);

      /* enter sub-directory */

      memset(current_path, 0, sizeof(current_path));
      strcpy(current_path, parent_directory);
      strcat(current_path, "/");
      strcat(current_path, str);
      chdir(current_path);

      /* get test path */

      getcwd(current_path, sizeof(current_path));

      for (int j = 1; j <= 10; j++)
        {
          sprintf(test_file_name, "test_3_file_%d", j);

          /* creat a test file */

          fd = creat(test_file_name, 0700);
          assert_true(fd > 0);
          close(fd);

          /* set memory */

          memset(test_file_name, 0, sizeof(test_file_name));
        }

      /* switch to directory 2 */

      itoa(2, str, 10);

      /* enter sub-directory */

      memset(current_path, 0, sizeof(current_path));
      strcpy(current_path, parent_directory);
      strcat(current_path, "/");
      strcat(current_path, str);
      chdir(current_path);

      /* get test path */

      getcwd(current_path, sizeof(current_path));

      for (int k = 1; k <= 5; k++)
        {
          sprintf(test_file_name, "test_3_file_%d", k);
          sprintf(test_dir_name, "test_3_dir_%d", k);

          /* create a test file */

          fd = creat(test_file_name, 0700);
          assert_true(fd > 0);
          close(fd);

          /* make  directory */

          status = mkdir(test_dir_name, 0700);
          assert_int_equal(status, 0);

          /* set memory */

          memset(test_file_name, 0, sizeof(test_file_name));
          memset(test_dir_name, 0, sizeof(test_dir_name));
        }

      /* wwitch to the test absolute directory */

      chdir(absolute_directory);

      /* call the recursive delete interface */

      cm_unlink_recursive(parent_directory);
    }
}

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PARENTDIR2 "parentDirName2"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_rmdir02
 ****************************************************************************/

void test_nuttx_fs_rmdir02(FAR void **state)
{
  int status;
  int ret;
  char str[20] = {
    0
  };

  char absolute_directory[20] = {
    0
  };

  char parent_directory[PATH_MAX] = {
    0
  };

  char temporary_path[300] = {
    0
  };

  unsigned long long size;

  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of the partition is less than 98K */

  if (size < 98304)
    {
      syslog(LOG_WARNING, "Partitioned free space not enough !\n");
      syslog(LOG_WARNING, "Test case (%s) exits early !\n", __func__);
    }
  else
    {
      getcwd(absolute_directory, sizeof(absolute_directory));

      /* create directory */

      status = mkdir(PARENTDIR2, 0700);
      assert_int_equal(status, 0);

      strcpy(parent_directory, absolute_directory);
      strcat(parent_directory, "/");
      strcat(parent_directory, PARENTDIR2);

      /* switch to test PARENTDIR */

      chdir(parent_directory);

      /* create a 6-level directory in a loop */

      for (int i = 0; i < 6; i++)
        {
          /* get current path */

          getcwd(temporary_path, sizeof(temporary_path));
          strcat(temporary_path, "/");

          /* do snprintf */

          ret = snprintf(str, 20, "test_dir_%d", i);
          assert_true(ret > 0);

          strcat(temporary_path, str);

          /* make directory */

          status = mkdir(temporary_path, 0700);
          assert_int_equal(status, 0);

          chdir(temporary_path);

          /* set memory */

          memset(temporary_path, 0, sizeof(temporary_path));
          memset(str, 0, sizeof(str));
        }

      /* wwitch to the test absolute directory */

      chdir(absolute_directory);

      /* call the recursive delete interface */

      cm_unlink_recursive(parent_directory);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_rmdir03
 ****************************************************************************/

void test_nuttx_fs_rmdir03(FAR void **state)
{
  int status;
  DIR *dir = NULL;
  char str[5];
  char buf[20] = {
    0
  };

  struct dirent *ptr;

  unsigned long long size;
  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of the partition is less than 80K */

  if (size < 81920)
    {
      syslog(LOG_WARNING, "Partitioned free space not enough !\n");
      syslog(LOG_WARNING, "Test case (%s) exits early !\n", __func__);
    }
  else
    {
      for (int i = 0; i < 5; i++)
        {
          itoa(i, str, 10);

          /* make directory */

          status = mkdir(str, 0700);
          assert_int_equal(status, 0);
        }

      getcwd(buf, sizeof(buf));

      /* open directory */

      dir = opendir(buf);
      while ((ptr = readdir(dir)) != NULL)
        {
          status = rmdir(ptr->d_name);
        }

      /* close directory flow */

      assert_int_equal(closedir(dir), 0);
    }
}
