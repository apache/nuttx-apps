/****************************************************************************
 * apps/examples/poll/poll_listener.c
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
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <arpa/inet.h>

#include "poll_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_DEV_CONSOLE
#  define HAVE_CONSOLE
#  define NPOLLFDS 2
#  define CONSNDX  0
#  define FIFONDX  1
#else
#  undef  HAVE_CONSOLE
#  define NPOLLFDS 1
#  define FIFONDX  0
#endif

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
 * Name: poll_listener
 ****************************************************************************/

void *poll_listener(pthread_addr_t pvarg)
{
  struct pollfd fds[NPOLLFDS];
  char buffer[64];
  ssize_t nbytes;
  bool timeout;
  bool pollin;
  int nevents;
  int fd;
  int ret;
  int i;

  /* Open the FIFO for non-blocking read */

  printf("poll_listener: Opening %s for non-blocking read\n", FIFO_PATH1);

  fd = open(FIFO_PATH1, O_RDONLY | O_NONBLOCK);
  if (fd < 0)
    {
      printf("poll_listener: ERROR Failed to open FIFO %s: %d\n",
             FIFO_PATH1, errno);
      close(fd);
      return (FAR void *)-1;
    }

  /* Loop forever */

  for (; ; )
    {
      printf("poll_listener: Calling poll()\n");

      memset(fds, 0, sizeof(struct pollfd)*NPOLLFDS);
#ifdef HAVE_CONSOLE
      fds[CONSNDX].fd      = 0;
      fds[CONSNDX].events  = POLLIN;
      fds[CONSNDX].revents = 0;
#endif
      fds[FIFONDX].fd      = fd;
      fds[FIFONDX].events  = POLLIN;
      fds[FIFONDX].revents = 0;

      timeout              = false;
      pollin               = false;

      ret = poll(fds, NPOLLFDS, POLL_LISTENER_DELAY);

      printf("\npoll_listener: poll returned: %d\n", ret);
      if (ret < 0)
        {
          printf("poll_listener: ERROR poll failed: %d\n", errno);
        }
      else if (ret == 0)
        {
          printf("poll_listener: Timeout\n");
          timeout = true;
        }
      else if (ret > NPOLLFDS)
        {
          printf("poll_listener: ERROR poll reported: %d\n", errno);
        }
      else
        {
          pollin = true;
        }

      nevents = 0;
      for (i = 0; i < NPOLLFDS; i++)
        {
          printf("poll_listener: FIFO revents[%d]=%08" PRIx32 "\n", i,
                 fds[i].revents);
          if (timeout)
            {
              if (fds[i].revents != 0)
                {
                  printf("poll_listener: ERROR? expected revents=00, "
                         "received revents[%d]=%08" PRIx32 "\n",
                         i, fds[i].revents);
                }
            }
          else if (pollin)
            {
              if (fds[i].revents == POLLIN)
                {
                  nevents++;
                }
              else if (fds[i].revents != 0)
                {
                  printf("poll_listener: ERROR unexpected revents[%d]="
                         "%08" PRIx32 "\n", i, fds[i].revents);
                }
            }
        }

      if (pollin && nevents != ret)
        {
           printf("poll_listener: ERROR found %d events, "
                  "poll reported %d\n", nevents, ret);
        }

      /* In any event, read until the pipe/serial  is empty */

      for (i = 0; i < NPOLLFDS; i++)
        {
          do
            {
#ifdef HAVE_CONSOLE
              /* Hack to work around the fact that the console driver on the
               * simulator is always non-blocking.
               */

              if (i == CONSNDX)
                {
                  if ((fds[CONSNDX].revents & POLLIN) != 0)
                    {
                      buffer[0] = getchar();
                      nbytes = 1;
                    }
                  else
                    {
                      nbytes = 0;
                    }
                }
              else
#endif
                {
                  /* The pipe works differently, it returns whatever data
                   * it has available without blocking.
                   */

                  nbytes = read(fds[i].fd, buffer, 63);
                }

              if (nbytes <= 0)
                {
                  if (nbytes == 0 || errno == EAGAIN)
                    {
                      if ((fds[i].revents & POLLIN) != 0)
                        {
                          printf("poll_listener: ERROR no read"
                                 " data[%d]\n", i);
                        }
                    }
                  else if (errno != EINTR)
                    {
                      printf("poll_listener: read[%d] failed: %d\n",
                             i, errno);
                    }

                  nbytes = 0;
                }
              else
                {
                  if (timeout)
                    {
                      printf("poll_listener: ERROR? Poll timeout, "
                              "but data read[%d]\n", i);
                      printf("               (might just be "
                             "a race condition)\n");
                    }

                  buffer[nbytes] = '\0';
                  printf("poll_listener: Read[%d] '%s' (%ld bytes)\n",
                         i, buffer, (long)nbytes);
                }

              /* Suppress error report if no read data on the next
               * time through
               */

              fds[i].revents = 0;
            }
          while (nbytes > 0);
        }

      /* Make sure that everything is displayed */

      fflush(stdout);
    }

  /* Won't get here */

  close(fd);
  return NULL;
}
