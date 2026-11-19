/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_closewait.c
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

/* A peer that closes its half of an IPv4 TCP connection over the loopback
 * address (FIN) before accept() or connect() returns.  The connection is
 * still made: accept() and connect() succeed, a receive gives the data
 * that came and then end-of-file, and a send still reaches the peer.
 *
 * A peer that resets the connection (RST: a close with SO_LINGER {1, 0})
 * before accept() or connect() returns, as Linux does: accept() gives the
 * socket, which reports ECONNRESET once (after the data that came before
 * the RST), then end-of-file; connect() fails with ECONNRESET.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCP_CW_MAXFDS  4
#define TCP_CW_TIMEOUT 10   /* s, SO_RCVTIMEO and SO_SNDTIMEO */
#define TCP_CW_DELAY   100  /* ms for a FIN to reach the other end */
#define TCP_CW_DATA    5    /* Bytes that come before the FIN */
#define TCP_CW_ROUNDS  20   /* Rounds of test_tcp_closewait_rst_race */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The sockets of one case: the teardown closes the ones that a failed case
 * leaves open.
 */

struct tcp_cw_state_s
{
  int fds[TCP_CW_MAXFDS];
};

/* A client that connects in another thread */

struct tcp_cw_client_s
{
  struct sockaddr_in addr;
  socklen_t addrlen;
  int fd;
  int ret;      /* The result of connect() */
  int err;      /* Its errno */
  ssize_t eof;  /* The result of the recv() after it */
  int rerr;     /* Its errno */
  int soerr;    /* SO_ERROR after it */
};

/* A client that connects in another thread and resets the connection */

struct tcp_cw_reset_s
{
  struct sockaddr_in addr;
  socklen_t addrlen;
  int before;   /* ms to wait before connect() */
  int after;    /* ms to wait after connect(), before the reset */
  int spin;     /* ms to keep the CPU after the reset */
  size_t len;   /* Bytes to send before the reset */
  int ret;      /* The result of connect() */
  int err;      /* Its errno */
};

/* The thread of a case and the CPUs of the caller before it */

struct tcp_cw_thread_s
{
  pthread_t tid;
#ifdef CONFIG_SMP
  cpu_set_t oldset;
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_cw_keep
 ****************************************************************************/

static int tcp_cw_keep(FAR struct tcp_cw_state_s *st, int fd)
{
  int i;

  assert_return_code(fd, errno);
  for (i = 0; i < TCP_CW_MAXFDS; i++)
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
 * Name: tcp_cw_close
 ****************************************************************************/

static void tcp_cw_close(FAR struct tcp_cw_state_s *st, int fd)
{
  int i;

  for (i = 0; i < TCP_CW_MAXFDS; i++)
    {
      if (st->fds[i] == fd)
        {
          st->fds[i] = -1;
          assert_return_code(close(fd), errno);
          return;
        }
    }

  fail();
}

/****************************************************************************
 * Name: tcp_cw_socket
 *
 * Description:
 *   Return a new socket.  send() and recv() on it have a timeout, so that
 *   a failing case ends.
 *
 ****************************************************************************/

static int tcp_cw_socket(FAR struct tcp_cw_state_s *st)
{
  int fd = tcp_cw_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0));
#ifdef CONFIG_NET_SOCKOPTS
  struct timeval tv;

  tv.tv_sec  = TCP_CW_TIMEOUT;
  tv.tv_usec = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv,
                                sizeof(tv)), errno);
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
#endif

  return fd;
}

/****************************************************************************
 * Name: tcp_cw_listener
 *
 * Description:
 *   Return a socket that listens on 127.0.0.1 and a port that bind()
 *   selects, and its address in addr.
 *
 ****************************************************************************/

static int tcp_cw_listener(FAR struct tcp_cw_state_s *st,
                           FAR struct sockaddr_in *addr,
                           FAR socklen_t *addrlen)
{
  int fd = tcp_cw_socket(st);

  *addrlen = nettest_lo_addr((FAR struct sockaddr *)addr, AF_INET);
  addr->sin_port = 0;
  assert_return_code(bind(fd, (FAR struct sockaddr *)addr, *addrlen),
                     errno);
  assert_return_code(listen(fd, 1), errno);
  assert_return_code(getsockname(fd, (FAR struct sockaddr *)addr,
                                 addrlen), errno);
  return fd;
}

