/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_reset.c
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

/* A reset of the peer on an IPv4 TCP connection over the loopback address.
 * The peer makes the reset: it closes its socket with data unread (RFC
 * 1122, 4.2.2.13).  The first receive without data, the first send and
 * SO_ERROR give ECONNRESET, once; then a receive gives end-of-file and a
 * send EPIPE.  A RST that refuses a connection still gives ECONNREFUSED.
 * A RST after the FIN of the peer gives EPIPE in SO_ERROR (CLOSE_WAIT).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

#define TCP_RESET_MAXFDS  4
#define TCP_RESET_TIMEOUT 10    /* s, SO_RCVTIMEO and SO_SNDTIMEO */
#define TCP_RESET_WAIT    3000  /* ms for an event of the loopback device */
#define TCP_RESET_DELAY   200   /* ms before the peer of a waiting call
                                 * resets the connection */
#define TCP_RESET_DATA    5     /* Bytes that come before the reset */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The sockets of one case: the teardown closes the ones that a failed case
 * leaves open.
 */

struct tcp_reset_state_s
{
  int fds[TCP_RESET_MAXFDS];
};

/* A connection over 127.0.0.1 */

struct tcp_reset_pair_s
{
  int client;
  int server;
};

/* The peer of a call that waits: after TCP_RESET_DELAY ms it closes fd
 * (which resets the connection); before that it sends len bytes.
 */

struct tcp_reset_peer_s
{
  int fd;
  size_t len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ALL_SIGNALS
static volatile int g_tcp_reset_sigpipe;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_reset_keep
 ****************************************************************************/

static int tcp_reset_keep(FAR struct tcp_reset_state_s *st, int fd)
{
  int i;

  assert_true(fd >= 0);
  for (i = 0; i < TCP_RESET_MAXFDS; i++)
    {
      if (st->fds[i] < 0)
        {
          st->fds[i] = fd;
          return fd;
        }
    }

  fail();
  return -1;
}

/****************************************************************************
 * Name: tcp_reset_forget
 *
 * Description:
 *   Take fd out of the list of the case: another thread closes it.
 *
 ****************************************************************************/

static void tcp_reset_forget(FAR struct tcp_reset_state_s *st, int fd)
{
  int i;

  for (i = 0; i < TCP_RESET_MAXFDS; i++)
    {
      if (st->fds[i] == fd)
        {
          st->fds[i] = -1;
          return;
        }
    }

  fail();
}

/****************************************************************************
 * Name: tcp_reset_fd
 ****************************************************************************/

static void tcp_reset_fd(FAR struct tcp_reset_state_s *st, int fd)
{
  tcp_reset_forget(st, fd);
  assert_return_code(close(fd), errno);
}

/****************************************************************************
 * Name: tcp_reset_timeouts
 *
 * Description:
 *   Give send() and recv() on fd a timeout, so that a failing case ends.
 *
 ****************************************************************************/

static void tcp_reset_timeouts(int fd)
{
#ifdef CONFIG_NET_SOCKOPTS
  struct timeval tv;

  tv.tv_sec  = TCP_RESET_TIMEOUT;
  tv.tv_usec = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv,
                                sizeof(tv)), errno);
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
#endif
}

/****************************************************************************
 * Name: tcp_reset_poll
 *
 * Description:
 *   Wait up to TCP_RESET_WAIT ms for events on fd.  Return revents.
 *
 ****************************************************************************/

static int tcp_reset_poll(int fd, short events)
{
  struct pollfd pfd;

  pfd.fd      = fd;
  pfd.events  = events;
  pfd.revents = 0;
  assert_return_code(poll(&pfd, 1, TCP_RESET_WAIT), errno);
  return pfd.revents;
}

/****************************************************************************
 * Name: tcp_reset_pair
 *
 * Description:
 *   Connect two sockets over 127.0.0.1, with send and receive timeouts.
 *   The server got one byte from the client and does not read it: its
 *   close() resets the connection.
 *
 ****************************************************************************/

