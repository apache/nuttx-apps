/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_bulk.c
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

/* A large transfer over an IPv4 TCP connection on the loopback address.
 * A thread sends with blocking send() calls and fills the write buffers,
 * so the received data must find IOBs while the write buffers hold most
 * of them.
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

#define TEST_BULK_SIZE   (4 * 1024 * 1024) /* Bytes to transfer */
#define TEST_BULK_CHUNK  4096              /* Bytes per send() and recv() */
#define TEST_BULK_LIMIT  30                /* Seconds for the transfer */
#define TEST_BULK_NFDS   3

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_bulk_state_s
{
  int fds[TEST_BULK_NFDS];        /* The listener, the client, the accepted */
  volatile bool stop;             /* Tells the sender to stop */
  size_t sent;                    /* Bytes that send() took */
  int senderr;                    /* errno of a failed send(), or 0 */
  uint8_t sbuf[TEST_BULK_CHUNK];  /* Buffer of the sender */
  uint8_t rbuf[TEST_BULK_CHUNK];  /* Buffer of the receiver */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_bulk_byte
 ****************************************************************************/

static uint8_t tcp_bulk_byte(size_t off)
{
  return (uint8_t)(off % 251);
}

/****************************************************************************
 * Name: tcp_bulk_timeouts
 *
 * Description:
 *   Let a blocking send() or recv() on fd return after one second.
 *
 ****************************************************************************/

static void tcp_bulk_timeouts(int fd)
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
 * Name: tcp_bulk_sender
 *
 * Description:
 *   Send TEST_BULK_SIZE bytes with blocking send() calls until all are
 *   sent, send() fails or st->stop is set.
 *
 ****************************************************************************/

static FAR void *tcp_bulk_sender(FAR void *arg)
{
  FAR struct tcp_bulk_state_s *st = arg;
  ssize_t ret;
  size_t len;
  size_t i;

  while (st->sent < TEST_BULK_SIZE && !st->stop)
    {
      len = TEST_BULK_SIZE - st->sent;
      if (len > TEST_BULK_CHUNK)
        {
          len = TEST_BULK_CHUNK;
        }

      for (i = 0; i < len; i++)
        {
          st->sbuf[i] = tcp_bulk_byte(st->sent + i);
        }

      ret = send(st->fds[1], st->sbuf, len, 0);
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

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_bulk_setup
 ****************************************************************************/

int test_tcp_bulk_setup(FAR void **state)
{
  FAR struct tcp_bulk_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TEST_BULK_NFDS; i++)
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
 * Name: test_tcp_bulk_teardown
 ****************************************************************************/

int test_tcp_bulk_teardown(FAR void **state)
{
  FAR struct tcp_bulk_state_s *st = *state;
  int i;

  for (i = 0; i < TEST_BULK_NFDS; i++)
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
 * Name: test_tcp_loopback_bulk
 *
 * Description:
 *   A thread sends TEST_BULK_SIZE bytes to another socket over 127.0.0.1
 *   with blocking send() calls; this thread reads them.  All bytes must
 *   arrive in order within TEST_BULK_LIMIT seconds.  The sender always
 *   stops before the case ends.
 *
 ****************************************************************************/

void test_tcp_loopback_bulk(FAR void **state)
{
  FAR struct tcp_bulk_state_s *st = *state;
  struct sockaddr_in addr;
  struct timespec start;
  struct timespec now;
  socklen_t addrlen;
  pthread_t sender;
  size_t recvd = 0;
  ssize_t bad = -1;
  ssize_t ret;
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
  tcp_bulk_timeouts(st->fds[1]);
  tcp_bulk_timeouts(st->fds[2]);

  assert_int_equal(pthread_create(&sender, NULL, tcp_bulk_sender, st), 0);

  /* No assert until the sender has stopped */

  clock_gettime(CLOCK_MONOTONIC, &start);
  now = start;
  while (recvd < TEST_BULK_SIZE)
    {
      clock_gettime(CLOCK_MONOTONIC, &now);
      if (now.tv_sec - start.tv_sec > TEST_BULK_LIMIT)
        {
          break;
        }

      ret = recv(st->fds[2], st->rbuf, TEST_BULK_CHUNK, 0);
      if (ret < 0 && (errno == EAGAIN || errno == ETIMEDOUT))
        {
          continue;
        }

      if (ret <= 0)
        {
          recverr = ret < 0 ? errno : 0;
          break;
        }

      for (i = 0; i < ret && bad < 0; i++)
        {
          if (st->rbuf[i] != tcp_bulk_byte(recvd + i))
            {
              bad = recvd + i;
            }
        }

      recvd += ret;
    }

  st->stop = true;
  pthread_join(sender, NULL);

  if (recvd != TEST_BULK_SIZE || bad >= 0)
    {
      fail_msg("%zu of %d bytes received (recv errno %d), %zu sent "
               "(send errno %d), first wrong byte %zd, after %d s",
               recvd, TEST_BULK_SIZE, recverr, st->sent, st->senderr, bad,
               (int)(now.tv_sec - start.tv_sec));
    }
}