/****************************************************************************
 * Name: tcp_cw_accept
 *
 * Description:
 *   Accept a connection that the client made, sent len bytes on and then
 *   closed (or only shut down for writing, if half is true), before the
 *   server called accept().  Return the accepted socket and in *client the
 *   client socket (-1 if it is closed).
 *
 ****************************************************************************/

static int tcp_cw_accept(FAR struct tcp_cw_state_s *st, size_t len,
                         bool half, FAR int *client)
{
  struct sockaddr_in addr;
  socklen_t addrlen;
  char buf[TCP_CW_DATA];
  int listener;
  int server;

  listener = tcp_cw_listener(st, &addr, &addrlen);
  *client  = tcp_cw_socket(st);
  assert_return_code(connect(*client, (FAR struct sockaddr *)&addr,
                             addrlen), errno);

  if (len > 0)
    {
      memset(buf, 'd', sizeof(buf));
      assert_int_equal(send(*client, buf, len, 0), len);
    }

  if (half)
    {
      assert_return_code(shutdown(*client, SHUT_WR), errno);
    }
  else
    {
      tcp_cw_close(st, *client);
      *client = -1;
    }

  /* The FIN of the client reaches the connection in the backlog of the
   * listener: it is in CLOSE_WAIT before accept().
   */

  usleep(TCP_CW_DELAY * 1000);

  server = tcp_cw_keep(st, accept4(listener, NULL, NULL, SOCK_CLOEXEC));
  tcp_cw_close(st, listener);
  return server;
}

/****************************************************************************
 * Name: tcp_cw_spin
 *
 * Description:
 *   Keep the CPU for ms milliseconds.
 *
 ****************************************************************************/

static void tcp_cw_spin(int ms)
{
  struct timespec start;
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &start);
  do
    {
      clock_gettime(CLOCK_MONOTONIC, &now);
    }
  while ((now.tv_sec - start.tv_sec) * 1000 +
         (now.tv_nsec - start.tv_nsec) / 1000000 < ms);
}

/****************************************************************************
 * Name: tcp_cw_soerror
 ****************************************************************************/

#ifdef CONFIG_NET_SOCKOPTS
static int tcp_cw_soerror(int fd)
{
  socklen_t len = sizeof(int);
  int err = -1;

  getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
  return err;
}
#endif

/****************************************************************************
 * Name: tcp_cw_start
 *
 * Description:
 *   Run fn(arg) in a thread with the priority of the caller plus delta.
 *   With SMP, the caller and the thread run on the CPU of the caller, so
 *   that the one with the higher priority runs first when both can.
 *
 ****************************************************************************/

static void tcp_cw_start(FAR struct tcp_cw_thread_s *t, int delta,
                         FAR void *(*fn)(FAR void *), FAR void *arg)
{
  struct sched_param param;
  pthread_attr_t attr;
  int policy;
#ifdef CONFIG_SMP
  cpu_set_t cpuset;
#endif

  assert_int_equal(pthread_getschedparam(pthread_self(), &policy, &param),
                   0);
  assert_int_equal(pthread_attr_init(&attr), 0);
  assert_int_equal(pthread_attr_setinheritsched(&attr,
                                                PTHREAD_EXPLICIT_SCHED), 0);
  assert_int_equal(pthread_attr_setschedpolicy(&attr, SCHED_FIFO), 0);
  param.sched_priority += delta;
  assert_int_equal(pthread_attr_setschedparam(&attr, &param), 0);

#ifdef CONFIG_SMP
  assert_int_equal(pthread_getaffinity_np(pthread_self(),
                                          sizeof(t->oldset), &t->oldset), 0);
  CPU_ZERO(&cpuset);
  CPU_SET(sched_getcpu(), &cpuset);
  assert_int_equal(pthread_setaffinity_np(pthread_self(), sizeof(cpuset),
                                          &cpuset), 0);
  assert_int_equal(pthread_attr_setaffinity_np(&attr, sizeof(cpuset),
                                               &cpuset), 0);
#endif

  assert_int_equal(pthread_create(&t->tid, &attr, fn, arg), 0);
  pthread_attr_destroy(&attr);
}

