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
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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
#endif /* CONFIG_NET_SOCKOPTS */

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
