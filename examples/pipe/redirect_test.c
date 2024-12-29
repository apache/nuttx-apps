/****************************************************************************
 * apps/examples/pipe/redirect_test.c
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <pthread.h>

#include "pipe.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define READ_SIZE 37

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: redirect_reader
 ****************************************************************************/

static void *redirect_reader(pthread_addr_t pvarg)
{
  char buffer[READ_SIZE];
  int *fd = (int *)pvarg;
  int fdin;
  int ret;
  int nbytes = 0;

  fprintf(stderr, "redirect_reader: started with fdin=%d\n", fd[0]);

  fdin  = fd[0];

  /* Re-direct the fdin to stdin */

  ret = dup2(fdin, 0);
  if (ret < 0)
    {
      fprintf(stderr, "redirect_reader: dup2 failed: %d\n", errno);
      close(fdin);
      return (void *)(uintptr_t)1;
    }

  /* Then read from stdin until we hit the end of file */

  for (; ; )
    {
      /* Read from stdin */

      ret = read(0, buffer, READ_SIZE);
      if (ret < 0)
        {
          fprintf(stderr, "redirect_reader: read failed, errno=%d\n", errno);
          return (void *)(uintptr_t)2;
        }
      else if (ret == 0)
        {
          break;
        }

      nbytes += ret;

      /* Echo to stdout */

      ret = write(2, buffer, ret);
      if (ret < 0)
        {
          fprintf(stderr, "redirect_reader: write failed, errno=%d\n",
                  errno);
          return (void *)(uintptr_t)3;
        }
    }

  fprintf(stderr, "redirect_reader: %d bytes read\n", nbytes);
  ret = close(0);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: failed to close fd=0\n");
      return (void *)(uintptr_t)4;
    }

  fprintf(stderr, "redirect_reader: Returning success\n");
  return NULL;
}

/****************************************************************************
 * Name: redirect_writer
 ****************************************************************************/

static void *redirect_writer(pthread_addr_t pvarg)
{
  int *fd = (int *)pvarg;
  int fdout;
  int nbytes = 0;
  int ret;

  fprintf(stderr, "redirect_writer: started with fdout=%d\n", fd[1]);

  fdout = fd[1];

  /* Re-direct the fdout to stdout */

  ret = dup2(fdout, 1);
  if (ret < 0)
    {
      fprintf(stderr, "redirect_writer: dup2 failed: %d\n", ret);
      return (void *)(uintptr_t)1;
    }

  /* Then write a bunch of stuff to stdout */

  fflush(stdout);
  nbytes += printf("\nFour score and seven years ago our fathers brought"
                   "forth on this continent a new nation,\n");
  nbytes += printf("conceived in Liberty, and dedicated to the proposition"
                   "that all men are created equal.\n");
  nbytes += printf("\nNow we are engaged in a great civil war, testing"
                   "whether that nation, or any nation, so\n");
  nbytes += printf("conceived and so dedicated, can long endure. We are met"
                   "on a great battle-field of that war.\n");
  nbytes += printf("We have come to dedicate a portion of that field, as a"
                   "final resting place for those who here\n");
  nbytes += printf("gave their lives that that nation might live. It is"
                   "altogether fitting and proper that we\n");
  nbytes += printf("should do this.\n");
  nbytes += printf("\nBut, in a larger sense, we can not dedicate - we can"
                   "not consecrate - we can not hallow - this ground.\n");
  nbytes += printf("The brave men, living and dead, who struggled here, have"
                   "consecrated it, far above our poor power\n");
  nbytes += printf("to add or detract. The world will little note, nor long"
                   "remember what we say here, but it can\n");
  nbytes += printf("never forget what they did here. It is for us the"
                   "living, rather, to be dedicated here to the\n");
  nbytes += printf("unfinished work which they who fought here have thus far"
                   "so nobly advanced. It is rather for us to\n");
  nbytes += printf("be here dedicated to the great task remaining before us"
                   "- that from these honored dead we take\n");
  nbytes += printf("increased devotion to that cause for which they gave the"
                   "last full measure of devotion - that we\n");
  nbytes += printf("here highly resolve that these dead shall not have died"
                   "in vain - that this nation, under God,\n");
  nbytes += printf("shall have a new birth of freedom - and that government"
                   "of the people, by the people, for the\n");
  nbytes += printf("people, shall not perish from the earth.\n\n");
  fflush(stdout);

  fprintf(stderr, "redirect_writer: %d bytes written\n", nbytes);

  ret = close(1);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_writer: failed to close fd=1\n");
      return (void *)(uintptr_t)2;
    }

  ret = close(fdout);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_writer: failed to close fdout\n");
      return (void *)(uintptr_t)4;
    }

  fprintf(stderr, "redirect_writer: Returning success\n");
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: redirection_test
 ****************************************************************************/

int redirection_test(void)
{
  pthread_t readerid;
  pthread_t writerid;
  void *value;
  int fd[2];
  int ret;

  /* Create the pipe */

  ret = pipe(fd);
  if (ret < 0)
    {
      fprintf(stderr, "redirection_test: pipe failed with errno=%d\n",
              errno);
      return 1;
    }

  /* Start redirect_writer task */

  fprintf(stderr,
          "redirection_test: Starting redirect_writer task with fd=%d\n",
          fd[1]);
  ret = pthread_create(&writerid, NULL, redirect_writer, fd);
  if (ret < 0)
    {
      fprintf(stderr, "redirection_test: "
              "Failed to create redirect_writer task: %d\n", errno);
      ret = pthread_cancel(writerid);
      if (ret != 0)
        {
          fprintf(stderr, "redirection_test: "
                  "Failed to delete redirect_reader task %d\n", errno);
        }

      return 2;
    }

  /* Start redirect_reader thread */

  fprintf(stderr,
          "redirection_test: Starting redirect_reader task with fd=%d\n",
          fd[0]);
  ret = pthread_create(&readerid, NULL, redirect_reader, fd);

  if (ret < 0)
    {
      fprintf(stderr, "redirection_test: "
              "Failed to create redirect_writer task: %d\n", ret);
      return 3;
    }

  /* Wait for redirect_reader thread to complete */

  fprintf(stderr, "redirection_test: Waiting for null_reader thread\n");
  ret = pthread_join(readerid, &value);
  if (ret != 0)
    {
      fprintf(stderr, \
        "interlock_test: pthread_join failed, error=%d\n", ret);
      ret = 4;
    }
  else
    {
      printf("interlock_test: reader returned %p\n", value);
      if (value != NULL)
        {
          ret = 5;
        }
    }

  /* We should be able to close the pipe file descriptors now. */

  if (close(fd[0]) != 0)
    {
      fprintf(stderr, "pipe_main: close failed: %d\n", errno);
    }

  /* Wait for redirect_writer thread to complete */

  fprintf(stderr, "redirection_test: Waiting...\n");
  fflush(stdout);

  fprintf(stderr, "redirection_test: returning %d\n", ret);
  return ret;
}
