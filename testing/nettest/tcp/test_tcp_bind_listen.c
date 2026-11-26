/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_bind_listen.c
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

/* bind() with SO_REUSEADDR, listen() and the options of an accepted
 * socket, for IPv4 TCP sockets on the loopback address, with the rules of
 * Linux.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
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
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_POLL_TIMEOUT 1000 /* ms, loopback answers at once */
#define TEST_MAX_FDS      8
#define TEST_FILL_SIZE    1024
#define TEST_FILL_MAX     1024  /* sends of TEST_FILL_SIZE, at most */
#define TEST_ACCEPT_TIMEO 300   /* ms, SO_RCVTIMEO of accept() */
#define TEST_ACCEPT_LATE  3000  /* ms before a client ends a wait */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The sockets of one case: the teardown closes the ones that a failed
 * case leaves open, so that the next cases have free connections.
 */

struct tcp_bl_state_s
{
  int fds[TEST_MAX_FDS];
};

/* A client that connects after TEST_ACCEPT_LATE ms, unless the case is
 * done: an accept() that ignores its timeout still returns.
 */

struct tcp_bl_late_s
{
  in_port_t port;
  volatile bool done;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_bl_keep
 ****************************************************************************/

static int tcp_bl_keep(FAR struct tcp_bl_state_s *st, int fd)
{
  int i;

  assert_true(fd >= 0);
  for (i = 0; i < TEST_MAX_FDS; i++)
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
 * Name: tcp_bl_close
 ****************************************************************************/

static void tcp_bl_close(FAR struct tcp_bl_state_s *st, int fd)
{
  int i;

  for (i = 0; i < TEST_MAX_FDS; i++)
    {
      if (st->fds[i] == fd)
        {
          st->fds[i] = -1;
          close(fd);
          return;
        }
    }

  fail();
}

/****************************************************************************
 * Name: tcp_bl_socket
 ****************************************************************************/

static int tcp_bl_socket(FAR struct tcp_bl_state_s *st, int type,
                         bool reuse)
{
  int fd = tcp_bl_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC |
                                  type, 0));
  int on = 1;

  if (reuse)
    {
      assert_return_code(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on,
                                    sizeof(on)), errno);
    }

  return fd;
}

/****************************************************************************
 * Name: tcp_bl_bind
 *
 * Description:
 *   Bind fd to 127.0.0.1 and port (network order).  Return 0 or -errno.
 *
 ****************************************************************************/

static int tcp_bl_bind(int fd, in_port_t port)
{
  struct sockaddr_in addr;
  socklen_t addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr,
                                      AF_INET);

  addr.sin_port = port;
  return bind(fd, (FAR struct sockaddr *)&addr, addrlen) < 0 ? -errno : 0;
}

/****************************************************************************
 * Name: tcp_bl_connect
 ****************************************************************************/

static int tcp_bl_connect(int fd, in_port_t port)
{
  struct sockaddr_in addr;
  socklen_t addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr,
                                      AF_INET);

  addr.sin_port = port;
  return connect(fd, (FAR struct sockaddr *)&addr, addrlen) < 0 ?
         -errno : 0;
}

/****************************************************************************
 * Name: tcp_bl_listener
 *
 * Description:
 *   Return a socket that listens on 127.0.0.1 and a port that bind()
 *   selects, and that port (network order) in *port.
 *
 ****************************************************************************/

static int tcp_bl_listener(FAR struct tcp_bl_state_s *st, bool reuse,
                           int backlog, FAR in_port_t *port)
{
  int fd = tcp_bl_socket(st, 0, reuse);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);

  assert_int_equal(tcp_bl_bind(fd, 0), 0);
  assert_return_code(listen(fd, backlog), errno);
  assert_return_code(getsockname(fd, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);
  *port = addr.sin_port;
  return fd;
}

/****************************************************************************
 * Name: tcp_bl_wait
 *
 * Description:
 *   Wait until fd has one of the events.
 *
 ****************************************************************************/

static void tcp_bl_wait(int fd, short events)
{
  struct pollfd pfd;

  pfd.fd      = fd;
  pfd.events  = events;
  pfd.revents = 0;
  assert_int_equal(poll(&pfd, 1, TEST_POLL_TIMEOUT), 1);
}

/****************************************************************************
 * Name: tcp_bl_accept
 ****************************************************************************/

