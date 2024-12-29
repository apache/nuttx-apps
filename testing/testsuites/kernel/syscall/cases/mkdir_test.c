/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/mkdir_test.c
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
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_mkdir01
 ****************************************************************************/

void test_nuttx_syscall_mkdir01(FAR void **state)
{
  int ret;
  int j;
  int times = 100;
  char tmpdir[40];

  for (j = 0; j < 5; j += 3)
    {
      sprintf(tmpdir, "%s_%d_dir", __func__, j);
      ret = mkdir(tmpdir, 0777);
      if (ret < 0)
        {
          if (errno != EEXIST)
            {
              syslog(LOG_ERR,
                     "Error creating permanent directories, ERRNO = %d",
                     errno);
              fail_msg("test fail !\n");
              goto out;
            }
        }
    }

  while (times--)
    {
      for (j = 0; j < 5; j += 3)
        {
          sprintf(tmpdir, "%s_%d_dir", __func__, j);
          ret = mkdir(tmpdir, 0777);
          if (ret < 0)
            {
              if (errno != EEXIST)
                {
                  syslog(LOG_ERR,
                         "MKDIR %s, errno = %d; Wrong error detected.",
                         tmpdir, errno);
                  fail_msg("test fail !\n");
                  goto out;
                }
            }
          else
            {
              syslog(LOG_ERR,
                     "MKDIR %s succeded when it shoud have failed.",
                     tmpdir);
              fail_msg("test fail !\n");
              goto out;
            }
        }
    }

out:
  for (j = 0; j < 5; j += 3)
    {
      sprintf(tmpdir, "%s_%d_dir", __func__, j);
      assert_int_equal(rmdir(tmpdir), 0);
    }
}

/****************************************************************************
 * Name: test_nuttx_syscall_mkdir02
 ****************************************************************************/

void test_nuttx_syscall_mkdir02(FAR void **state)
{
  int ret;
  int j;
  int times = 1000;
  char tmpdir[40];

  while (times--)
    {
      for (j = 1; j < 5; j += 3)
        {
          sprintf(tmpdir, "%s_%d_dir", __func__, j);
          ret = rmdir(tmpdir);
          if (ret < 0)
            {
              if (errno != ENOENT)
                {
                  syslog(LOG_ERR,
                         "RMDIR %s, errno = %d; Wrong error detected.",
                         tmpdir, errno);
                  fail_msg("test fail !\n");
                }
            }

          else
            {
              syslog(LOG_ERR,
                     "RMDIR %s succeded when it should have failed.",
                     tmpdir);
              fail_msg("test fail !\n");
            }
        }
    }
}

/****************************************************************************
 * Name: test_nuttx_syscall_mkdir03
 ****************************************************************************/

void test_nuttx_syscall_mkdir03(FAR void **state)
{
  int ret;
  int j;
  int times = 1000;
  char tmpdir[40];

  while (times--)
    {
      for (j = 2; j < 5; j += 3)
        {
          sprintf(tmpdir, "%s_%d_dir", __func__, j);
          ret = mkdir(tmpdir, 0777);
          if (ret < 0)
            {
              syslog(LOG_ERR,
                     "MKDIR %s, errno = %d; Wrong error detected.",
                     tmpdir, errno);
              goto out;
            }
        }

      for (j = 2; j < 5; j += 3)
        {
          sprintf(tmpdir, "%s_%d_dir", __func__, j);
          ret = rmdir(tmpdir);
          if (ret < 0)
            {
              syslog(LOG_ERR,
                     "RMDIR %s, errno = %d; Wrong error detected.",
                     tmpdir, errno);
              goto out;
            }
        }
    }

  goto testpass;

out:
  for (j = 2; j < 5; j += 3)
    {
      sprintf(tmpdir, "%s/test%d", __func__, j);
      rmdir(tmpdir);
    }

  assert_true(0);
testpass:
  assert_true(1);
}
