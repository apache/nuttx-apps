/****************************************************************************
 * apps/examples/pipe/pipe_main.c
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

#include "pipe.h"

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

  printf("\npipe_main: Performing FIFO test\n");

  ret = mkfifo(FIFO_PATH1, 0666);
  if (ret < 0)
    {
      fprintf(stderr, "pipe_main: mkfifo failed with errno=%d\n", errno);
      return 1;
    }

  /* Open one end of the FIFO for reading and the other end for writing.
   * NOTE: The following might not work on most FIFO implementations because
   * the attempt to open just one end of the FIFO for writing might block.
   * The NuttX FIFOs block only on open for read-only (see interlock_test()).
   */

  fd[1] = open(FIFO_PATH1, O_WRONLY);
  if (fd[1] < 0)
    {
      fprintf(stderr,
              "pipe_main: Failed to open FIFO %s for writing, errno=%d\n",
              FIFO_PATH1, errno);
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

  /* Then perform the test using those file descriptors */

  ret = transfer_test(fd[0], fd[1]);
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
      return 4;
    }

  printf("pipe_main: FIFO test PASSED\n");

#else
  printf("\npipe_main: Skipping FIFO test\n");

#endif /* CONFIG_DEV_FIFO_SIZE > 0 */

#if CONFIG_DEV_PIPE_SIZE > 0
  /* Test PIPE logic */

  printf("\npipe_main: Performing pipe test\n");

  ret = pipe(fd);
  if (ret < 0)
    {
      fprintf(stderr, "pipe_main: pipe failed with errno=%d\n", errno);
      return 5;
    }

  /* Then perform the test using those file descriptors */

  ret = transfer_test(fd[0], fd[1]);
  if (close(fd[0]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }
  if (close(fd[1]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }

  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: PIPE test FAILED (%d)\n", ret);
      return 6;
    }

  printf("pipe_main: PIPE test PASSED\n");

  /* Perform the FIFO interlock test */

  printf("\npipe_main: Performing pipe interlock test\n");
  ret = interlock_test();
  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO interlock test FAILED (%d)\n", ret);
      return 7;
    }

  printf("pipe_main: PIPE interlock test PASSED\n");

  /* Perform the pipe redirection test */

  printf("\npipe_main: Performing redirection test\n");
  ret = redirection_test();
  if (ret != 0)
    {
      fprintf(stderr, "pipe_main: FIFO redirection test FAILED (%d)\n", ret);
      return 7;
    }

  printf("pipe_main: PIPE redirection test PASSED\n");

#else
  printf("\npipe_main: Skipping pipe test\n");

#endif /* CONFIG_DEV_PIPE_SIZE > 0 */

  fflush(stdout);
  return 0;
}
