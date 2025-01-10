/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/fsync_test.c
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
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/statvfs.h>
#include <sys/resource.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
 * Name: test_nuttx_syscall_fsync01
 ****************************************************************************/

void test_nuttx_syscall_fsync01(FAR void **state)
{
  char fname[255];
  char buf[8] = "davef";
  int fd;
  int ret;
  ssize_t w_size;

  sprintf(fname, "Fsync01_%d", gettid());

  /* creat a test file */

  fd = open(fname, O_RDWR | O_CREAT, 0700);
  assert_true(fd > 0);

  for (int i = 0; i < 10; i++)
    {
      /* write file */

      w_size = write(fd, buf, sizeof(buf));
      assert_true(w_size != -1);

      /* do sync */

      ret = fsync(fd);
      assert_int_equal(ret, 0);
    }

  assert_int_equal(close(fd), 0);
  assert_int_equal(unlink(fname), 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fsync02
 ****************************************************************************/

void test_nuttx_syscall_fsync02(FAR void **state)
{
  int fifo_rfd;
  int fifo_wfd;
  int pipe_fd[2];
#ifndef CONFIG_FDSAN
  int bad_fd = -1;
#endif
  int ret;
  int test_flag = 0;

  struct tcase
  {
    int *fd;
    int error;
  }

  tcases[] =
    {
      /* EINVAL - fsync() on pipe should not succeed. */

        {
          &pipe_fd[1], EINVAL
        },

      /* EBADF - fd is closed */

        {
          &pipe_fd[0], EBADF
        },

      /* EBADF - fd is invalid (-1) */

  #ifndef CONFIG_FDSAN
        {
          &bad_fd, EBADF
        },
  #endif

      /* EINVAL - fsync() on fifo should not succeed. */

        {
          &fifo_wfd, EINVAL
        },
    };

  ret = mkfifo("/var/Test_Fifo_SyscallFsync02", 0666);
  assert_int_equal(ret, 0);

  ret = pipe(pipe_fd);
  assert_int_not_equal(ret, -1);

  fifo_rfd =
      open("/var/Test_Fifo_SyscallFsync02", O_RDONLY | O_NONBLOCK);
  fifo_wfd = open("/var/Test_Fifo_SyscallFsync02", O_WRONLY);

  close(pipe_fd[0]);

  for (int i = 0; i < sizeof(tcases) / sizeof(tcases[0]); i++)
    {
      ret = fsync(*(tcases[i].fd));
      if (ret != -1)
        {
          syslog(LOG_ERR, "fsync() returned unexpected value %d", ret);
          test_flag = -1;
        }
      else if (errno != tcases[i].error)
        {
          syslog(LOG_ERR, "fsync(): unexpected error. exp=%d   errno=%d",
                 tcases[i].error, errno);
        }
      else
        {
          /* test PASS */

          test_flag = 0;
        }
    }

  close(fifo_wfd);
  close(fifo_rfd);
  close(pipe_fd[1]);
  unlink("/var/Test_Fifo_SyscallFsync02");

  assert_int_equal(test_flag, 0);
}

/****************************************************************************
 * Name: test_nuttx_syscall_fsync03
 ****************************************************************************/

void test_nuttx_syscall_fsync03(FAR void **state)
{
  int fd;
  int BLOCKSIZE = 8192;
  unsigned long MAXBLKS = 65536;
  int TIME_LIMIT = 120;
  int BUF_SIZE = 2048;
  char fname[255];
  struct statvfs stat_buf;
  unsigned long f_bavail;
  char pbuf[2048];

  off_t max_blks = 65536;

  sprintf(fname, "Fsync03_%d", getpid());
  fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);

  if (fstatvfs(fd, &stat_buf) != 0)
    {
      syslog(LOG_ERR, "FAIL, fstatvfs failed\n");
    }

  f_bavail = (stat_buf.f_bavail * stat_buf.f_bsize) / BLOCKSIZE;
  if (f_bavail && (f_bavail < MAXBLKS))
    {
      max_blks = f_bavail;
    }

  off_t offset;
  int i;
  int ret;
  int max_block = 0;
  int data_blocks = 0;
  time_t time_start;
  time_t time_end;
  double time_delta;
  long int random_number;

  random_number = rand();
  max_block = random_number % max_blks + 1;
  data_blocks = random_number % max_block;

  for (i = 1; i <= data_blocks; i++)
    {
      offset = i * ((BLOCKSIZE * max_block) / data_blocks);
      offset -= BUF_SIZE;
      lseek(fd, offset, SEEK_SET);
      write(fd, pbuf, BUF_SIZE);
    }

  time_start = time(0);

  ret = fsync(fd);

  time_end = time(0);
  assert_true(time_end != (time_t)-1);
  assert_int_not_equal(ret, -1);
  assert_int_equal(ret, 0);
  assert_false(time_end < time_start);
  assert_false((time_delta = difftime(time_end, time_start)) >
               TIME_LIMIT);

  /* SAFE_FTRUNCATE(fd, 0); */

  ret = ftruncate(fd, 0);
  assert_int_equal(ret, 0);

  close(fd);
  unlink(fname);
}