/****************************************************************************
 * Name: tcp_cw_join
 ****************************************************************************/

static void tcp_cw_join(FAR struct tcp_cw_thread_s *t)
{
  assert_int_equal(pthread_join(t->tid, NULL), 0);
#ifdef CONFIG_SMP
  pthread_setaffinity_np(pthread_self(), sizeof(t->oldset), &t->oldset);
#endif
}

/****************************************************************************
 * Name: tcp_cw_connect
 *
 * Description:
 *   The client: connect(), then wait for the end-of-file of the server.
 *
 ****************************************************************************/

static FAR void *tcp_cw_connect(FAR void *arg)
{
  FAR struct tcp_cw_client_s *c = arg;
  char ch;

  c->ret = connect(c->fd, (FAR struct sockaddr *)&c->addr, c->addrlen);
  c->err = errno;
  if (c->ret == 0)
    {
      c->eof = recv(c->fd, &ch, 1, 0);
    }
  else
    {
      c->eof = recv(c->fd, &ch, 1, MSG_DONTWAIT);
    }

  c->rerr = errno;
#ifdef CONFIG_NET_SOCKOPTS
  c->soerr = tcp_cw_soerror(c->fd);
#endif
  return NULL;
}

/****************************************************************************
 * Name: tcp_cw_reset
 *
 * Description:
 *   Close fd with SO_LINGER {1, 0}: the connection is reset (RST).
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOLINGER
static int tcp_cw_reset(int fd)
{
  struct linger l;

  l.l_onoff  = 1;
  l.l_linger = 0;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
  return close(fd);
}

/****************************************************************************
 * Name: tcp_cw_resetter
 *
 * Description:
 *   The client: connect(), send data, reset the connection.
 *
 ****************************************************************************/

static FAR void *tcp_cw_resetter(FAR void *arg)
{
  FAR struct tcp_cw_reset_s *c = arg;
  char buf[TCP_CW_DATA];
  int fd;

  usleep(c->before * 1000);
  fd     = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  c->ret = connect(fd, (FAR struct sockaddr *)&c->addr, c->addrlen);
  c->err = errno;
  if (c->ret == 0)
    {
      memset(buf, 'd', sizeof(buf));
      if (c->len > 0 && send(fd, buf, c->len, 0) != (ssize_t)c->len)
        {
          c->ret = -1;
          c->err = errno;
        }

      usleep(c->after * 1000);
      tcp_cw_reset(fd);
    }
  else if (fd >= 0)
    {
      close(fd);
    }

  tcp_cw_spin(c->spin);
  return NULL;
}

/****************************************************************************
 * Name: tcp_cw_rst_accept
 *
 * Description:
 *   The client connects to an accept() that waits, sends len bytes and
 *   resets the connection before accept() returns: the client runs at a
 *   higher priority than the server, on the same CPU, and keeps the CPU
 *   until its RST reached the server.  Return the accepted socket.
 *
 ****************************************************************************/

static int tcp_cw_rst_accept(FAR struct tcp_cw_state_s *st, size_t len)
{
  struct tcp_cw_reset_s c;
  struct tcp_cw_thread_s t;
  int listener;
  int fd;
  int err;

  memset(&c, 0, sizeof(c));
  listener = tcp_cw_listener(st, &c.addr, &c.addrlen);
  c.before = TCP_CW_DELAY;
  c.spin   = TCP_CW_DELAY;
  c.len    = len;

  tcp_cw_start(&t, 1, tcp_cw_resetter, &c);
  fd  = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
  err = errno;
  tcp_cw_join(&t);

  assert_return_code(c.ret, c.err);
  assert_return_code(fd, err);
  tcp_cw_keep(st, fd);
  tcp_cw_close(st, listener);
  return fd;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_closewait_setup
 ****************************************************************************/

int test_tcp_closewait_setup(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TCP_CW_MAXFDS; i++)
    {
      st->fds[i] = -1;
    }

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_closewait_teardown
 ****************************************************************************/

int test_tcp_closewait_teardown(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  int i;

  for (i = 0; i < TCP_CW_MAXFDS; i++)
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
 * Name: test_tcp_closewait_accept_data
 *
 * Description:
 *   The client connects, sends data and closes before accept().  accept()
 *   succeeds, recv() gives the data, then end-of-file.
 *
 ****************************************************************************/

void test_tcp_closewait_accept_data(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  char buf[2 * TCP_CW_DATA];
  int client;
  int fd;

  fd = tcp_cw_accept(st, TCP_CW_DATA, false, &client);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), TCP_CW_DATA);
  assert_memory_equal(buf, "ddddd", TCP_CW_DATA);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), 0);
  tcp_cw_close(st, fd);
}

