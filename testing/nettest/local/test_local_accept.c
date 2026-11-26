/****************************************************************************
 * apps/testing/nettest/local/test_local_accept.c
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

/* accept() of a local stream socket with SO_RCVTIMEO */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <cmocka.h>

#include "test_local.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_ACCEPT_TIMEO 300   /* ms, SO_RCVTIMEO of accept() */
#define TEST_ACCEPT_LATE  3000  /* ms before a client ends a wait */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* A client that connects after TEST_ACCEPT_LATE ms, unless the case is
 * done: an accept() that ignores its timeout still returns.
 */

struct test_local_late_s
{
  struct sockaddr_un addr;
  volatile bool done;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *test_local_late(FAR void *arg)
{
  FAR struct test_local_late_s *late = arg;
  int fd;
  int i;

  for (i = 0; i < TEST_ACCEPT_LATE / 100 && !late->done; i++)
    {
      usleep(100 * 1000);
    }

  if (!late->done)
    {
      fd = socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
      if (fd >= 0)
        {
          connect(fd, (FAR struct sockaddr *)&late->addr,
                  sizeof(late->addr));
          while (!late->done)
            {
              usleep(100 * 1000);
            }

          close(fd);
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_accept_rcvtimeo
 *
 * Description:
 *   accept() on a listener with SO_RCVTIMEO and no client fails with
 *   EAGAIN after the timeout (Linux).
 *
 ****************************************************************************/

void test_local_accept_rcvtimeo(FAR void **state)
{
  struct test_local_late_s late;
  struct timespec start;
  struct timespec end;
  struct timeval tv;
  pthread_t tid;
  int listener;
  long ms;
  int err;
  int fd;

  memset(&late, 0, sizeof(late));
  late.addr.sun_family = AF_LOCAL;
  snprintf(late.addr.sun_path, sizeof(late.addr.sun_path), "%s/accept.%d",
           P_tmpdir, getpid());
  unlink(late.addr.sun_path);

  listener = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_true(listener >= 0);
  assert_return_code(bind(listener, (FAR struct sockaddr *)&late.addr,
                          sizeof(late.addr)), errno);
  assert_return_code(listen(listener, 1), errno);
  tv.tv_sec  = 0;
  tv.tv_usec = TEST_ACCEPT_TIMEO * 1000;
  assert_return_code(setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);

  assert_int_equal(pthread_create(&tid, NULL, test_local_late, &late), 0);
  clock_gettime(CLOCK_MONOTONIC, &start);
  fd  = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
  err = errno;
  clock_gettime(CLOCK_MONOTONIC, &end);
  late.done = true;
  pthread_join(tid, NULL);
  if (fd >= 0)
    {
      close(fd);
    }

  close(listener);
  unlink(late.addr.sun_path);

  ms = (end.tv_sec - start.tv_sec) * 1000 +
       (end.tv_nsec - start.tv_nsec) / 1000000;
  assert_int_equal(fd, -1);
  assert_int_equal(err, EAGAIN);
  assert_in_range(ms, TEST_ACCEPT_TIMEO - 50, TEST_ACCEPT_LATE - 500);
}
