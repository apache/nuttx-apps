/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_sndlimit.c
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

/* Rounds of "send a message, wait for a 1-byte reply" over 127.0.0.1.  A
 * message is larger than the send buffer of the socket, so send() waits on
 * the send limit in each round, and the last ACK of the round must wake it.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_SNDLIMIT_MSG     16384 /* Bytes of one message */
#define TEST_SNDLIMIT_TIME    10    /* Seconds of rounds */
#define TEST_SNDLIMIT_TIMEO   5     /* Send and receive time-out, seconds */
#define TEST_SNDLIMIT_ROUNDMS 2000  /* Limit of one round, milliseconds */
#define TEST_SNDLIMIT_NFDS    3

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_sndlimit_state_s
{
  int fds[TEST_SNDLIMIT_NFDS];       /* The listener, the client, the accepted */
  volatile bool stop;                /* Tells the server thread to stop */
  uint8_t sbuf[TEST_SNDLIMIT_MSG];   /* Buffer of the client */
  uint8_t rbuf[1024];                /* Buffer of the server thread */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_sndlimit_ms
 ****************************************************************************/

static long tcp_sndlimit_ms(FAR const struct timespec *from)
{
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  return (now.tv_sec - from->tv_sec) * 1000 +
         (now.tv_nsec - from->tv_nsec) / 1000000;
}

/****************************************************************************
 * Name: tcp_sndlimit_timeouts
 ****************************************************************************/

static void tcp_sndlimit_timeouts(int fd)
{
  struct timeval tv;

  tv.tv_sec  = TEST_SNDLIMIT_TIMEO;
  tv.tv_usec = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv,
                                sizeof(tv)), errno);
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
}

/****************************************************************************
 * Name: tcp_sndlimit_server
 *
 * Description:
 *   Read each message and answer it with one byte, until st->stop is set
 *   or a call fails.
 *
 ****************************************************************************/

static FAR void *tcp_sndlimit_server(FAR void *arg)
{
  FAR struct tcp_sndlimit_state_s *st = arg;
  uint8_t reply = 'r';
  size_t got;
  ssize_t ret;

  while (!st->stop)
    {
      for (got = 0; got < TEST_SNDLIMIT_MSG; got += ret)
        {
          ret = recv(st->fds[2], st->rbuf, sizeof(st->rbuf), 0);
          if (ret <= 0)
            {
              return NULL;
            }
        }

      if (send(st->fds[2], &reply, 1, 0) != 1)
        {
          return NULL;
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_sndlimit_setup
 ****************************************************************************/

int test_tcp_sndlimit_setup(FAR void **state)
{
  FAR struct tcp_sndlimit_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TEST_SNDLIMIT_NFDS; i++)
    {
      st->fds[i] = -1;
    }

  st->stop = false;
  memset(st->sbuf, 'm', sizeof(st->sbuf));
  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_sndlimit_teardown
 ****************************************************************************/

int test_tcp_sndlimit_teardown(FAR void **state)
{
  FAR struct tcp_sndlimit_state_s *st = *state;
  int i;

  for (i = 0; i < TEST_SNDLIMIT_NFDS; i++)
    {
      if (st->fds[i] >= 0)
        {
          close(st->fds[i]);
        }
    }

  free(st);
  return 0;
}

/****************************************************************************
 * Name: test_tcp_send_limit_wakeup
 *
 * Description:
 *   For TEST_SNDLIMIT_TIME seconds, send a message of TEST_SNDLIMIT_MSG
 *   bytes over 127.0.0.1 and wait for the 1-byte reply of a thread that
 *   reads it.  Each round must end within TEST_SNDLIMIT_ROUNDMS: a send()
 *   that waits on the send limit and is not woken when the last data is
 *   acknowledged waits until its time-out.  With CONFIG_SMP the ACK is
 *   often handled on another CPU while the sender is about to wait.
 *
 ****************************************************************************/

void test_tcp_send_limit_wakeup(FAR void **state)
{
  FAR struct tcp_sndlimit_state_s *st = *state;
  struct sockaddr_in addr;
  struct timespec start;
  struct timespec round;
  socklen_t addrlen;
  pthread_t server;
  unsigned long rounds = 0;
  long ms = 0;
  bool failed = false;
  uint8_t reply;
  size_t sent = 0;
  ssize_t ret = 0;
  int err = 0;

  st->fds[0] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_true(st->fds[0] >= 0);
  addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  addr.sin_port = 0;
  assert_return_code(bind(st->fds[0], (FAR struct sockaddr *)&addr,
                          addrlen), errno);
  assert_return_code(listen(st->fds[0], 1), errno);
  assert_return_code(getsockname(st->fds[0], (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);

  st->fds[1] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_true(st->fds[1] >= 0);
  assert_return_code(connect(st->fds[1], (FAR struct sockaddr *)&addr,
                             addrlen), errno);
  st->fds[2] = accept4(st->fds[0], NULL, NULL, SOCK_CLOEXEC);
  assert_true(st->fds[2] >= 0);
  tcp_sndlimit_timeouts(st->fds[1]);
  tcp_sndlimit_timeouts(st->fds[2]);

  assert_int_equal(pthread_create(&server, NULL, tcp_sndlimit_server, st),
                   0);

  /* No assert until the server thread has stopped */

  clock_gettime(CLOCK_MONOTONIC, &start);
  while (tcp_sndlimit_ms(&start) < TEST_SNDLIMIT_TIME * 1000)
    {
      clock_gettime(CLOCK_MONOTONIC, &round);
      for (sent = 0; sent < TEST_SNDLIMIT_MSG; sent += ret)
        {
          ret = send(st->fds[1], st->sbuf + sent, TEST_SNDLIMIT_MSG - sent,
                     0);
          if (ret <= 0)
            {
              err = ret < 0 ? errno : 0;
              break;
            }
        }

      if (sent == TEST_SNDLIMIT_MSG)
        {
          ret = recv(st->fds[1], &reply, 1, 0);
          if (ret != 1)
            {
              err = ret < 0 ? errno : 0;
            }
        }

      ms = tcp_sndlimit_ms(&round);
      if (sent != TEST_SNDLIMIT_MSG || ret != 1 ||
          ms > TEST_SNDLIMIT_ROUNDMS)
        {
          failed = true;
          break;
        }

      rounds++;
    }

  /* Wake up the server thread: shut down both directions */

  st->stop = true;
  shutdown(st->fds[1], SHUT_RDWR);
  shutdown(st->fds[2], SHUT_RDWR);
  pthread_join(server, NULL);

  if (failed && sent != TEST_SNDLIMIT_MSG)
    {
      fail_msg("round %lu: send() returned %zd (errno %d) after %zu of %d "
               "bytes and %ld ms", rounds + 1, ret, err, sent,
               TEST_SNDLIMIT_MSG, ms);
    }
  else if (failed && ret != 1)
    {
      fail_msg("round %lu: no reply, recv() returned %zd (errno %d) after "
               "%ld ms", rounds + 1, ret, err, ms);
    }
  else if (failed)
    {
      fail_msg("round %lu took %ld ms", rounds + 1, ms);
    }
}
