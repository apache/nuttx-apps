/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/symlink_test.c
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
#include <dirent.h>
#include <string.h>
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
 * Name: test_nuttx_syscall_symlink01
 ****************************************************************************/

void test_nuttx_syscall_symlink01(FAR void **state)
{
  char fname[255];
  char symlnk[255];
  int fd;
  int ret;

  sprintf(fname, "%s_tfile", __func__);
  fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);
  if (fd > 0)
    close(fd);
  sprintf(symlnk, "/%s_t_%d", __func__, gettid());
  ret = symlink(fname, symlnk);
  if (fd > 0)
    close(fd);
  assert_int_equal(ret, OK);

  assert_int_equal(unlink(fname), 0);
  assert_int_equal(unlink(symlnk), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_symlink02
 ****************************************************************************/

void test_nuttx_syscall_symlink02(FAR void **state)
{
  int fd;
  int ret;
  struct stat stat_buf;
  char testfile[128] =
  {
    0
  };

  snprintf(testfile, sizeof(testfile), "%s_file", __func__);

  /* creat/open a testfile */

  fd = open(testfile, O_RDWR | O_CREAT,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0)
    {
      syslog(LOG_ERR, "open test file fail !\n");
      fail_msg("test fail !");
    }

  /* Close the temporary file created above */

  assert_int_equal(close(fd), 0);

  ret = symlink(testfile, "/Symlink02_file");
  if (ret == -1)
    {
      syslog(LOG_ERR, "symlink testfile to symfile Failed, errno=%d \n",
             errno);
      fail_msg("test fail!");
    }
  else
    {
      /* Get the symlink file status information
       * using lstat(2).
       */

      if (lstat("/Symlink02_file", &stat_buf) < 0)
        {
          syslog(LOG_ERR, "lstat(2) of symfile failed, error:%d \n",
                 errno);
          fail_msg("test fail!");
        }

      /* Check if the st_mode contains a link  */

      if (!S_ISLNK(stat_buf.st_mode))
        {
          syslog(LOG_ERR, "symlink of testfile doesn't exist \n");
          fail_msg("test fail!");
        }
    }

  assert_int_equal(unlink(testfile), 0);
  assert_int_equal(unlink("/Symlink02_file"), 0);
}