/****************************************************************************
 * Name: test_tcp_closewait_accept
 *
 * Description:
 *   The client connects and closes before accept().  accept() succeeds,
 *   recv() gives end-of-file.
 *
 ****************************************************************************/

void test_tcp_closewait_accept(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  char buf[TCP_CW_DATA];
  int client;
  int fd;

  fd = tcp_cw_accept(st, 0, false, &client);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), 0);
  tcp_cw_close(st, fd);
}

/****************************************************************************
 * Name: test_tcp_closewait_accept_half
 *
 * Description:
 *   The client connects and shuts down its writing side before accept().
 *   accept() succeeds, recv() gives end-of-file, and the server can still
 *   send to the client.
 *
 ****************************************************************************/

void test_tcp_closewait_accept_half(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  char buf[TCP_CW_DATA];
  int client;
  int fd;

  fd = tcp_cw_accept(st, 0, true, &client);
  assert_int_equal(recv(fd, buf, sizeof(buf), 0), 0);
  assert_int_equal(send(fd, "r", 1, 0), 1);
  tcp_cw_close(st, fd);

  assert_int_equal(recv(client, buf, sizeof(buf), 0), 1);
  assert_int_equal(buf[0], 'r');
  assert_int_equal(recv(client, buf, sizeof(buf), 0), 0);
  tcp_cw_close(st, client);
}

/****************************************************************************
 * Name: test_tcp_closewait_connect
 *
 * Description:
 *   The server accepts and closes the connection at once, before the
 *   connect() of the client returns.  connect() succeeds, recv() gives
 *   end-of-file.
 *
 *   The client runs at a lower priority than the server, on the same CPU:
 *   the server runs first when the connection is made, and keeps the CPU
 *   until its FIN reached the client.
 *
 ****************************************************************************/

void test_tcp_closewait_connect(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  struct tcp_cw_client_s c;
  struct tcp_cw_thread_s t;
  int listener;
  int server;
  int err;

  memset(&c, 0, sizeof(c));
  c.eof    = -1;
  listener = tcp_cw_listener(st, &c.addr, &c.addrlen);
  c.fd     = tcp_cw_socket(st);

  tcp_cw_start(&t, -1, tcp_cw_connect, &c);
  server = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
  err    = errno;
  if (server >= 0)
    {
      close(server);
    }

  tcp_cw_spin(TCP_CW_DELAY);
  tcp_cw_join(&t);

  assert_return_code(server, err);
  assert_return_code(c.ret, c.err);
  assert_int_equal(c.eof, 0);
  tcp_cw_close(st, c.fd);
  tcp_cw_close(st, listener);
}

#ifdef CONFIG_NET_SOLINGER
/****************************************************************************
 * Name: test_tcp_closewait_rst_accept
 *
 * Description:
 *   The client connects and resets the connection before accept()
 *   returns.  accept() succeeds, recv() gives ECONNRESET, then end-of-file,
 *   and send() EPIPE.
 *
 ****************************************************************************/

