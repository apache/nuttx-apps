/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_halfclose.c
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

/* A half-closed IPv4 TCP connection on the loopback address: the client
 * sends a request and shuts down writing, the server reads it to the end,
 * sends an answer and closes.  The client must read the whole answer and
 * then end-of-file.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

#ifdef CONFIG_NET_SOCKOPTS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_HALFCLOSE_REQUEST 100         /* Bytes of the request */
#define TEST_HALFCLOSE_ANSWER  (16 * 1024) /* Bytes of the answer */
#define TEST_HALFCLOSE_CHUNK   1024        /* Bytes per send() and recv() */
#define TEST_HALFCLOSE_LIMIT   10          /* Seconds for each direction */
#define TEST_HALFCLOSE_NFDS    3

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_halfclose_state_s
{
  int fds[TEST_HALFCLOSE_NFDS];         /* The listener, client, accepted */
  volatile bool stop;                   /* Tells the server to stop */
  size_t sent;                          /* Bytes of the answer sent */
  int senderr;                          /* errno of a failed send(), or 0 */
  uint8_t sbuf[TEST_HALFCLOSE_CHUNK];   /* Buffer of the server */
  uint8_t rbuf[TEST_HALFCLOSE_CHUNK];   /* Buffer of the client */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_halfclose_byte
 ****************************************************************************/

static uint8_t tcp_halfclose_byte(size_t off)
{
  return (uint8_t)(off % 251);
}

/****************************************************************************
 * Name: tcp_halfclose_inlimit
 *
 * Description:
 *   True while less than TEST_HALFCLOSE_LIMIT seconds passed since start.
 *
 ****************************************************************************/

static bool tcp_halfclose_inlimit(FAR const struct timespec *start)
{
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  return (now.tv_sec - start->tv_sec) * 1000 +
         (now.tv_nsec - start->tv_nsec) / 1000000 <
         TEST_HALFCLOSE_LIMIT * 1000;
}

/****************************************************************************
 * Name: tcp_halfclose_timeouts
 *
 * Description:
 *   Let a blocking send() or recv() on fd return after one second.
 *
 ****************************************************************************/

static void tcp_halfclose_timeouts(int fd)
{
  struct timeval tv;

  tv.tv_sec  = 1;
  tv.tv_usec = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv,
                                sizeof(tv)), errno);
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
}

/****************************************************************************
 * Name: tcp_halfclose_server
 *
 * Description:
 *   Send the answer with blocking send() calls until all of it is sent,
 *   send() fails or st->stop is set, then close the accepted socket.
 *
 ****************************************************************************/

static FAR void *tcp_halfclose_server(FAR void *arg)
{
  FAR struct tcp_halfclose_state_s *st = arg;
  ssize_t ret;
  size_t len;
  size_t i;

  while (st->sent < TEST_HALFCLOSE_ANSWER && !st->stop)
    {
      len = TEST_HALFCLOSE_ANSWER - st->sent;
      if (len > TEST_HALFCLOSE_CHUNK)
        {
          len = TEST_HALFCLOSE_CHUNK;
        }

      for (i = 0; i < len; i++)
        {
          st->sbuf[i] = tcp_halfclose_byte(st->sent + i);
        }

      ret = send(st->fds[2], st->sbuf, len, 0);
      if (ret > 0)
        {
          st->sent += ret;
        }
      else if (ret < 0 && errno != EAGAIN && errno != ETIMEDOUT)
        {
          st->senderr = errno;
          break;
        }
    }

  close(st->fds[2]);
  st->fds[2] = -1;
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_halfclose_setup
 ****************************************************************************/

int test_tcp_halfclose_setup(FAR void **state)
{
  FAR struct tcp_halfclose_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TEST_HALFCLOSE_NFDS; i++)
    {
      st->fds[i] = -1;
    }

  st->stop    = false;
  st->sent    = 0;
  st->senderr = 0;

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_halfclose_teardown
 ****************************************************************************/

int test_tcp_halfclose_teardown(FAR void **state)
{
  FAR struct tcp_halfclose_state_s *st = *state;
  int i;

  for (i = 0; i < TEST_HALFCLOSE_NFDS; i++)
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
 * Name: test_tcp_halfclose_answer
 *
 * Description:
 *   The client sends TEST_HALFCLOSE_REQUEST bytes and calls
 *   shutdown(SHUT_WR).  The server reads them and end-of-file; a thread
 *   then sends TEST_HALFCLOSE_ANSWER bytes on the accepted socket and
 *   closes it.  The client must read every byte of the answer, in order,
 *   then end-of-file, within TEST_HALFCLOSE_LIMIT seconds.
 *
 ****************************************************************************/

void test_tcp_halfclose_answer(FAR void **state)
{
  FAR struct tcp_halfclose_state_s *st = *state;
  struct sockaddr_in addr;
  struct timespec start;
  socklen_t addrlen;
  pthread_t server;
  size_t recvd = 0;
  ssize_t bad = -1;
  ssize_t ret;
  bool eof = false;
  int recverr = 0;
  ssize_t i;

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
  tcp_halfclose_timeouts(st->fds[1]);
  tcp_halfclose_timeouts(st->fds[2]);

  /* The request, then the end of the client's data */

  for (i = 0; i < TEST_HALFCLOSE_REQUEST; i++)
    {
      st->sbuf[i] = tcp_halfclose_byte(i);
    }

  assert_int_equal(send(st->fds[1], st->sbuf, TEST_HALFCLOSE_REQUEST, 0),
                   TEST_HALFCLOSE_REQUEST);
  assert_return_code(shutdown(st->fds[1], SHUT_WR), errno);

  clock_gettime(CLOCK_MONOTONIC, &start);
  do
    {
      ret = recv(st->fds[2], st->rbuf, TEST_HALFCLOSE_CHUNK, 0);
      if (ret > 0)
        {
          recvd += ret;
        }
    }
  while ((ret > 0 || (ret < 0 && (errno == EAGAIN || errno == ETIMEDOUT))) &&
         tcp_halfclose_inlimit(&start));

  assert_int_equal(ret, 0);
  assert_int_equal(recvd, TEST_HALFCLOSE_REQUEST);

  /* The answer from a thread: the client reads it here */

  assert_int_equal(pthread_create(&server, NULL, tcp_halfclose_server, st),
                   0);

  /* No assert until the server thread has stopped */

  recvd = 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  while (tcp_halfclose_inlimit(&start))
    {
      ret = recv(st->fds[1], st->rbuf, TEST_HALFCLOSE_CHUNK, 0);
      if (ret < 0 && (errno == EAGAIN || errno == ETIMEDOUT))
        {
          continue;
        }

      if (ret <= 0)
        {
          eof = ret == 0;
          recverr = ret < 0 ? errno : 0;
          break;
        }

      for (i = 0; i < ret && bad < 0; i++)
        {
          if (st->rbuf[i] != tcp_halfclose_byte(recvd + i))
            {
              bad = recvd + i;
            }
        }

      recvd += ret;
    }

  st->stop = true;
  pthread_join(server, NULL);

  if (recvd != TEST_HALFCLOSE_ANSWER || bad >= 0 || !eof)
    {
      fail_msg("%zu of %d bytes of the answer received, %s (recv errno %d), "
               "%zu sent (send errno %d), first wrong byte %zd",
               recvd, TEST_HALFCLOSE_ANSWER, eof ? "end-of-file" : "no end",
               recverr, st->sent, st->senderr, bad);
    }
}

#endif /* CONFIG_NET_SOCKOPTS */
