/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_rmdir_test.c
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
#include "nsh.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PARENTDIR1 "parentDirName"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_rmdir01
 ****************************************************************************/

void test_nuttx_fs_rmdir01(FAR void **state)
{
  int status;
  int fd;
  char str[5];
  char testfilename[20] =
  {
    0
  };

  char testdirname[20] =
  {
    0
  };

  char absolutedirectory[100] =
  {
    0
  };

  char currentpath[100] =
  {
    0
  };

  char parentdirectory[PATH_MAX] =
  {
    0
  };

  unsigned long long size;
  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of the partition is less than
   * 160K
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

      getcwd(currentpath, sizeof(currentpath));

      strcpy(absolutedirectory, currentpath);
      strcat(currentpath, "/");
      strcat(currentpath, PARENTDIR1);

      strcpy(parentdirectory, currentpath);

      chdir(currentpath);

      /* get test path */

      getcwd(currentpath, sizeof(currentpath));

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

      strcat(currentpath, "/");
      strcat(currentpath, str);
      chdir(currentpath);

      /* get test path */

      getcwd(currentpath, sizeof(currentpath));

      /* make directory */

      status = mkdir("test_3_dir_1", 0700);
      assert_int_equal(status, 0);

      /* make directory */

      status = mkdir("test_3_dir_2", 0700);
      assert_int_equal(status, 0);

      /* switch to directory 8 */

      itoa(8, str, 10);

      /* enter sub-directory */

      memset(currentpath, 0, sizeof(currentpath));
      strcpy(currentpath, parentdirectory);
      strcat(currentpath, "/");
      strcat(currentpath, str);
      chdir(currentpath);

      /* get test path */

      getcwd(currentpath, sizeof(currentpath));

      for (int j = 1; j <= 10; j++)
        {
          sprintf(testfilename, "test_3_file_%d", j);

          /* creat a test file */

          fd = creat(testfilename, 0700);
          assert_true(fd > 0);
          close(fd);

          /* set memory */

          memset(testfilename, 0, sizeof(testfilename));
        }

      /* switch to directory 2 */

      itoa(2, str, 10);

      /* enter sub-directory */

      memset(currentpath, 0, sizeof(currentpath));
      strcpy(currentpath, parentdirectory);
      strcat(currentpath, "/");
      strcat(currentpath, str);
      chdir(currentpath);

      /* get test path */

      getcwd(currentpath, sizeof(currentpath));

      for (int k = 1; k <= 5; k++)
        {
          sprintf(testfilename, "test_3_file_%d", k);
          sprintf(testdirname, "test_3_dir_%d", k);

          /* create a test file */

          fd = creat(testfilename, 0700);
          assert_true(fd > 0);
          close(fd);

          /* make  directory */

          status = mkdir(testdirname, 0700);
          assert_int_equal(status, 0);

          /* set memory */

          memset(testfilename, 0, sizeof(testfilename));
          memset(testdirname, 0, sizeof(testdirname));
        }

      /* wwitch to the test absolute directory */

      chdir(absolutedirectory);

      /* call the recursive delete interface */

      cm_unlink_recursive(parentdirectory);
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
  char str[20] =
  {
    0
  };

  char absolutedirectory[20] =
  {
    0
  };

  char parentdirectory[PATH_MAX] =
  {
    0
  };

  char temporarypath[300] =
  {
    0
  };

  unsigned long long size;

  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of the partition is less than
   * 98K
   */

  if (size < 98304)
    {
      syslog(LOG_WARNING, "Partitioned free space not enough !\n");
      syslog(LOG_WARNING, "Test case (%s) exits early !\n", __func__);
    }
  else
    {
      getcwd(absolutedirectory, sizeof(absolutedirectory));

      /* create directory */

      status = mkdir(PARENTDIR2, 0700);
      assert_int_equal(status, 0);

      strcpy(parentdirectory, absolutedirectory);
      strcat(parentdirectory, "/");
      strcat(parentdirectory, PARENTDIR2);

      /* switch to test PARENTDIR */

      chdir(parentdirectory);

      /* create a 6-level directory in a loop */

      for (int i = 0; i < 6; i++)
        {
          /* get current path */

          getcwd(temporarypath, sizeof(temporarypath));
          strcat(temporarypath, "/");

          /* do snprintf */

          ret = snprintf(str, 20, "test_dir_%d", i);
          assert_true(ret > 0);

          strcat(temporarypath, str);

          /* make directory */

          status = mkdir(temporarypath, 0700);
          assert_int_equal(status, 0);

          chdir(temporarypath);

          /* set memory */

          memset(temporarypath, 0, sizeof(temporarypath));
          memset(str, 0, sizeof(str));
        }

      /* wwitch to the test absolute directory */

      chdir(absolutedirectory);

      /* call the recursive delete interface */

      cm_unlink_recursive(parentdirectory);
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
  char buf[20] =
  {
    0
  };

  struct dirent *ptr;

  unsigned long long size;
  size = cm_get_partition_available_size();
  if (size == (unsigned long long)-1)
    {
      fail_msg("Failed to obtain partition information !\n");
    }

  /* Stop the test if the available space of the partition is less than
   * 80K
   */

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