static void tcp_reset_pair(FAR struct tcp_reset_state_s *st,
                           FAR struct tcp_reset_pair_s *pair)
{
  struct sockaddr_in addr;
  socklen_t addrlen;
  int listener;

  listener = tcp_reset_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC,
                                       0));
  addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  addr.sin_port = 0;
  assert_return_code(bind(listener, (FAR struct sockaddr *)&addr,
                          addrlen), errno);
  assert_return_code(listen(listener, 1), errno);
  assert_return_code(getsockname(listener, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);

  pair->client = tcp_reset_keep(st, socket(AF_INET,
                                           SOCK_STREAM | SOCK_CLOEXEC, 0));
  assert_return_code(connect(pair->client, (FAR struct sockaddr *)&addr,
                             addrlen), errno);
  pair->server = tcp_reset_keep(st, accept4(listener, NULL, NULL,
                                            SOCK_CLOEXEC));
  tcp_reset_fd(st, listener);
  tcp_reset_timeouts(pair->client);
  tcp_reset_timeouts(pair->server);

  assert_int_equal(send(pair->client, "u", 1, 0), 1);
  assert_int_equal(tcp_reset_poll(pair->server, POLLIN), POLLIN);
}

/****************************************************************************
 * Name: tcp_reset_conn
 *
 * Description:
 *   Make a connection whose server sends len bytes to the client, then
 *   resets the connection.  Return the client socket once the reset came
 *   in.
 *
 ****************************************************************************/

static int tcp_reset_conn(FAR struct tcp_reset_state_s *st, size_t len)
{
  struct tcp_reset_pair_s pair;
  char buf[TCP_RESET_DATA];

  tcp_reset_pair(st, &pair);
  if (len > 0)
    {
      memset(buf, 'd', sizeof(buf));
      assert_int_equal(send(pair.server, buf, len, 0), len);
      assert_true((tcp_reset_poll(pair.client, POLLIN) & POLLIN) != 0);
    }

  tcp_reset_fd(st, pair.server);

  /* With no events, poll() returns at POLLHUP (or POLLERR) */

  assert_true((tcp_reset_poll(pair.client, 0) & POLLHUP) != 0);
  return pair.client;
}

/****************************************************************************
 * Name: tcp_reset_peer
 *
 * Description:
 *   The peer of a call that waits: send the data, wait, then reset the
 *   connection.
 *
 ****************************************************************************/

static FAR void *tcp_reset_peer(FAR void *arg)
{
  FAR struct tcp_reset_peer_s *peer = arg;
  char buf[TCP_RESET_DATA];

  memset(buf, 'd', sizeof(buf));
  if (peer->len > 0 && send(peer->fd, buf, peer->len, 0) != peer->len)
    {
      peer->len = 0;
    }

  usleep(TCP_RESET_DELAY * 1000);
  close(peer->fd);
  return NULL;
}

/****************************************************************************
 * Name: tcp_reset_start
 *
 * Description:
 *   Hand the server socket of pair over to a thread that resets the
 *   connection after TCP_RESET_DELAY ms.  It sends len bytes first.
 *
 ****************************************************************************/

static pthread_t tcp_reset_start(FAR struct tcp_reset_state_s *st,
                                 FAR struct tcp_reset_pair_s *pair,
                                 FAR struct tcp_reset_peer_s *peer,
                                 size_t len)
{
  pthread_t tid;

  tcp_reset_forget(st, pair->server);
  peer->fd  = pair->server;
  peer->len = len;
  assert_int_equal(pthread_create(&tid, NULL, tcp_reset_peer, peer), 0);
  return tid;
}

/****************************************************************************
 * Name: tcp_reset_onpipe
 ****************************************************************************/

#ifndef CONFIG_DISABLE_ALL_SIGNALS
static void tcp_reset_onpipe(int signo)
{
  g_tcp_reset_sigpipe++;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_reset_setup
 ****************************************************************************/

int test_tcp_reset_setup(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TCP_RESET_MAXFDS; i++)
    {
      st->fds[i] = -1;
    }

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_reset_teardown
 ****************************************************************************/

int test_tcp_reset_teardown(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  int i;

  for (i = 0; i < TCP_RESET_MAXFDS; i++)
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
 * Name: test_tcp_reset_recv
 *
 * Description:
 *   The data that came before the reset is read first (recvmsg() with two
 *   buffers: the first one takes all the data).  Then recv() gives
 *   ECONNRESET, then end-of-file.
 *
 ****************************************************************************/

void test_tcp_reset_recv(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  char buf[TCP_RESET_DATA];
  char more[16];
  struct iovec iov[2];
  struct msghdr msg;
  int fd;

  fd = tcp_reset_conn(st, TCP_RESET_DATA);

  iov[0].iov_base = buf;
  iov[0].iov_len  = sizeof(buf);
  iov[1].iov_base = more;
  iov[1].iov_len  = sizeof(more);
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov    = iov;
  msg.msg_iovlen = 2;
  assert_int_equal(recvmsg(fd, &msg, 0), TCP_RESET_DATA);

  assert_int_equal(recv(fd, buf, sizeof(buf), 0), -1);
  assert_int_equal(errno, ECONNRESET);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), 0);
  tcp_reset_fd(st, fd);
}

