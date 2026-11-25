/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_epoll.c
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

/* EPOLLET on a TCP connection over 127.0.0.1: an epoll set reports each
 * new event once, and reports no event when nothing changed.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_EPOLL_WAIT    1000  /* ms, time for an event that must come */
#define TEST_EPOLL_QUIET   300   /* ms, time without an event */
#define TEST_EPOLL_ROUNDS  5
#define TEST_EPOLL_BUFSIZE 2048  /* SO_SNDBUF and SO_RCVBUF, bytes */
#define TEST_EPOLL_NSEND   256   /* Limit of the sends that fill a buffer */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_epoll_state_s
{
  int fds[3];  /* The listener, the client, the accepted socket */
  int epfd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_epoll_add
 *
 * Description:
 *   Add 'fd' to the epoll set with 'events'.
 *
 ****************************************************************************/

static void tcp_epoll_add(FAR struct tcp_epoll_state_s *st, int fd,
                          uint32_t events)
{
  struct epoll_event ev;

  memset(&ev, 0, sizeof(ev));
  ev.events  = events;
  ev.data.fd = fd;
  assert_return_code(epoll_ctl(st->epfd, EPOLL_CTL_ADD, fd, &ev), errno);
}

/****************************************************************************
 * Name: tcp_epoll_wait
 *
 * Description:
 *   Wait at most 'ms' milliseconds for one event.  Return the result of
 *   epoll_wait() and the events in 'events'.
 *
 ****************************************************************************/

static int tcp_epoll_wait(FAR struct tcp_epoll_state_s *st, int ms,
                          FAR uint32_t *events)
{
  struct epoll_event ev;
  int ret;

  memset(&ev, 0, sizeof(ev));
  ret = epoll_wait(st->epfd, &ev, 1, ms);
  *events = ev.events;
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_epoll_setup
 ****************************************************************************/

int test_tcp_epoll_setup(FAR void **state)
{
  FAR struct tcp_epoll_state_s *st = malloc(sizeof(*st));
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  struct timeval tv;
  struct pollfd pfd;

  assert_non_null(st);
  st->fds[0] = -1;
  st->fds[1] = -1;
  st->fds[2] = -1;
  st->epfd   = -1;
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
  assert_int_equal(poll(&pfd, 1, TEST_EPOLL_WAIT), 1);
  st->fds[2] = accept4(st->fds[0], NULL, NULL, SOCK_CLOEXEC);
  assert_return_code(st->fds[2], errno);

  /* A receive that waits by mistake returns after this time */

  tv.tv_sec  = TEST_EPOLL_WAIT / 1000;
  tv.tv_usec = (TEST_EPOLL_WAIT % 1000) * 1000;
  assert_return_code(setsockopt(st->fds[2], SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);

  st->epfd = epoll_create1(EPOLL_CLOEXEC);
  assert_return_code(st->epfd, errno);
  return 0;
}

/****************************************************************************
 * Name: test_tcp_epoll_teardown
 ****************************************************************************/

int test_tcp_epoll_teardown(FAR void **state)
{
  FAR struct tcp_epoll_state_s *st = *state;
  int i;

  if (st->epfd >= 0)
    {
      close(st->epfd);
    }

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
 * Name: test_tcp_epollet_in
 *
 * Description:
 *   An EPOLLIN | EPOLLET socket is reported once for each new data, and
 *   not again until more data comes.
 *
 ****************************************************************************/

void test_tcp_epollet_in(FAR void **state)
{
  FAR struct tcp_epoll_state_s *st = *state;
  uint32_t events;
  char buf[8];
  int i;

  tcp_epoll_add(st, st->fds[2], EPOLLIN | EPOLLET);
  assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_QUIET, &events), 0);

  for (i = 0; i < TEST_EPOLL_ROUNDS; i++)
    {
      assert_int_equal(send(st->fds[1], "x", 1, 0), 1);
      assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_WAIT, &events), 1);
      assert_true((events & EPOLLIN) != 0);
      assert_int_equal(recv(st->fds[2], buf, sizeof(buf), 0), 1);
      assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_QUIET, &events), 0);
    }
}

/****************************************************************************
 * Name: test_tcp_epollet_out
 *
 * Description:
 *   An idle EPOLLOUT | EPOLLET socket is reported once.  Data that it
 *   receives makes the device poll the connection, and this is no event.
 *
 ****************************************************************************/

void test_tcp_epollet_out(FAR void **state)
{
  FAR struct tcp_epoll_state_s *st = *state;
  uint32_t events;
  char buf[8];
  int i;

  tcp_epoll_add(st, st->fds[2], EPOLLOUT | EPOLLET);
  assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_WAIT, &events), 1);
  assert_true((events & EPOLLOUT) != 0);

  for (i = 0; i < TEST_EPOLL_ROUNDS; i++)
    {
      assert_int_equal(send(st->fds[1], "y", 1, 0), 1);
      assert_int_equal(recv(st->fds[2], buf, sizeof(buf), 0), 1);
      assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_QUIET, &events), 0);
    }
}

/****************************************************************************
 * Name: test_tcp_epollet_eagain
 *
 * Description:
 *   An EPOLLOUT | EPOLLET socket fills its send buffer until send()
 *   fails with EAGAIN.  When the peer reads the data, the socket is
 *   reported again.
 *
 ****************************************************************************/

#if defined(CONFIG_NET_TCP_WRITE_BUFFERS) && defined(CONFIG_NET_SOCKOPTS)
void test_tcp_epollet_eagain(FAR void **state)
{
  FAR struct tcp_epoll_state_s *st = *state;
  int size = TEST_EPOLL_BUFSIZE;
  uint32_t events;
  char buf[256];
  ssize_t ret = 0;
  int n;
  int i;

  assert_return_code(setsockopt(st->fds[1], SOL_SOCKET, SO_SNDBUF, &size,
                                sizeof(size)), errno);
  assert_return_code(setsockopt(st->fds[2], SOL_SOCKET, SO_RCVBUF, &size,
                                sizeof(size)), errno);

  tcp_epoll_add(st, st->fds[1], EPOLLOUT | EPOLLET);
  assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_WAIT, &events), 1);
  assert_true((events & EPOLLOUT) != 0);

  /* The peer does not read: the send buffer fills */

  memset(buf, 'z', sizeof(buf));
  for (i = 0; i < TEST_EPOLL_NSEND; i++)
    {
      ret = send(st->fds[1], buf, sizeof(buf), MSG_DONTWAIT);
      if (ret < 0)
        {
          break;
        }
    }

  assert_int_equal(ret, -1);
  assert_int_equal(errno, EAGAIN);

  /* Let the segments in flight finish, and take their events */

  usleep(TEST_EPOLL_QUIET * 1000);
  do
    {
      n = tcp_epoll_wait(st, 0, &events);
    }
  while (n > 0);

  /* The peer reads: the queued data goes out, and its ACK makes room */

  do
    {
      ret = recv(st->fds[2], buf, sizeof(buf), MSG_DONTWAIT);
    }
  while (ret > 0);

  assert_int_equal(tcp_epoll_wait(st, TEST_EPOLL_WAIT, &events), 1);
  assert_true((events & EPOLLOUT) != 0);
}
#endif
