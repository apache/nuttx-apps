/****************************************************************************
 * apps/examples/poll/select_listener.c
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
#include <sys/select.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
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
 * Name: select_listener
 ****************************************************************************/

void *select_listener(pthread_addr_t pvarg)
{
  fd_set rfds;
  struct timeval tv;
  char buffer[64];
  ssize_t nbytes;
  bool timeout;
  bool ready;
  int fd;
  int ret;

  /* Open the FIFO for non-blocking read */

  printf("select_listener: Opening %s for non-blocking read\n", FIFO_PATH2);
  fd = open(FIFO_PATH2, O_RDONLY|O_NONBLOCK);
  if (fd < 0)
    {
      printf("select_listener: ERROR Failed to open FIFO %s: %d\n",
             FIFO_PATH2, errno);
      close(fd);
      return (void*)-1;
    }

  /* Loop forever */

  for (;;)
    {
      printf("select_listener: Calling select()\n");

      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);

      tv.tv_sec  = SELECT_LISTENER_DELAY;
      tv.tv_usec = 0;

      timeout    = false;
      ready      = false;

      ret = select(fd+1, (FAR fd_set*)&rfds, (FAR fd_set*)NULL, (FAR fd_set*)NULL, &tv);
      printf("\nselect_listener: select returned: %d\n", ret);

      if (ret < 0)
        {
          printf("select_listener: ERROR select failed: %d\n", errno);
        }
      else if (ret == 0)
        {
          printf("select_listener: Timeout\n");
          timeout = true;
        }
      else
        {
          if (ret != 1)
            {
              printf("select_listener: ERROR poll reported: %d\n", ret);
            }
          else
            {
              ready = true;
            }

          if (!FD_ISSET(fd, &rfds))
            {
              printf("select_listener: ERROR fd=%d not in fd_set\n", fd);
            }
        }

      /* In any event, read until the pipe is empty */

      do
        {
          nbytes = read(fd, buffer, 63);
          if (nbytes <= 0)
            {
              if (nbytes == 0 || errno == EAGAIN)
                {
                  if (ready)
                    {
                      printf("select_listener: ERROR no read data\n");
                    }
                }
              else if (errno != EINTR)
                {
                  printf("select_listener: read failed: %d\n", errno);
                }
              nbytes = 0;
            }
          else
            {
              if (timeout)
                {
                  printf("select_listener: ERROR? Poll timeout, but data read\n");
                  printf("               (might just be a race condition)\n");
                }

              buffer[nbytes] = '\0';
              printf("select_listener: Read '%s' (%ld bytes)\n", buffer, (long)nbytes);
            }

          timeout = false;
          ready   = false;
        }
      while (nbytes > 0);

      /* Make sure that everything is displayed */

      fflush(stdout);
    }

  /* Won't get here */

  close(fd);
  return NULL;
}
