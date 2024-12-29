/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/fcntl_test.c
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
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
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
 * Name: test_nuttx_syscall_fcntl01
 ****************************************************************************/

void test_nuttx_syscall_fcntl01(FAR void **state)
{
  int flags;
  int is_pass = 0;
  int ret;
  char fname[40] = "";
  int fd[10];
  int fd2[10];

  for (int i = 0; i < 8; i++)
    {
      sprintf(fname, "Fcntl01%d-%d", gettid(), i);
      fd[i] = open(fname, O_WRONLY | O_CREAT, 0666);
      if (fd[i] < 0)
        {
          syslog(LOG_ERR, "open test file fail !\n");
          is_pass = -1;
          break;
        }

      fd2[i] = fd[i];
    }

  assert_int_equal(is_pass, 0);
  close(fd[2]);
  close(fd[3]);
  close(fd[4]);
  close(fd[5]);

  assert_false((fd[2] = fcntl(fd[1], F_DUPFD, 1)) == -1);

  assert_false(fd[2] < fd2[2]);

  assert_false((fd[4] = fcntl(fd[1], F_DUPFD, fd2[3])) < 0);

  assert_false(fd[4] < fd2[3]);

  assert_false((fd[8] = fcntl(fd[1], F_DUPFD, fd2[5])) < 0);

  assert_false(fd[8] != fd2[5]);

  /* //block1: */

  flags = fcntl(fd[2], F_GETFL, 0);
  assert_false((flags & O_WRONLY) == 0);

  /* Check setting of no_delay flag */

  assert_false(fcntl(fd[2], F_SETFL, O_NDELAY) == -1);

  flags = fcntl(fd[2], F_GETFL, 0);
  assert_false((flags & (O_NDELAY | O_WRONLY)) == 0);

  /* Check of setting append flag */

  assert_false(fcntl(fd[2], F_SETFL, O_APPEND) == -1);

  flags = fcntl(fd[2], F_GETFL, 0);
  assert_false((flags & (O_APPEND | O_WRONLY)) == 0);

  /* Check setting flags together */

  assert_false(fcntl(fd[2], F_SETFL, O_NDELAY | O_APPEND) < 0);

  flags = fcntl(fd[2], F_GETFL, 0);
  assert_false((flags & (O_NDELAY | O_APPEND | O_WRONLY)) == 0);

  /* Check that flags are not cummulative */

  assert_false(fcntl(fd[2], F_SETFL, 0) == -1);

  flags = fcntl(fd[2], F_GETFL, 0);
  assert_false((flags & O_WRONLY) == 0);
  assert_false((flags = fcntl(fd[2], F_GETFD, 0)) < 0);
  assert_false(flags != 0);
  assert_false((flags = fcntl(fd[2], F_SETFD, 1)) == -1);
  assert_false((flags = fcntl(fd[2], F_GETFD, 0)) == -1);
  assert_false(flags != 1);

  for (int j = 0; j < 10; j++)
    close(fd[j]);
  for (int k = 0; k < 8; k++)
    {
      sprintf(fname, "Fcntl01%d-%d", gettid(), k);
      ret = unlink(fname);
      if (ret != 0)
        {
          syslog(LOG_ERR, "unlink test file(%s) fail !\n", fname);
          is_pass = -1;
        }
    }

  assert_int_equal(is_pass, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fcntl02
 ****************************************************************************/

void test_nuttx_syscall_fcntl02(FAR void **state)
{
  char fname[256] = "";
  int fd;
  int min_fd;
  int ret = 0;
  int min_fds[] =
  {
    0, 1, 2, 3, 10, 100
  };

  sprintf(fname, "Fcntl02_%d", gettid());
  fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  if (fd > 0)
    {
      min_fd = min_fds[0];
      ret = fcntl(fd, F_DUPFD, min_fd);
      assert_true(ret > min_fd);
    }

  if (ret > 0)
    close(ret);
  if (fd > 0)
    close(fd);
  ret = unlink(fname);
  assert_int_equal(ret, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fcntl03
 ****************************************************************************/

void test_nuttx_syscall_fcntl03(FAR void **state)
{
  int fd;
  int ret;
  char fname[256] = "";
  sprintf(fname, "fcntl03_%d", gettid());
  fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  ret = fcntl(fd, F_GETFD, 0);
  assert_true(ret != -1);

  close(fd);
  ret = unlink(fname);
  assert_int_equal(ret, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fcntl04
 ****************************************************************************/

void test_nuttx_syscall_fcntl04(FAR void **state)
{
  int fd;
  int ret;
  char fname[256] = "";
  sprintf(fname, "fcntl04_%d", gettid());

  fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  ret = fcntl(fd, F_GETFL, 0);
  assert_int_equal(ret & O_ACCMODE, O_RDWR);

  close(fd);
  ret = unlink(fname);
  assert_int_equal(ret, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fcntl05
 ****************************************************************************/

void test_nuttx_syscall_fcntl05(FAR void **state)
{
  int file_fd;
  int fifo_fd;
  int lc;
  int ret;
  int pipe_fds[2];
  char fname[256] = "";
  struct tcase
  {
    int *fd;
    const char *msg;
  }

  tcases[] =
    {
        {
          NULL, "regular file"
        },

        {
          NULL, "pipe (write end)"
        },

        {
          NULL, "pipe (read end)"
        },

        {
          NULL, "fifo"
        },
    };

  sprintf(fname, "fcntl05_%d", gettid());

  file_fd = open(fname, O_CREAT | O_RDWR, 0666);
  assert_true(file_fd > 0);
  assert_int_equal(pipe(pipe_fds), 0);
  unlink("/fcntl05_fifo");
  assert_int_equal(mkfifo("/fcntl05_fifo", 0777), 0);

  fifo_fd = open("/fcntl05_fifo", O_RDWR);
  assert_true(fifo_fd > 0);

  tcases[0].fd = &file_fd;
  tcases[1].fd = pipe_fds;
  tcases[2].fd = pipe_fds + 1;
  tcases[3].fd = &fifo_fd;

  for (lc = 0; lc < 1; lc++)
    {
      for (int i = 0; i < 4; i++)
        {
          ret = fcntl(*((tcases + i)->fd), F_SETFD, FD_CLOEXEC);
          assert_int_equal(ret, 0);
        }
    }

  if (file_fd > 0)
    assert_int_equal(close(file_fd), 0);
  if (pipe_fds[0] > 0)
    assert_int_equal(close(pipe_fds[0]), 0);
  if (pipe_fds[1] > 0)
    assert_int_equal(close(pipe_fds[1]), 0);
  if (fifo_fd > 0)
    assert_int_equal(close(fifo_fd), 0);

  assert_int_equal(unlink(fname), 0);
  assert_int_equal(unlink("/fcntl05_fifo"), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fcntl06
 ****************************************************************************/

void test_nuttx_syscall_fcntl06(FAR void **state)
{
  int fd;
  int ret;
  int lc;
  char fname[256] = "";

  sprintf(fname, "fcntl06_%d", gettid());

  fd = open(fname, O_RDWR | O_CREAT);
  assert_true(fd > 0);

  for (lc = 0; lc < 10; lc++)
    {
      ret = fcntl(fd, F_SETFL, O_NDELAY | O_APPEND | O_NONBLOCK);
      assert_true(ret != -1);
    }

  assert_int_equal(close(fd), 0);
  assert_int_equal(unlink(fname), 0);
}
