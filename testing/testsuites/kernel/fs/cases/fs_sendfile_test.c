/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_sendfile_test.c
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
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <errno.h>
#include <cmocka.h>
#include "fstest.h"

#define O_FILE "outputFile"
#define I_FILE1 "inputFile1"
#define I_FILE2 "inputFile2"
#define P_FILE "pipeInputFile"
#define P_SIZE 8192
#define P_BUFSIZE 256

static void set_test(void)
{
  int fd;
  fd = open(O_FILE, O_CREAT | O_RDWR, 0777);
  if (fd == -1)
    {
      syslog(LOG_ERR, "Unable to open file %s, errno %d\n", O_FILE,
             errno);
      assert_true(1 == 0);
    }
  write(fd, "ABCDEFGHIJ", 10);
  close(fd);
}

#if defined(CONFIG_PIPES) && CONFIG_DEV_PIPE_SIZE > 0

/****************************************************************************
 * Name: sendfile_pipe_send
 *
 * Description:
 *   Call sendfile() and check that the offset (or the file position if
 *   offset is NULL) moves by the count that it returns.
 *
 * Returned Value:
 *   The count, 0 for EAGAIN, or -1 on a failure.
 ****************************************************************************/

static ssize_t sendfile_pipe_send(int outfd, int infd, FAR off_t *offset,
                                  size_t count)
{
  off_t before = offset != NULL ? *offset : lseek(infd, 0, SEEK_CUR);
  off_t after;
  ssize_t ret;

  ret = sendfile(outfd, infd, offset, count);
  if (ret < 0 && errno == EAGAIN)
    {
      ret = 0;
    }
  else if (ret <= 0)
    {
      syslog(LOG_ERR, "sendfile fail, ret %zd, errno %d\n", ret, errno);
      return -1;
    }

  after = offset != NULL ? *offset : lseek(infd, 0, SEEK_CUR);
  if (after - before != ret)
    {
      syslog(LOG_ERR, "sendfile sent %zd but the offset moved %" PRIdOFF
             "\n", ret, after - before);
      return -1;
    }

  return ret;
}

/****************************************************************************
 * Name: sendfile_pipe
 *
 * Description:
 *   Send a file to a nonblocking pipe that fills up.  Each byte must
 *   arrive once, and the count that sendfile() returns must match the
 *   offset and the bytes in the pipe.
 *
 * Returned Value:
 *   0 if all checks pass, else 1.
 ****************************************************************************/

static int sendfile_pipe(FAR off_t *offset)
{
  unsigned char buf[P_BUFSIZE];
  size_t received = 0;
  size_t sent = 0;
  ssize_t ret;
  ssize_t n;
  ssize_t i;
  int fail = 0;
  int fds[2];
  int fd;

  fd = open(P_FILE, O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0777);
  if (fd < 0)
    {
      syslog(LOG_ERR, "open %s fail, errno %d\n", P_FILE, errno);
      return 1;
    }

  for (n = 0; n < P_SIZE; n += P_BUFSIZE)
    {
      for (i = 0; i < P_BUFSIZE; i++)
        {
          buf[i] = (n + i) % 251;
        }

      if (write(fd, buf, P_BUFSIZE) != P_BUFSIZE)
        {
          syslog(LOG_ERR, "write %s fail, errno %d\n", P_FILE, errno);
          close(fd);
          return 1;
        }
    }

  if (lseek(fd, 0, SEEK_SET) != 0 || pipe2(fds, O_CLOEXEC | O_NONBLOCK) < 0)
    {
      syslog(LOG_ERR, "lseek or pipe2 fail, errno %d\n", errno);
      close(fd);
      return 1;
    }

  while (sent < P_SIZE)
    {
      /* The pipe is empty: the call must send some bytes */

      ret = sendfile_pipe_send(fds[1], fd, offset, P_SIZE - sent);
      if (ret <= 0)
        {
          fail = 1;
          break;
        }

      sent += ret;

      /* The pipe is full now: the next call must send nothing */

      if (sent < P_SIZE &&
          sendfile_pipe_send(fds[1], fd, offset, P_SIZE - sent) != 0)
        {
          fail = 1;
          break;
        }

      while ((n = read(fds[0], buf, P_BUFSIZE)) > 0)
        {
          for (i = 0; i < n; i++)
            {
              if (buf[i] != (received + i) % 251)
                {
                  fail = 1;
                }
            }

          received += n;
        }

      if (received != sent)
        {
          syslog(LOG_ERR, "the pipe got %zu bytes, sendfile sent %zu\n",
                 received, sent);
          fail = 1;
          break;
        }
    }

  /* With an offset, sendfile() must not move the file position */

  if (offset != NULL && lseek(fd, 0, SEEK_CUR) != 0)
    {
      syslog(LOG_ERR, "sendfile moved the file position\n");
      fail = 1;
    }

  close(fds[0]);
  close(fds[1]);
  close(fd);
  return fail;
}
#endif