/****************************************************************************
 * Name: test_tcp_reset_recv_wait
 *
 * Description:
 *   A recv() that waits when the peer resets the connection gives
 *   ECONNRESET.  The next recv() gives end-of-file.
 *
 ****************************************************************************/

void test_tcp_reset_recv_wait(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  struct tcp_reset_pair_s pair;
  struct tcp_reset_peer_s peer;
  pthread_t tid;
  ssize_t ret;
  int err;
  char c;

  tcp_reset_pair(st, &pair);
  tid = tcp_reset_start(st, &pair, &peer, 0);
  ret = recv(pair.client, &c, 1, 0);
  err = errno;
  assert_int_equal(pthread_join(tid, NULL), 0);

  assert_int_equal(ret, -1);
  assert_int_equal(err, ECONNRESET);
  assert_int_equal(recv(pair.client, &c, 1, 0), 0);
  tcp_reset_fd(st, pair.client);
}

/****************************************************************************
 * Name: test_tcp_reset_recv_waitall
 *
 * Description:
 *   A recv() with MSG_WAITALL that waits when the peer sends a part of the
 *   data and resets the connection gives that part.  The next recv() gives
 *   ECONNRESET, the one after it end-of-file.
 *
 ****************************************************************************/

void test_tcp_reset_recv_waitall(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  struct tcp_reset_pair_s pair;
  struct tcp_reset_peer_s peer;
  char buf[4 * TCP_RESET_DATA];
  pthread_t tid;
  ssize_t ret;

  tcp_reset_pair(st, &pair);
  tid = tcp_reset_start(st, &pair, &peer, TCP_RESET_DATA);
  ret = recv(pair.client, buf, sizeof(buf), MSG_WAITALL);
  assert_int_equal(pthread_join(tid, NULL), 0);
  assert_int_equal(peer.len, TCP_RESET_DATA);

  assert_int_equal(ret, TCP_RESET_DATA);
  assert_int_equal(recv(pair.client, buf, sizeof(buf), 0), -1);
  assert_int_equal(errno, ECONNRESET);
  assert_int_equal(recv(pair.client, buf, sizeof(buf), 0), 0);
  tcp_reset_fd(st, pair.client);
}

/****************************************************************************
 * Name: test_tcp_reset_send
 *
 * Description:
 *   After the reset, send() gives ECONNRESET once, then EPIPE.  Without
 *   MSG_NOSIGNAL, EPIPE comes with SIGPIPE.
 *
 ****************************************************************************/

void test_tcp_reset_send(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
#ifndef CONFIG_DISABLE_ALL_SIGNALS
  struct sigaction act;
  struct sigaction old;
  ssize_t ret;
  int err;
#endif
  int fd;

  fd = tcp_reset_conn(st, 0);
  assert_int_equal(send(fd, "s", 1, MSG_NOSIGNAL), -1);
  assert_int_equal(errno, ECONNRESET);
  assert_int_equal(send(fd, "s", 1, MSG_NOSIGNAL), -1);
  assert_int_equal(errno, EPIPE);

#ifndef CONFIG_DISABLE_ALL_SIGNALS
  memset(&act, 0, sizeof(act));
  act.sa_handler = tcp_reset_onpipe;
  sigemptyset(&act.sa_mask);
  assert_return_code(sigaction(SIGPIPE, &act, &old), errno);
  g_tcp_reset_sigpipe = 0;
  ret = send(fd, "s", 1, 0);
  err = errno;
  sigaction(SIGPIPE, &old, NULL);

  assert_int_equal(ret, -1);
  assert_int_equal(err, EPIPE);
  assert_int_equal(g_tcp_reset_sigpipe, 1);
#endif

  tcp_reset_fd(st, fd);
}

/****************************************************************************
 * Name: test_tcp_reset_poll
 *
 * Description:
 *   After the reset with data unread, poll() gives POLLIN, POLLHUP and
 *   POLLERR, not POLLOUT.  SO_ERROR gives ECONNRESET once; then poll()
 *   gives no POLLERR, and recv() the data, then end-of-file.
 *
 ****************************************************************************/