static int tcp_bl_accept(FAR struct tcp_bl_state_s *st, int listener)
{
  tcp_bl_wait(listener, POLLIN);
  return tcp_bl_keep(st, accept4(listener, NULL, NULL, SOCK_CLOEXEC));
}

/****************************************************************************
 * Name: tcp_bl_late
 ****************************************************************************/

#ifdef CONFIG_NET_SOCKOPTS
static FAR void *tcp_bl_late(FAR void *arg)
{
  FAR struct tcp_bl_late_s *late = arg;
  int fd;
  int i;

  for (i = 0; i < TEST_ACCEPT_LATE / 100 && !late->done; i++)
    {
      usleep(100 * 1000);
    }

  if (!late->done)
    {
      fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
      if (fd >= 0)
        {
          tcp_bl_connect(fd, late->port);
          while (!late->done)
            {
              usleep(100 * 1000);
            }

          close(fd);
        }
    }

  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_bind_listen_setup
 ****************************************************************************/

int test_tcp_bind_listen_setup(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TEST_MAX_FDS; i++)
    {
      st->fds[i] = -1;
    }

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_bind_listen_teardown
 ****************************************************************************/

int test_tcp_bind_listen_teardown(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int i;

  for (i = 0; i < TEST_MAX_FDS; i++)
    {
      if (st->fds[i] >= 0)
        {
          close(st->fds[i]);
        }
    }

  free(st);
  return 0;
}

#ifdef CONFIG_NET_SOCKOPTS

/****************************************************************************
 * Name: test_tcp_bind_reuseaddr_timewait
 *
 * Description:
 *   A server closes its connection first, so the connection stays in
 *   TIME_WAIT on the port.  A new server binds the port with SO_REUSEADDR;
 *   without SO_REUSEADDR the port is in use.
 *
 ****************************************************************************/

void test_tcp_bind_reuseaddr_timewait(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int client = tcp_bl_socket(st, 0, false);
  int noreuse = tcp_bl_socket(st, 0, false);
  int reuse = tcp_bl_socket(st, 0, true);
  in_port_t port;
  int listener;
  int server;
  char c;

  /* The sockets are made first: a new connection could take the place of
   * the connection in TIME_WAIT (tcp_alloc()) when there are no free ones.
   */

  listener = tcp_bl_listener(st, true, 1, &port);
  assert_int_equal(tcp_bl_connect(client, port), 0);
  server = tcp_bl_accept(st, listener);
  tcp_bl_close(st, listener);

  tcp_bl_close(st, server);
  assert_int_equal(recv(client, &c, 1, 0), 0);
  tcp_bl_close(st, client);

  assert_int_equal(tcp_bl_bind(noreuse, port), -EADDRINUSE);
  assert_int_equal(tcp_bl_bind(reuse, port), 0);
  assert_return_code(listen(reuse, 1), errno);
}

/****************************************************************************
 * Name: test_tcp_bind_reuseaddr_connected
 *
 * Description:
 *   A connection stays on the port after its listener closed.  A socket
 *   with SO_REUSEADDR binds the port only if the listener had
 *   SO_REUSEADDR too: the accepted connection gets it from the listener.
 *
 ****************************************************************************/

void test_tcp_bind_reuseaddr_connected(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  in_port_t port;
  int listener;
  int noreuse;
  int client;
  int server;
  int reuse;
  int old;

  for (old = 0; old < 2; old++)
    {
      client  = tcp_bl_socket(st, 0, false);
      noreuse = tcp_bl_socket(st, 0, false);
      reuse   = tcp_bl_socket(st, 0, true);

      listener = tcp_bl_listener(st, old, 1, &port);
      assert_int_equal(tcp_bl_connect(client, port), 0);
      server = tcp_bl_accept(st, listener);
      tcp_bl_close(st, listener);

      assert_int_equal(tcp_bl_bind(noreuse, port), -EADDRINUSE);
      assert_int_equal(tcp_bl_bind(reuse, port), old ? 0 : -EADDRINUSE);
      if (old)
        {
          assert_return_code(listen(reuse, 1), errno);
        }

      tcp_bl_close(st, noreuse);
      tcp_bl_close(st, reuse);
      tcp_bl_close(st, server);
      tcp_bl_close(st, client);
    }
}

/****************************************************************************
 * Name: test_tcp_bind_reuseaddr_listener
 *
 * Description:
 *   SO_REUSEADDR does not let a socket bind the port of a listener.
 *
 ****************************************************************************/

void test_tcp_bind_reuseaddr_listener(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  in_port_t port;
  int listener;
  int noreuse;
  int reuse;
  int old;

  for (old = 0; old < 2; old++)
    {
      noreuse  = tcp_bl_socket(st, 0, false);
      reuse    = tcp_bl_socket(st, 0, true);
      listener = tcp_bl_listener(st, old, 1, &port);

      assert_int_equal(tcp_bl_bind(noreuse, port), -EADDRINUSE);
      assert_int_equal(tcp_bl_bind(reuse, port), -EADDRINUSE);

      tcp_bl_close(st, noreuse);
      tcp_bl_close(st, reuse);
      tcp_bl_close(st, listener);
    }
}

/****************************************************************************
 * Name: test_tcp_accept_inherit
 *
 * Description:
 *   An accepted socket inherits the socket options of its listener.
 *
 ****************************************************************************/

void test_tcp_accept_inherit(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int client = tcp_bl_socket(st, 0, false);
  int listener = tcp_bl_socket(st, 0, false);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  socklen_t len;
  int server;
  int value;
  int on = 1;
#ifdef CONFIG_NET_TCPPROTO_OPTIONS
  int mss = 1000;
#endif
#ifdef CONFIG_NET_TCP_KEEPALIVE
  int idle = 30;
  int intvl = 7;
  int cnt = 5;
#endif

  assert_return_code(setsockopt(listener, SOL_SOCKET, SO_KEEPALIVE, &on,
                                sizeof(on)), errno);
#ifdef CONFIG_NET_TCPPROTO_OPTIONS
  assert_return_code(setsockopt(listener, IPPROTO_TCP, TCP_MAXSEG, &mss,
                                sizeof(mss)), errno);
#endif
#ifdef CONFIG_NET_TCP_KEEPALIVE
  assert_return_code(setsockopt(listener, IPPROTO_TCP, TCP_KEEPIDLE, &idle,
                                sizeof(idle)), errno);
  assert_return_code(setsockopt(listener, IPPROTO_TCP, TCP_KEEPINTVL,
                                &intvl, sizeof(intvl)), errno);
  assert_return_code(setsockopt(listener, IPPROTO_TCP, TCP_KEEPCNT, &cnt,
                                sizeof(cnt)), errno);
#endif

  assert_int_equal(tcp_bl_bind(listener, 0), 0);
  assert_return_code(listen(listener, 1), errno);
  assert_return_code(getsockname(listener, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);
  assert_int_equal(tcp_bl_connect(client, addr.sin_port), 0);
  server = tcp_bl_accept(st, listener);

  len = sizeof(value);
  assert_return_code(getsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &value,
                                &len), errno);
  assert_int_equal(value, 1);

#ifdef CONFIG_NET_TCPPROTO_OPTIONS
  /* The connection sends no segment larger than TCP_MAXSEG of the
   * listener.
   */

  len = sizeof(value);
  assert_return_code(getsockopt(server, IPPROTO_TCP, TCP_MAXSEG, &value,
                                &len), errno);
  assert_in_range(value, 1, mss);
#endif

#ifdef CONFIG_NET_TCP_KEEPALIVE
  len = sizeof(value);
  assert_return_code(getsockopt(server, IPPROTO_TCP, TCP_KEEPIDLE, &value,
                                &len), errno);
  assert_int_equal(value, idle);
  len = sizeof(value);
  assert_return_code(getsockopt(server, IPPROTO_TCP, TCP_KEEPINTVL, &value,
                                &len), errno);
  assert_int_equal(value, intvl);
  len = sizeof(value);
  assert_return_code(getsockopt(server, IPPROTO_TCP, TCP_KEEPCNT, &value,
                                &len), errno);
  assert_int_equal(value, cnt);
#endif
}

/****************************************************************************
 * Name: test_tcp_accept_rcvtimeo
 *
 * Description:
 *   accept() on a listener with SO_RCVTIMEO and no client fails with
 *   EAGAIN after the timeout (Linux).
 *
 ****************************************************************************/

void test_tcp_accept_rcvtimeo(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  struct tcp_bl_late_s late;
  struct timespec start;
  struct timespec end;
  struct timeval tv;
  pthread_t tid;
  int listener;
  long ms;
  int err;
  int fd;

  listener = tcp_bl_listener(st, false, 1, &late.port);
  tv.tv_sec  = 0;
  tv.tv_usec = TEST_ACCEPT_TIMEO * 1000;
  assert_return_code(setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);

  late.done = false;
  assert_int_equal(pthread_create(&tid, NULL, tcp_bl_late, &late), 0);
  clock_gettime(CLOCK_MONOTONIC, &start);
  fd  = accept4(listener, NULL, NULL, SOCK_CLOEXEC);
  err = errno;
  clock_gettime(CLOCK_MONOTONIC, &end);
  late.done = true;
  assert_int_equal(pthread_join(tid, NULL), 0);
  if (fd >= 0)
    {
      tcp_bl_keep(st, fd);
    }

  ms = (end.tv_sec - start.tv_sec) * 1000 +
       (end.tv_nsec - start.tv_nsec) / 1000000;
  assert_int_equal(fd, -1);
  assert_int_equal(err, EAGAIN);
  assert_in_range(ms, TEST_ACCEPT_TIMEO - 50, TEST_ACCEPT_LATE - 500);
}
#endif /* CONFIG_NET_SOCKOPTS */

/****************************************************************************
 * Name: test_tcp_accept_addrlen
 *
 * Description:
 *   accept() gives the address of the client cut to the size of the
 *   buffer, and the whole length in addrlen (POSIX).  It writes nothing
 *   after the address.
 *
 ****************************************************************************/

void test_tcp_accept_addrlen(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  struct sockaddr_storage addr;
  struct sockaddr_in name;
  FAR struct sockaddr_in *in = (FAR struct sockaddr_in *)&addr;
  FAR const uint8_t *p = (FAR const uint8_t *)&addr;
  socklen_t namelen;
  socklen_t addrlen;
  in_port_t port;
  int listener;
  int client;
  size_t cut;
  size_t i;

  listener = tcp_bl_listener(st, false, 2, &port);
  for (cut = 0; cut < 2; cut++)
    {
      client = tcp_bl_socket(st, 0, false);
      assert_int_equal(tcp_bl_connect(client, port), 0);
      namelen = sizeof(name);
      assert_return_code(getsockname(client, (FAR struct sockaddr *)&name,
                                     &namelen), errno);
      tcp_bl_wait(listener, POLLIN);

      /* Room for the family, the port and the IPv4 address only, or for
       * more than the whole address
       */

      memset(&addr, 0xaa, sizeof(addr));
      addrlen = cut ? offsetof(struct sockaddr_in, sin_zero) : sizeof(addr);
      tcp_bl_keep(st, accept4(listener, (FAR struct sockaddr *)&addr,
                              &addrlen, SOCK_CLOEXEC));

      assert_int_equal(addrlen, sizeof(struct sockaddr_in));
      assert_int_equal(in->sin_family, AF_INET);
      assert_int_equal(in->sin_port, name.sin_port);
      assert_int_equal(in->sin_addr.s_addr, name.sin_addr.s_addr);
      for (i = cut ? offsetof(struct sockaddr_in, sin_zero) :
                     sizeof(struct sockaddr_in);
           i < sizeof(addr); i++)
        {
          assert_int_equal(p[i], 0xaa);
        }
    }
}

/****************************************************************************
 * Name: test_tcp_listen_again
 *
 * Description:
 *   listen() on a listening socket returns 0, sets the new backlog and
 *   keeps the connection that waits in the queue.
 *
 ****************************************************************************/

void test_tcp_listen_again(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int client0 = tcp_bl_socket(st, 0, false);
  int client1 = tcp_bl_socket(st, SOCK_NONBLOCK, false);
  in_port_t port;
  int listener;
  int ret;

  /* client0 waits in the backlog of one connection */

  listener = tcp_bl_listener(st, false, 1, &port);
  assert_int_equal(tcp_bl_connect(client0, port), 0);
  tcp_bl_wait(listener, POLLIN);

  assert_return_code(listen(listener, 2), errno);

  /* The new backlog has room for client1 too */

  ret = tcp_bl_connect(client1, port);
  assert_true(ret == 0 || ret == -EINPROGRESS);
  tcp_bl_wait(client1, POLLOUT);

  tcp_bl_close(st, tcp_bl_accept(st, listener));
  tcp_bl_close(st, tcp_bl_accept(st, listener));
}

/****************************************************************************
 * Name: test_tcp_listen_recv
 *
 * Description:
 *   recv() on a listener and on a socket that never connected fails with
 *   ENOTCONN.  A connection whose reader does not read first takes the
 *   free I/O buffers down to the throttle.
 *
 ****************************************************************************/

void test_tcp_listen_recv(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int client = tcp_bl_socket(st, SOCK_NONBLOCK, false);
  int unconnected = tcp_bl_socket(st, 0, false);
  FAR char *buf;
  in_port_t port;
  int listener;
  int ret;
  int i;
  char c;

  listener = tcp_bl_listener(st, false, 1, &port);
  ret = tcp_bl_connect(client, port);
  assert_true(ret == 0 || ret == -EINPROGRESS);
  tcp_bl_wait(client, POLLOUT);
  tcp_bl_accept(st, listener);

  /* The accepted socket does not read: the sends stop when the free I/O
   * buffers reach the throttle (EAGAIN).
   */

  buf = malloc(TEST_FILL_SIZE);
  assert_non_null(buf);
  memset(buf, 'f', TEST_FILL_SIZE);
  for (i = 0; i < TEST_FILL_MAX; i++)
    {
      if (send(client, buf, TEST_FILL_SIZE, 0) < 0)
        {
          break;
        }
    }

  free(buf);

  assert_int_equal(recv(unconnected, &c, 1, MSG_DONTWAIT), -1);
  assert_int_equal(errno, ENOTCONN);
  assert_int_equal(recv(listener, &c, 1, MSG_DONTWAIT), -1);
  assert_int_equal(errno, ENOTCONN);
}

/****************************************************************************
 * Name: test_tcp_listen_connected
 *
 * Description:
 *   listen() on a connected socket fails with EINVAL.
 *
 ****************************************************************************/

void test_tcp_listen_connected(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int client = tcp_bl_socket(st, 0, false);
  in_port_t port;
  int listener;
  int server;

  listener = tcp_bl_listener(st, false, 1, &port);
  assert_int_equal(tcp_bl_connect(client, port), 0);
  server = tcp_bl_accept(st, listener);

  assert_int_equal(listen(client, 1), -1);
  assert_int_equal(errno, EINVAL);
  assert_int_equal(listen(server, 1), -1);
  assert_int_equal(errno, EINVAL);
}

/****************************************************************************
 * Name: test_tcp_connect_listener
 *
 * Description:
 *   connect() on a listening socket fails with EISCONN (Linux), and the
 *   socket still listens.
 *
 ****************************************************************************/

void test_tcp_connect_listener(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int client = tcp_bl_socket(st, 0, false);
  int other = tcp_bl_socket(st, 0, false);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  in_port_t free_port;
  in_port_t port;
  int listener;

  /* A free port: bind a socket to port 0, then close it */

  assert_int_equal(tcp_bl_bind(other, 0), 0);
  assert_return_code(getsockname(other, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);
  free_port = addr.sin_port;
  tcp_bl_close(st, other);

  listener = tcp_bl_listener(st, false, 1, &port);
  assert_int_equal(tcp_bl_connect(listener, free_port), -EISCONN);
  assert_int_equal(tcp_bl_connect(listener, port), -EISCONN);

  assert_int_equal(tcp_bl_connect(client, port), 0);
  tcp_bl_accept(st, listener);
}

/****************************************************************************
 * Name: test_tcp_connect_family
 *
 * Description:
 *   connect() of an AF_INET socket to an address of another family fails
 *   with EAFNOSUPPORT.
 *
 ****************************************************************************/

void test_tcp_connect_family(FAR void **state)
{
  FAR struct tcp_bl_state_s *st = *state;
  int fd = tcp_bl_socket(st, 0, false);
  struct sockaddr_storage addr;

  memset(&addr, 0, sizeof(addr));
  addr.ss_family = AF_UNIX;
  assert_int_equal(connect(fd, (FAR struct sockaddr *)&addr, sizeof(addr)),
                   -1);
  assert_int_equal(errno, EAFNOSUPPORT);
}
