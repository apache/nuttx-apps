/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/pipe_test.c
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
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
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
 * Name: test_nuttx_syscall_pipe01
 ****************************************************************************/

void test_nuttx_syscall_pipe01(FAR void **state)
{
  int fds[2];
  int ret;
  int rd_size;
  int wr_size;
  char wrbuf[64] = "abcdefghijklmnopqrstuvwxyz";
  char rdbuf[128];

  memset(rdbuf, 0, sizeof(rdbuf));

  ret = pipe(fds);
  assert_int_not_equal(ret, -1);

  /* write fds[1] */

  wr_size = write(fds[1], wrbuf, sizeof(wrbuf));

  /* read fds[0] */

  rd_size = read(fds[0], rdbuf, sizeof(rdbuf));

  assert_int_equal(rd_size, wr_size);
  assert_int_equal(strncmp(rdbuf, wrbuf, wr_size), 0);

  assert_int_equal(close(fds[0]), 0);
  assert_int_equal(close(fds[1]), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_pipe02
 ****************************************************************************/

void test_nuttx_syscall_pipe02(FAR void **state)
{
  int fd[2];
  int ret;
  char buf[2];

  ret = pipe(fd);
  assert_int_not_equal(ret, -1);

  ret = write(fd[0], "A", 1);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, EACCES);

  ret = read(fd[1], buf, 1);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, EACCES);

  assert_int_equal(close(fd[0]), 0);
  assert_int_equal(close(fd[1]), 0);
}
