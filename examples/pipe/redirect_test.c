/****************************************************************************
 * apps/examples/pipe/redirect_test.c
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
#include <sched.h>
#include <semaphore.h>
#include <errno.h>

#include "pipe.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define READ_SIZE 37

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_rddone;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: redirect_reader
 ****************************************************************************/

static int redirect_reader(int argc, char *argv[])
{
  char buffer[READ_SIZE];
  int fdin;
  int fdout;
  int ret;
  int nbytes = 0;

  printf("redirect_reader: started with fdin=%s\n", argv[1]);

  /* Convert the fdin to binary */

  fdin  = atoi(argv[1]);
  fdout = atoi(argv[2]);

  /* Close fdout -- we don't need it */

  ret = close(fdout);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: failed to close fdout=%d\n",
              fdout);
      return 1;
    }

  /* Re-direct the fdin to stdin */

  ret = dup2(fdin, 0);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: dup2 failed: %d\n", errno);
      close(fdin);
      return 2;
    }

  /* Close the original file descriptor */

  ret = close(fdin);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: failed to close fdin=%d\n", fdin);
      return 3;
    }

  /* Then read from stdin until we hit the end of file */

  fflush(stdout);
  for (; ; )
    {
      /* Read from stdin */

      ret = read(0, buffer, READ_SIZE);
      if (ret < 0)
        {
          fprintf(stderr, "redirect_reader: read failed, errno=%d\n",
                  errno);
          return 4;
        }
      else if (ret == 0)
        {
          break;
        }

      nbytes += ret;

      /* Echo to stdout */

      ret = write(1, buffer, ret);
      if (ret < 0)
        {
          fprintf(stderr, "redirect_reader: read failed, errno=%d\n",
                  errno);
          return 5;
        }
    }

  printf("redirect_reader: %d bytes read\n", nbytes);
  ret = close(0);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: failed to close fd=0\n");
      return 6;
    }

  sem_post(&g_rddone);
  printf("redirect_reader: Returning success\n");
  return 0;
}

/****************************************************************************
 * Name: redirect_writer
 ****************************************************************************/

static int redirect_writer(int argc, char *argv[])
{
  int fdin;
  int fdout;
  int nbytes = 0;
  int ret;

  fprintf(stderr, "redirect_writer: started with fdout=%s\n", argv[2]);

  /* Convert the fdout to binary */

  fdin  = atoi(argv[1]);
  fdout = atoi(argv[2]);

  /* Close fdin -- we don't need it */

  ret = close(fdin);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: failed to close fdin=%d\n", fdin);
      return 1;
    }

  /* Re-direct the fdout to stdout */

  ret = dup2(fdout, 1);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_writer: dup2 failed: %d\n", errno);
      return 2;
    }

  /* Close the original file descriptor */

  ret = close(fdout);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_reader: failed to close fdout=%d\n",
              fdout);
      return 3;
    }

  /* Then write a bunch of stuff to stdout */

  fflush(stderr);
  nbytes += printf("\nFour score and seven years ago our fathers brought forth on this continent a new nation,\n");
  nbytes += printf("conceived in Liberty, and dedicated to the proposition that all men are created equal.\n");
  nbytes += printf("\nNow we are engaged in a great civil war, testing whether that nation, or any nation, so\n");
  nbytes += printf("conceived and so dedicated, can long endure. We are met on a great battle-field of that war.\n");
  nbytes += printf("We have come to dedicate a portion of that field, as a final resting place for those who here\n");
  nbytes += printf("gave their lives that that nation might live. It is altogether fitting and proper that we\n");
  nbytes += printf("should do this.\n");
  nbytes += printf("\nBut, in a larger sense, we can not dedicate - we can not consecrate - we can not hallow - this ground.\n");
  nbytes += printf("The brave men, living and dead, who struggled here, have consecrated it, far above our poor power\n");
  nbytes += printf("to add or detract. The world will little note, nor long remember what we say here, but it can\n");
  nbytes += printf("never forget what they did here. It is for us the living, rather, to be dedicated here to the\n");
  nbytes += printf("unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to\n");
  nbytes += printf("be here dedicated to the great task remaining before us - that from these honored dead we take\n");
  nbytes += printf("increased devotion to that cause for which they gave the last full measure of devotion - that we\n");
  nbytes += printf("here highly resolve that these dead shall not have died in vain - that this nation, under God,\n");
  nbytes += printf("shall have a new birth of freedom - and that government of the people, by the people, for the\n");
  nbytes += printf("people, shall not perish from the earth.\n\n");
  fflush(stdout);

  fprintf(stderr, "redirect_writer: %d bytes written\n", nbytes);

  ret = close(1);
  if (ret != 0)
    {
      fprintf(stderr, "redirect_writer: failed to close fd=1\n");
      return 4;
    }

  fprintf(stderr, "redirect_writer: Returning success\n");
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: redirection_test
 ****************************************************************************/

int redirection_test(void)
{
  char *argv[3];
  char buffer1[8];
  char buffer2[8];
  int readerid;
  int writerid;
  int fd[2];
  int ret;

  sem_init(&g_rddone, 0, 0);

  /* Create the pipe */

  ret = pipe(fd);
  if (ret < 0)
    {
      fprintf(stderr, "redirection_test: pipe failed with errno=%d\n",
              errno);
      return 5;
    }

  sprintf(buffer1, "%d", fd[0]);
  argv[0] = buffer1;
  sprintf(buffer2, "%d", fd[1]);
  argv[1] = buffer2;
  argv[2] = NULL;

  /* Start redirect_reader thread */

  printf("redirection_test: Starting redirect_reader task with fd=%d\n",
         fd[0]);
  readerid = task_create("redirect_reader",
                         50, CONFIG_EXAMPLES_PIPE_STACKSIZE,
                         redirect_reader, argv);
  if (readerid < 0)
    {
      fprintf(stderr, "redirection_test: "
              "Failed to create redirect_writer task: %d\n", errno);
      return 1;
    }

  /* Start redirect_writer task */

  printf("redirection_test: Starting redirect_writer task with fd=%d\n",
         fd[1]);
  writerid = task_create("redirect_writer",
                         50, CONFIG_EXAMPLES_PIPE_STACKSIZE,
                         redirect_writer, argv);
  if (writerid < 0)
    {
      fprintf(stderr, "redirection_test: "
              "Failed to create redirect_writer task: %d\n", errno);

      ret = task_delete(readerid);
      if (ret != 0)
        {
          fprintf(stderr, "redirection_test: "
                  "Failed to delete redirect_reader task %d\n", errno);
        }

      return 2;
    }

  /* We should be able to close the pipe file descriptors now. */

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

  /* Wait for redirect_writer thread to complete */

  printf("redirection_test: Waiting...\n");
  fflush(stdout);
  sem_wait(&g_rddone);

  printf("redirection_test: returning %d\n", ret);
  return ret;
}