void test_tcp_reset_poll(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  char buf[2 * TCP_RESET_DATA];
#ifdef CONFIG_NET_SOCKOPTS
  socklen_t len;
  int err;
#endif
  int fd;

  fd = tcp_reset_conn(st, TCP_RESET_DATA);
  assert_int_equal(tcp_reset_poll(fd, POLLIN | POLLOUT),
                   POLLIN | POLLHUP | POLLERR);

#ifdef CONFIG_NET_SOCKOPTS
  len = sizeof(err);
  assert_return_code(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len),
                     errno);
  assert_int_equal(err, ECONNRESET);
  assert_return_code(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len),
                     errno);
  assert_int_equal(err, 0);
  assert_int_equal(tcp_reset_poll(fd, POLLIN | POLLOUT), POLLIN | POLLHUP);

  /* SO_ERROR took the error: recv() gives the data, then end-of-file */

  assert_int_equal(recv(fd, buf, sizeof(buf), 0), TCP_RESET_DATA);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), 0);
#else
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), TCP_RESET_DATA);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), -1);
  assert_int_equal(errno, ECONNRESET);
  assert_int_equal(tcp_reset_poll(fd, POLLIN | POLLOUT), POLLIN | POLLHUP);
#endif

  tcp_reset_fd(st, fd);
}

/****************************************************************************
 * Name: test_tcp_reset_refused
 *
 * Description:
 *   A non-blocking connect() to a port without a listener: the RST refuses
 *   the connection.  poll() gives POLLERR and SO_ERROR ECONNREFUSED, not
 *   ECONNRESET.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOCKOPTS
void test_tcp_reset_refused(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  struct sockaddr_in addr;
  socklen_t addrlen;
  socklen_t len;
  int err;
  int fd;

  /* A free port: bind a socket to port 0, then close it */

  fd = tcp_reset_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0));
  addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  addr.sin_port = 0;
  assert_return_code(bind(fd, (FAR struct sockaddr *)&addr, addrlen),
                     errno);
  assert_return_code(getsockname(fd, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);
  tcp_reset_fd(st, fd);

  fd = tcp_reset_keep(st, socket(AF_INET,
                                 SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
                                 0));
  assert_int_equal(connect(fd, (FAR struct sockaddr *)&addr, addrlen), -1);
  assert_int_equal(errno, EINPROGRESS);
  assert_true((tcp_reset_poll(fd, POLLOUT) & POLLERR) != 0);

  len = sizeof(err);
  assert_return_code(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len),
                     errno);
  assert_int_equal(err, ECONNREFUSED);
  tcp_reset_fd(st, fd);
}

/****************************************************************************
 * Name: test_tcp_reset_closewait
 *
 * Description:
 *   The peer closes its half of the connection (FIN), then resets it.
 *   SO_ERROR gives EPIPE once (Linux), not ECONNREFUSED or 0.  poll()
 *   after it gives POLLHUP and does not set another error.  send() gives
 *   EPIPE.
 *
 ****************************************************************************/

void test_tcp_reset_closewait(FAR void **state)
{
  FAR struct tcp_reset_state_s *st = *state;
  struct tcp_reset_pair_s pair;
  socklen_t len;
  int err;
  char c;

  /* The FIN of the server: the client is in CLOSE_WAIT */

  tcp_reset_pair(st, &pair);
  assert_return_code(shutdown(pair.server, SHUT_WR), errno);
  assert_true((tcp_reset_poll(pair.client, POLLIN) & POLLIN) != 0);
  assert_int_equal(recv(pair.client, &c, 1, 0), 0);

  /* The byte that the server did not read: its close() resets the
   * connection.
   */

  tcp_reset_fd(st, pair.server);
  usleep(TCP_RESET_DELAY * 1000);

  len = sizeof(err);
  assert_return_code(getsockopt(pair.client, SOL_SOCKET, SO_ERROR, &err,
                                &len), errno);
  assert_int_equal(err, EPIPE);
  assert_return_code(getsockopt(pair.client, SOL_SOCKET, SO_ERROR, &err,
                                &len), errno);
  assert_int_equal(err, 0);

  assert_true((tcp_reset_poll(pair.client, POLLIN | POLLOUT) &
               POLLHUP) != 0);
  assert_return_code(getsockopt(pair.client, SOL_SOCKET, SO_ERROR, &err,
                                &len), errno);
  assert_int_equal(err, 0);

  assert_int_equal(send(pair.client, "s", 1, MSG_NOSIGNAL), -1);
  assert_int_equal(errno, EPIPE);
  tcp_reset_fd(st, pair.client);
}
#endif