/****************************************************************************
 * Name: sendfile
 * Example description:
 *  1.Test copy the entire file
 * Expect results: TEST PASSED
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void test_nuttx_fs_sendfile01(FAR void **state)
{
  int ret;
  int fd_o1;
  int fd_o2;
  int fd_i;
  int test_flag = 0;
  struct stat f_stat;

  set_test();

  /* open file readonly */

  fd_i = open(O_FILE, O_RDONLY);
  if (fd_i == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  /* open file O_RDWR */

  fd_o1 = open(I_FILE1, O_CREAT | O_RDWR, 0777);
  if (fd_o1 == -1)
    {
      close(fd_i);
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  if (fstat(fd_i, &f_stat) < 0)
    {
      syslog(LOG_ERR, "fstat fail !, errno %d\n", errno);
      close(fd_i);
      close(fd_o1);
      assert_true(1 == 0);
    }

  /* sendfile , copy the entire file */

  ret = sendfile(fd_o1, fd_i, NULL, f_stat.st_size);

  if (ret != f_stat.st_size)
    {
      syslog(LOG_ERR, "ret != f_stat.st_size\n");
      test_flag = 1;
    }

  lseek(fd_i, 0, SEEK_SET);

  fd_o2 = open(I_FILE2, O_CREAT | O_RDWR, 0777);
  if (fd_o2 == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      close(fd_i);
      close(fd_o1);
      assert_true(1 == 0);
    }

  /* sendfile , Copy part of the file */

  ret = sendfile(fd_o2, fd_i, NULL, 5);

  if (ret != 5)
    {
      syslog(LOG_ERR, "ret != 5\n");
      test_flag = 1;
    }

  close(fd_o1);
  close(fd_o2);
  close(fd_i);
  assert_true(test_flag == 0);
}

/****************************************************************************
 * Name: sendfile
 * Example description:
 *  1.Test copy the entire file
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_sendfile02(FAR void **state)
{
  int fd_o;
  int fd_i;
  off_t offset;
  struct stat f_stat;

  set_test();

  /* open file readonly */

  fd_i = open(O_FILE, O_RDONLY);
  if (fd_i == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      assert_true(1 == 0);
    }

  /* open file O_RDWR */

  fd_o = open(I_FILE1, O_CREAT | O_RDWR, 0777);
  if (fd_o == -1)
    {
      syslog(LOG_ERR, "open file fail !, errno %d\n", errno);
      close(fd_i);
      assert_true(1 == 0);
    }

  if (fstat(fd_i, &f_stat) < 0)
    {
      syslog(LOG_ERR, "fstat fail ! errno %d\n", errno);
      close(fd_i);
      close(fd_o);
      assert_true(1 == 0);
    }

  offset = 5;

  /* sendfile */

  sendfile(fd_o, fd_i, &offset, f_stat.st_size - offset);

  close(fd_o);
  close(fd_i);

  /* Checks whether the file pointer is offset to the specified position
   */

  assert_int_equal(offset, f_stat.st_size);
}

#if defined(CONFIG_PIPES) && CONFIG_DEV_PIPE_SIZE > 0

/****************************************************************************
 * Name: sendfile
 * Example description:
 *  1.Test sendfile with an offset to a nonblocking pipe that fills up
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_sendfile03(FAR void **state)
{
  off_t offset = 0;

  assert_int_equal(sendfile_pipe(&offset), 0);
  assert_int_equal(offset, P_SIZE);
}

/****************************************************************************
 * Name: sendfile
 * Example description:
 *  1.Test sendfile without an offset to a nonblocking pipe that fills up
 * Expect results: TEST PASSED
 ****************************************************************************/

void test_nuttx_fs_sendfile04(FAR void **state)
{
  assert_int_equal(sendfile_pipe(NULL), 0);
}
#endif
