/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_poll_test.c
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
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/epoll.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define I_FILE1 "poll_test1"
#define I_FILE2 "poll_test2"
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_poll01
 ****************************************************************************/

void test_nuttx_fs_poll01(FAR void **state)
{
  int poll01_fd1;
  int poll01_fd2;
  int poll01_ret;
  struct pollfd poll01_fds[5];
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  poll01_fd1 = open(I_FILE1, O_RDONLY | O_CREAT);
  assert_true(poll01_fd1 >= 0);
  test_state->fd1 = poll01_fd1;

  poll01_fds[0].fd = poll01_fd1;
  poll01_fds[0].events = POLLOUT;

  poll01_fd2 = open(I_FILE2, O_RDWR | O_CREAT);
  assert_true(poll01_fd2 >= 0);
  test_state->fd2 = poll01_fd2;

  poll01_fds[1].fd = poll01_fd2;
  poll01_fds[1].events = POLLIN;

  poll01_ret = poll(poll01_fds, 2, 5);
  assert_int_equal(poll01_ret, 2);

  close(poll01_fd1);
  close(poll01_fd2);
}
