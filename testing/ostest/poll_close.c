/****************************************************************************
 * apps/testing/ostest/poll_close.c
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

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "ostest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Close the descriptor while the main thread is in poll() on it */

static FAR void *close_thread(FAR void *arg)
{
  usleep(100000);
  close((int)(intptr_t)arg);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: poll_close_test
 *
 * Description:
 *   Another thread closes the read end of a pipe during poll().  When
 *   poll() ends the read end must be gone, so the write end reports POLLERR,
 *   and no driver may keep a pointer to the entry of that poll().
 *
 ****************************************************************************/

int poll_close_test(void)
{
  struct pollfd pfd;
  pthread_t thread;
  int fd[2];
  int ret;

  ret = pipe(fd);
  if (ret < 0)
    {
      printf("poll_close_test: ERROR: pipe failed: %d\n", errno);
      ASSERT(false);
    }

  ret = pthread_create(&thread, NULL, close_thread,
                       (FAR void *)(intptr_t)fd[0]);
  if (ret != 0)
    {
      printf("poll_close_test: ERROR: pthread_create failed: %d\n", ret);
      ASSERT(false);
    }

  pfd.fd      = fd[0];
  pfd.events  = POLLIN;
  pfd.revents = 0;
  poll(&pfd, 1, 300);
  pthread_join(thread, NULL);

  pfd.fd      = fd[1];
  pfd.events  = POLLOUT;
  pfd.revents = 0;
  ret = poll(&pfd, 1, 0);
  if (ret != 1 || (pfd.revents & POLLERR) == 0)
    {
      printf("poll_close_test: ERROR: poll of the writer: %d revents %x\n",
             ret, pfd.revents);
      ASSERT(false);
    }

  close(fd[1]);
  printf("poll_close_test: PASS\n");
  return 0;
}
