/****************************************************************************
 * apps/examples/pipe/pipe_main.c
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

#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "pipe.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: open_write_only
 ****************************************************************************/

static void *open_write_only(pthread_addr_t pvarg)
{
  void *fd_addr = (void *)pvarg;

  fprintf(stderr, "open_write_only: Opening FIFO for write access\n");
  ((int *)fd_addr)[1] = open(FIFO_PATH1, O_WRONLY);
  if (((int *)fd_addr)[1] < 0)
    {
      fprintf(stderr, \
              "open_write_only: Failed to open FIFO %s for writing,"
              "errno=%d\n", FIFO_PATH1, errno);
      return (void *)(uintptr_t)1;
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pipe_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#if CONFIG_DEV_FIFO_SIZE > 0 || CONFIG_DEV_PIPE_SIZE > 0
  int fd[2];
  int ret;
#endif

#if CONFIG_DEV_FIFO_SIZE > 0
  /* Test FIFO logic */

  fprintf(stderr, "\npipe_main: Performing FIFO test\n");

  pthread_t writeonly;
  void *status;

  /* Open one end of the FIFO for reading and the other end for writing.
   * NOTE: This test's result is expected to be the same on any other
   * POSIX-compliant system. NuttX FIFOs block for write-only and
   * read-only (see interlock_test()).
   */

  ret = mkfifo(FIFO_PATH1, 0666);
  if (ret < 0)
    {
      fprintf(stderr, "pipe_main: mkfifo failed with errno=%d\n", errno);
      return 1;
    }

  ret = pthread_create(&writeonly, NULL, open_write_only, &fd);

  if (ret < 0)
    {
      fprintf(stderr, "redirection_test: "
              "Failed to create open_write_only task: %d\n", ret);
      return 2;
    }

  fd[0] = open(FIFO_PATH1, O_RDONLY);
  if (fd[0] < 0)
    {
      fprintf(stderr,
              "pipe_main: Failed to open FIFO %s for reading, errno=%d\n",
              FIFO_PATH1, errno);
      if (close(fd[1]) != 0)
        {
          fprintf(stderr, "pipe_main: close failed: %d\n", errno);
        }

      return 3;
    }

  /* Wait for open_write_only thread to complete */

  fprintf(stderr, "open_write_only: Waiting for open_write_only thread\n");
  ret = pthread_join(writeonly, &status);
  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: pthread_join failed, error=%d\n", ret);
      return 4;
    }
  else
    {
      fprintf(stderr, "pipe_main: open_write_only returned %p\n", status);
      if (status != NULL)
        {
          return 5;
        }
    }

  /* Then perform the test using those file descriptors */

  ret = transfer_test(fd[0], fd[1], 0, 0);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO test FAILED (00) (%d)\n", ret);
      return 6;
    }

  ret = transfer_test(fd[0], fd[1], 1, 0);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO test FAILED (10) (%d)\n", ret);
      return 6;
    }

  ret = transfer_test(fd[0], fd[1], 0, 1);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO test FAILED (01)(%d)\n", ret);
      return 6;
    }

  ret = transfer_test(fd[0], fd[1], 1, 1);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO test FAILED (11) (%d)\n", ret);
      return 6;
    }

  if (close(fd[0]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }

  if (close(fd[1]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }

  /* unlink(FIFO_PATH1); fails */

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO test FAILED (%d)\n", ret);
      return 6;
    }

  ret = remove(FIFO_PATH1);
  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: remove failed with errno=%d\n", errno);
    }

  /* Perform the FIFO interlock test */

  fprintf(stderr, "\npipe_main: Performing pipe interlock test\n");
  ret = interlock_test();
  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO interlock test FAILED (%d)\n", ret);
      return 7;
    }

  fprintf(stderr, "pipe_main: FIFO interlock test PASSED\n");

  fprintf(stderr, "pipe_main: FIFO test PASSED\n");

#else
  fprintf(stderr, "\npipe_main: Skipping FIFO test\n");

#endif /* CONFIG_DEV_FIFO_SIZE > 0 */

#if CONFIG_DEV_PIPE_SIZE > 0
  /* Test PIPE logic */

  fprintf(stderr, "\npipe_main: Performing pipe test\n");

  ret = pipe(fd);
  if (ret < 0)
    {
      fprintf(stderr, "pipe_main: pipe failed with errno=%d\n", errno);
      return 8;
    }

  /* Then perform the test using those file descriptors */

  ret = transfer_test(fd[0], fd[1], 0, 0);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: PIPE test FAILED (00) (%d)\n", ret);
      return 9;
    }

  ret = transfer_test(fd[0], fd[1], 1, 0);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: PIPE test FAILED (10) (%d)\n", ret);
      return 9;
    }

  ret = transfer_test(fd[0], fd[1], 0, 1);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: PIPE test FAILED (01) (%d)\n", ret);
      return 9;
    }

  ret = transfer_test(fd[0], fd[1], 1, 1);

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: PIPE test FAILED (11) (%d)\n", ret);
      return 9;
    }

  if (close(fd[0]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }

  if (close(fd[1]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }

  /* Perform the pipe redirection test */

  fprintf(stderr, "\npipe_main: Performing redirection test\n");
  ret = redirection_test();
  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: PIPE redirection test FAILED (%d)\n", ret);
      return 10;
    }

  fprintf(stderr, "pipe_main: PIPE redirection test PASSED\n");

  fprintf(stderr, "pipe_main: PIPE test PASSED\n");

#else
  fprintf(stderr, "\npipe_main: Skipping pipe test\n");

#endif /* CONFIG_DEV_PIPE_SIZE > 0 */

  fflush(stderr);
  return 0;
}
