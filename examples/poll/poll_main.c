/****************************************************************************
 * apps/examples/poll/poll_main.c
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
#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <debug.h>

#include "poll_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: poll_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char buffer[64];
  ssize_t nbytes;
  pthread_t tid1;
  pthread_t tid2;
#ifdef HAVE_NETPOLL
  pthread_t tid3;
#endif
  int count;
  int fd1 = -1;
  int fd2 = -1;
  int ret;
  int exitcode = 0;

  /* Open FIFOs */

  printf("\npoll_main: Creating FIFO %s\n", FIFO_PATH1);
  ret = mkfifo(FIFO_PATH1, 0666);
  if (ret < 0)
    {
      printf("poll_main: mkfifo failed: %d\n", errno);
      exitcode = 1;
      goto errout;
    }

  printf("\npoll_main: Creating FIFO %s\n", FIFO_PATH2);
  ret = mkfifo(FIFO_PATH2, 0666);
  if (ret < 0)
    {
      printf("poll_main: mkfifo failed: %d\n", errno);
      exitcode = 2;
      goto errout;
    }

  /* Open the FIFOs for blocking, write */

  fd1 = open(FIFO_PATH1, O_WRONLY);
  if (fd1 < 0)
    {
      printf("poll_main: Failed to open FIFO %s for writing, errno=%d\n",
             FIFO_PATH1, errno);
      exitcode = 3;
      goto errout;
    }

  fd2 = open(FIFO_PATH2, O_WRONLY);
  if (fd2 < 0)
    {
      printf("poll_main: Failed to open FIFO %s for writing, errno=%d\n",
            FIFO_PATH2, errno);
      exitcode = 4;
      goto errout;
    }

  /* Start the listeners */

  printf("poll_main: Starting poll_listener thread\n");

  ret = pthread_create(&tid1, NULL, poll_listener, NULL);
  if (ret != 0)
    {
      printf("poll_main: Failed to create poll_listener thread: %d\n", ret);
      exitcode = 5;
      goto errout;
    }

  printf("poll_main: Starting select_listener thread\n");

  ret = pthread_create(&tid2, NULL, select_listener, NULL);
  if (ret != 0)
    {
      printf("poll_main: Failed to create select_listener thread: %d\n", ret);
      exitcode = 6;
      goto errout;
    }

#ifdef HAVE_NETPOLL
#ifdef CONFIG_NET_TCPBACKLOG
  printf("poll_main: Starting net_listener thread\n");

  ret = pthread_create(&tid3, NULL, net_listener, NULL);
#else
  printf("poll_main: Starting net_reader thread\n");

  ret = pthread_create(&tid3, NULL, net_reader, NULL);
#endif
  if (ret != 0)
    {
      printf("poll_main: Failed to create net_listener thread: %d\n", ret);
    }
#endif

  /* Loop forever */

  for (count = 0; ; count++)
    {
      /* Send a message to the listener... this should wake the listener
       * from the poll.
       */

      sprintf(buffer, "Message %d", count);
      nbytes = write(fd1, buffer, strlen(buffer));
      if (nbytes < 0)
        {
          printf("poll_main: Write to fd1 failed: %d\n", errno);
          exitcode = 7;
          goto errout;
        }

      nbytes = write(fd2, buffer, strlen(buffer));
      if (nbytes < 0)
        {
          printf("poll_main: Write fd2 failed: %d\n", errno);
          exitcode = 8;
          goto errout;
        }

      printf("\npoll_main: Sent '%s' (%ld bytes)\n",
             buffer, (long)nbytes);
      fflush(stdout);

      /* Wait awhile.  This delay should be long enough that the
       * listener will timeout.
       */

      sleep(WRITER_DELAY);
    }

errout:
  if (fd1 >= 0)
    {
      close(fd1);
    }

  if (fd2 >= 0)
    {
      close(fd2);
    }

  fflush(stdout);
  return exitcode;
}
