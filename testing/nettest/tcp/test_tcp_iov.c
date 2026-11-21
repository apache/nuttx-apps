/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_iov.c
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

/* readv() and recvmsg() on a TCP socket: the buffers of the iovec array
 * are one buffer.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_RCVTIMEO 1000  /* ms */
#define TEST_POLL     1000  /* ms, loopback answers at once */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_iov_state_s
{
  int fds[3];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_iov_send
 *
 * Description:
 *   Send the string and wait until the receiver has data.
 *
 ****************************************************************************/

static void tcp_iov_send(FAR struct tcp_iov_state_s *st, FAR const char *s)
{
  struct pollfd pfd;
  size_t len = strlen(s);

  assert_int_equal(send(st->fds[1], s, len, 0), len);
  pfd.fd      = st->fds[2];
  pfd.events  = POLLIN;
  pfd.revents = 0;
  assert_int_equal(poll(&pfd, 1, TEST_POLL), 1);

  /* Loopback sends the data in one segment, but give it time */

  usleep(10 * 1000);
}

/****************************************************************************
 * Name: tcp_iov_ms
 ****************************************************************************/

static long tcp_iov_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_iov_setup
 ****************************************************************************/

int test_tcp_iov_setup(FAR void **state)
{
  FAR struct tcp_iov_state_s *st = malloc(sizeof(*st));
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  struct timeval tv;
  struct pollfd pfd;

  assert_non_null(st);
  st->fds[0] = -1;
  st->fds[1] = -1;
  st->fds[2] = -1;
  *state = st;

  st->fds[0] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(st->fds[0], errno);
  nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  addr.sin_port = 0;
  assert_return_code(bind(st->fds[0], (FAR struct sockaddr *)&addr,
                          sizeof(addr)), errno);
  assert_return_code(listen(st->fds[0], 1), errno);
  assert_return_code(getsockname(st->fds[0], (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);

  st->fds[1] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(st->fds[1], errno);
  assert_return_code(connect(st->fds[1], (FAR struct sockaddr *)&addr,
                             addrlen), errno);
  pfd.fd      = st->fds[0];
  pfd.events  = POLLIN;
  pfd.revents = 0;
  assert_int_equal(poll(&pfd, 1, TEST_POLL), 1);
  st->fds[2] = accept4(st->fds[0], NULL, NULL, SOCK_CLOEXEC);
  assert_return_code(st->fds[2], errno);

  /* A receive that waits by mistake returns after this time */

  tv.tv_sec  = TEST_RCVTIMEO / 1000;
  tv.tv_usec = (TEST_RCVTIMEO % 1000) * 1000;
  assert_return_code(setsockopt(st->fds[2], SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
  return 0;
}

/****************************************************************************
 * Name: test_tcp_iov_teardown
 ****************************************************************************/

int test_tcp_iov_teardown(FAR void **state)
{
  FAR struct tcp_iov_state_s *st = *state;
  int i;

  for (i = 0; i < 3; i++)
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
 * Name: test_tcp_recv_iov_empty
 *
 * Description:
 *   An empty first buffer takes no data and does not end the receive.
 *
 ****************************************************************************/

void test_tcp_recv_iov_empty(FAR void **state)
{
  FAR struct tcp_iov_state_s *st = *state;
  struct iovec iov[2];
  struct msghdr msg;
  char a[4];
  char b[16];

  /* readv() */

  tcp_iov_send(st, "abcd");
  memset(b, 0, sizeof(b));
  iov[0].iov_base = a;
  iov[0].iov_len  = 0;
  iov[1].iov_base = b;
  iov[1].iov_len  = sizeof(b);
  assert_int_equal(readv(st->fds[2], iov, 2), 4);
  assert_memory_equal(b, "abcd", 4);

  /* recvmsg() MSG_DONTWAIT */

  tcp_iov_send(st, "efgh");
  memset(&msg, 0, sizeof(msg));
  memset(b, 0, sizeof(b));
  msg.msg_iov    = iov;
  msg.msg_iovlen = 2;
  assert_int_equal(recvmsg(st->fds[2], &msg, MSG_DONTWAIT), 4);
  assert_memory_equal(b, "efgh", 4);

  /* After the peer closes, the end of the stream */

  close(st->fds[1]);
  st->fds[1] = -1;
  usleep(100 * 1000);
  assert_int_equal(readv(st->fds[2], iov, 2), 0);
}

/****************************************************************************
 * Name: test_tcp_recv_iov_nowait
 *
 * Description:
 *   A readv() whose first buffer takes all the data returns at once: it
 *   does not wait for data for the second buffer.  Data that fill both
 *   buffers go to both.
 *
 ****************************************************************************/

void test_tcp_recv_iov_nowait(FAR void **state)
{
  FAR struct tcp_iov_state_s *st = *state;
  struct iovec iov[2];
  char a[4];
  char b[8];
  long t0;

  tcp_iov_send(st, "ijkl");
  iov[0].iov_base = a;
  iov[0].iov_len  = sizeof(a);
  iov[1].iov_base = b;
  iov[1].iov_len  = sizeof(b);
  t0 = tcp_iov_ms();
  assert_int_equal(readv(st->fds[2], iov, 2), 4);
  assert_in_range(tcp_iov_ms() - t0, 0, TEST_RCVTIMEO / 2);
  assert_memory_equal(a, "ijkl", 4);

  tcp_iov_send(st, "mnopqrstuvwx");
  assert_int_equal(readv(st->fds[2], iov, 2), 12);
  assert_memory_equal(a, "mnop", 4);
  assert_memory_equal(b, "qrstuvwx", 8);
}
