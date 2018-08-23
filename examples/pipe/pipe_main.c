/****************************************************************************
 * examples/pipe/pipe_main.c
 *
 *   Copyright (C) 2008-2009, 2011, 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int pipe_main(int argc, char *argv[])
#endif
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
