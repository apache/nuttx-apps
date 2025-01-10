/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/readdir_test.c
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
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <syslog.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_setup
 ****************************************************************************/

static void test_nuttx_syscall_setup(char *prefix, int nfiles)
{
  char fname[255] =
  {
    0
  };

  int i;
  int fd;
  int ret;

  for (i = 0; i < nfiles; i++)
    {
      sprintf(fname, "%s_%d", prefix, i);
      fd = open(fname, O_RDWR | O_CREAT, 0700);
      assert_true(fd > 0);

      ret = write(fd, "hello\n", 6);
      assert_int_in_range(ret, 1, 6);

      assert_int_equal(close(fd), 0);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_readdirtest01
 ****************************************************************************/

void test_nuttx_syscall_readdirtest01(FAR void **state)
{
  char *prefix = "readdirfile";
  int nfiles = 10;
  test_nuttx_syscall_setup(prefix, nfiles);

  int cnt = 0;
  DIR *test_dir;
  struct dirent *ent;

  char buf[20] =
  {
    0
  };

  getcwd(buf, sizeof(buf));
  test_dir = opendir(buf);
  assert_non_null(test_dir);

  while ((ent = readdir(test_dir)))
    {
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
        continue;
      if (!strncmp(ent->d_name, prefix, sizeof(prefix) - 1))
        cnt++;
    }

  assert_int_equal(cnt, nfiles);
  assert_int_equal(closedir(test_dir), 0);
}
