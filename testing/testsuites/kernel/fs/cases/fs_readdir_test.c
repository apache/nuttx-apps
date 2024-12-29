/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_readdir_test.c
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_readdir01
 ****************************************************************************/

void test_nuttx_fs_readdir01(FAR void **state)
{
  int fd;
  int ret;
  char buf[20] =
  {
    0
  };

  char *filename[] =
  {
    "testfile1", "testfile2", "testfile3", "testfile4",
                      "testfile5", "testfile6", "testfile7"
  };

  DIR *test_dir;
  struct dirent *dptr;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  for (int i = 0; i < 6; i++)
    {
      /* open file */

      fd = open(filename[i], O_RDWR | O_CREAT, 0700);
      assert_true(fd > 0);
      test_state->fd1 = fd;

      /* do wirte */

      ret = write(fd, "hello!\n", 6);
      assert_uint_in_range(ret, 1, 6);

      close(fd);
    }

  /* do getcwd */

  getcwd(buf, sizeof(buf));

  /* open directory */

  test_dir = opendir(buf);
  assert_non_null(test_dir);

  while ((dptr = readdir(test_dir)) != 0)
    {
      if (strcmp(dptr->d_name, ".") && strcmp(dptr->d_name, ".."))
        continue;
    }

  /* close dir */

  assert_int_equal(closedir(test_dir), 0);
}
