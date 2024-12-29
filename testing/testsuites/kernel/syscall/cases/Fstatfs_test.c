/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/Fstatfs_test.c
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
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
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
 * Name: test_nuttx_syscall_fstatfs01
 ****************************************************************************/

void test_nuttx_syscall_fstatfs01(FAR void **state)
{
  int file_fd;
  int pipe_fd;
  int lc;
  int i;
  int ret;
  int p[2];
  struct statfs stats;
  char fname[256] = "";

  struct tcase
  {
    int *fd;
    const char *msg;
  }

  tcases[2] =
    {
        {
          NULL, "fstatfs() on a file"
        },

        {
          NULL, "fstatfs() on a pipe"
        },
    };

  tcases[0].fd = &file_fd;
  tcases[1].fd = &pipe_fd;

  sprintf(fname, "Fstatfs01_%d", gettid());

  file_fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(file_fd > 0);

  assert_int_equal(pipe(p), 0);
  pipe_fd = p[0];
  assert_int_equal(close(p[1]), 0);

  for (lc = 0; lc < 2; lc++)
    {
      for (i = 0; i < 1; i++)
        {
          ret = fstatfs(*(tcases[i].fd), &stats);
          assert_int_equal(ret, 0);
        }
    }

  if (file_fd > 0)
    assert_int_equal(close(file_fd), 0);

  if (pipe_fd > 0)
    assert_int_equal(close(pipe_fd), 0);

  assert_int_equal(unlink(fname), 0);
}