void test_tcp_closewait_rst_accept(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  char buf[TCP_CW_DATA];
  int fd;

  fd = tcp_cw_rst_accept(st, 0);
  assert_int_equal(recv(fd, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_int_equal(errno, ECONNRESET);
  assert_int_equal(recv(fd, buf, sizeof(buf), MSG_DONTWAIT), 0);
  assert_int_equal(send(fd, "r", 1, MSG_NOSIGNAL), -1);
  assert_int_equal(errno, EPIPE);
  tcp_cw_close(st, fd);
}

/****************************************************************************
 * Name: test_tcp_closewait_rst_accept_data
 *
 * Description:
 *   The client connects, sends data and resets the connection before
 *   accept() returns.  accept() succeeds, recv() gives the data, SO_ERROR
 *   ECONNRESET (without CONFIG_NET_SOCKOPTS: recv()), then recv() gives
 *   end-of-file.
 *
 ****************************************************************************/

void test_tcp_closewait_rst_accept_data(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  char buf[2 * TCP_CW_DATA];
  int fd;

  fd = tcp_cw_rst_accept(st, TCP_CW_DATA);
  assert_int_equal(recv(fd, buf, sizeof(buf), MSG_DONTWAIT), TCP_CW_DATA);
  assert_memory_equal(buf, "ddddd", TCP_CW_DATA);
#ifdef CONFIG_NET_SOCKOPTS
  assert_int_equal(tcp_cw_soerror(fd), ECONNRESET);
#else
  assert_int_equal(recv(fd, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_int_equal(errno, ECONNRESET);
#endif
  assert_int_equal(recv(fd, buf, sizeof(buf), MSG_DONTWAIT), 0);
  tcp_cw_close(st, fd);
}

/****************************************************************************
 * Name: test_tcp_closewait_rst_connect
 *
 * Description:
 *   The server accepts and resets the connection at once, before the
 *   connect() of the client returns.  connect() fails with ECONNRESET; the
 *   socket is not connected (recv() gives ENOTCONN) and SO_ERROR is 0.
 *
 ****************************************************************************/

void test_tcp_closewait_rst_connect(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  struct tcp_cw_client_s c;
  struct tcp_cw_thread_s t;
  int listener;
  int server;
  int err;

  memset(&c, 0, sizeof(c));
  c.eof    = -2;
  c.soerr  = -1;
  listener = tcp_cw_listener(st, &c.addr, &c.addrlen);
  c.fd     = tcp_cw_socket(st);

  tcp_cw_start(&t, -1, tcp_cw_connect, &c);
  server = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
  err    = errno;
  if (server >= 0)
    {
      tcp_cw_reset(server);
    }

  tcp_cw_spin(TCP_CW_DELAY);
  tcp_cw_join(&t);

  assert_return_code(server, err);
  assert_int_equal(c.ret, -1);
  assert_int_equal(c.err, ECONNRESET);
  assert_int_equal(c.eof, -1);
  assert_int_equal(c.rerr, ENOTCONN);
#ifdef CONFIG_NET_SOCKOPTS
  assert_int_equal(c.soerr, 0);
#endif
  tcp_cw_close(st, c.fd);
  tcp_cw_close(st, listener);
}

/****************************************************************************
 * Name: test_tcp_closewait_rst_race
 *
 * Description:
 *   In each round, the client connects to an accept() that waits and
 *   resets the connection 0 to 4 ms later, at the same priority, around
 *   the return of accept().  accept() succeeds, and once the RST came the
 *   socket is not connected: send() fails with ECONNRESET or EPIPE, and
 *   recv() gives end-of-file or ECONNRESET.
 *
 ****************************************************************************/

void test_tcp_closewait_rst_race(FAR void **state)
{
  FAR struct tcp_cw_state_s *st = *state;
  struct tcp_cw_reset_s c;
  pthread_t tid;
  char buf[TCP_CW_DATA];
  ssize_t ret;
  int listener;
  int round;
  int fd;
  int err;

  for (round = 0; round < TCP_CW_ROUNDS; round++)
    {
      memset(&c, 0, sizeof(c));
      listener = tcp_cw_listener(st, &c.addr, &c.addrlen);
      c.before = TCP_CW_DELAY / 5;
      c.after  = round % 5;

      assert_int_equal(pthread_create(&tid, NULL, tcp_cw_resetter, &c), 0);
      fd  = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
      err = errno;
      assert_int_equal(pthread_join(tid, NULL), 0);
      assert_return_code(c.ret, c.err);
      assert_return_code(fd, err);
      tcp_cw_keep(st, fd);
      tcp_cw_close(st, listener);

      usleep(TCP_CW_DELAY * 1000);
      ret = send(fd, "r", 1, MSG_NOSIGNAL | MSG_DONTWAIT);
      err = errno;
      assert_int_equal(ret, -1);
      assert_true(err == ECONNRESET || err == EPIPE);
      ret = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
      assert_true(ret == 0 || (ret < 0 && errno == ECONNRESET));
      tcp_cw_close(st, fd);
    }
}
#endif /* CONFIG_NET_SOLINGER */
