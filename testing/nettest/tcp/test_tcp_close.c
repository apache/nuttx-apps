/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_close.c
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

/* The end of IPv4 TCP connections on the loopback address: shutdown() and
 * close() with data queued or unread, connections in TIME_WAIT, and a
 * device that goes down.  The cases look in /proc/net/tcp for the
 * connections that they closed: these must be freed.  Without
 * /proc/net/tcp those checks are skipped.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCP_CLOSE_PROC    "/proc/net/tcp"
#define TCP_CLOSE_PROCBUF 1024
#define TCP_CLOSE_MAXFDS  8
#define TCP_CLOSE_CHUNK   1024
#define TCP_CLOSE_MAXFILL (4 * 1024 * 1024)
#define TCP_CLOSE_RCVTMO  10    /* s, SO_RCVTIMEO of a reader */
#define TCP_CLOSE_SETTLE  3000  /* ms for a closed connection to go away */
#define TCP_CLOSE_ROUNDS  5
#define TCP_CLOSE_TWDATA  512

/* Enough connections in TIME_WAIT to hold all preallocated callbacks when
 * each one keeps three of them.
 */

#ifdef CONFIG_NET_PREALLOC_DEVIF_CALLBACKS
#  define TCP_CLOSE_NTW   (CONFIG_NET_PREALLOC_DEVIF_CALLBACKS / 3 + 1)
#else
#  define TCP_CLOSE_NTW   6
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The sockets of one case: the teardown closes the ones that a failed case
 * leaves open.
 */

struct tcp_close_state_s
{
  int fds[TCP_CLOSE_MAXFDS];
};

/* A connection over 127.0.0.1 (ports in host order) */

struct tcp_close_pair_s
{
  int client;
  int server;
  int cport;
  int sport;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_close_keep
 ****************************************************************************/

static int tcp_close_keep(FAR struct tcp_close_state_s *st, int fd)
{
  int i;

  assert_true(fd >= 0);
  for (i = 0; i < TCP_CLOSE_MAXFDS; i++)
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
 * Name: tcp_close_fd
 ****************************************************************************/

static void tcp_close_fd(FAR struct tcp_close_state_s *st, int fd)
{
  int i;

  for (i = 0; i < TCP_CLOSE_MAXFDS; i++)
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
 * Name: tcp_close_timeouts
 *
 * Description:
 *   Give send() and recv() on fd a timeout, so that a failing case ends.
 *
 ****************************************************************************/

static void tcp_close_timeouts(int fd)
{
  struct timeval tv;

  tv.tv_sec  = TCP_CLOSE_RCVTMO;
  tv.tv_usec = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv,
                                sizeof(tv)), errno);
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
}

/****************************************************************************
 * Name: tcp_close_pair
 *
 * Description:
 *   Connect two sockets over 127.0.0.1, with send and receive timeouts.
 *   The listener is closed again.
 *
 ****************************************************************************/

static void tcp_close_pair(FAR struct tcp_close_state_s *st,
                           FAR struct tcp_close_pair_s *pair)
{
  struct sockaddr_in addr;
  socklen_t addrlen;
  int listener;

  listener = tcp_close_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC,
                                          0));
  addrlen = nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  addr.sin_port = 0;
  assert_return_code(bind(listener, (FAR struct sockaddr *)&addr,
                          addrlen), errno);
  assert_return_code(listen(listener, 1), errno);
  assert_return_code(getsockname(listener, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);
  pair->sport = ntohs(addr.sin_port);

  pair->client = tcp_close_keep(st, socket(AF_INET,
                                           SOCK_STREAM | SOCK_CLOEXEC, 0));
  assert_return_code(connect(pair->client, (FAR struct sockaddr *)&addr,
                             addrlen), errno);
  pair->server = tcp_close_keep(st, accept4(listener, NULL, NULL,
                                            SOCK_CLOEXEC));
  tcp_close_fd(st, listener);

  addrlen = sizeof(addr);
  assert_return_code(getsockname(pair->client, (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);
  pair->cport = ntohs(addr.sin_port);
  tcp_close_timeouts(pair->client);
  tcp_close_timeouts(pair->server);
}

/****************************************************************************
 * Name: tcp_close_find
 *
 * Description:
 *   Return the state of the connection with the local port lport and the
 *   remote port rport in /proc/net/tcp, -1 if it is not there, or -2 if
 *   there is no /proc/net/tcp.  Each read() gives whole lines.
 *
 ****************************************************************************/

static int tcp_close_find(int lport, int rport)
{
  char buf[TCP_CLOSE_PROCBUF];
  FAR char *line;
  FAR char *save;
  FAR char *laddr;
  FAR char *raddr;
  FAR char *colon;
  ssize_t n;
  int state = -1;
  int fd;

  fd = open(TCP_CLOSE_PROC, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return -2;
    }

  while (state < 0 && (n = read(fd, buf, sizeof(buf) - 1)) > 0)
    {
      buf[n] = '\0';
      for (line = strtok_r(buf, "\n", &save); line != NULL && state < 0;
           line = strtok_r(NULL, "\n", &save))
        {
          laddr = strstr(line, "127.0.0.1:");
          raddr = laddr != NULL ? strstr(laddr + 10, "127.0.0.1:") : NULL;
          colon = strchr(line, ':');
          if (raddr != NULL && colon != NULL &&
              atoi(laddr + 10) == lport && atoi(raddr + 10) == rport)
            {
              state = strtol(colon + 1, NULL, 16);
            }
        }
    }

  close(fd);
  return state;
}

/****************************************************************************
 * Name: tcp_close_gone
 *
 * Description:
 *   Wait until the connection lport:rport is no longer in /proc/net/tcp.
 *   Return true when it is gone or when there is no /proc/net/tcp.
 *
 ****************************************************************************/

static bool tcp_close_gone(int lport, int rport)
{
  int ms;
  int state;

  for (ms = 0; ms < TCP_CLOSE_SETTLE; ms += 50)
    {
      state = tcp_close_find(lport, rport);
      if (state < 0)
        {
          return true;
        }

      usleep(50000);
    }

  printf("connection %d -> %d is still there, state %02x\n", lport, rport,
         state);
  return false;
}

/****************************************************************************
 * Name: tcp_close_byte
 ****************************************************************************/

static uint8_t tcp_close_byte(size_t i)
{
  return i * 13 + i / 251;
}

/****************************************************************************
 * Name: tcp_close_fill
 *
 * Description:
 *   Send the pattern on fd without blocking until it takes no more.
 *   Return the number of bytes sent.
 *
 ****************************************************************************/

static size_t tcp_close_fill(int fd)
{
  uint8_t buf[TCP_CLOSE_CHUNK];
  size_t sent = 0;
  ssize_t n;
  int again = 0;
  size_t i;

  assert_return_code(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK),
                     errno);
  while (again < 2 && sent < TCP_CLOSE_MAXFILL)
    {
      for (i = 0; i < sizeof(buf); i++)
        {
          buf[i] = tcp_close_byte(sent + i);
        }

      n = send(fd, buf, sizeof(buf), 0);
      if (n > 0)
        {
          sent += n;
          again = 0;
        }
      else
        {
          assert_true(n < 0 && errno == EAGAIN);
          again++;
          usleep(50000);
        }
    }

  return sent;
}

/****************************************************************************
 * Name: tcp_close_drain
 *
 * Description:
 *   Read fd to the end: every byte of the pattern up to len, then EOF.
 *
 ****************************************************************************/

static void tcp_close_drain(int fd, size_t len)
{
  struct timeval tv;
  uint8_t buf[TCP_CLOSE_CHUNK];
  size_t got = 0;
  ssize_t n;
  ssize_t i;

  tv.tv_sec  = TCP_CLOSE_RCVTMO;
  tv.tv_usec = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                                sizeof(tv)), errno);
  while ((n = recv(fd, buf, sizeof(buf), 0)) > 0)
    {
      for (i = 0; i < n; i++)
        {
          assert_int_equal(buf[i], tcp_close_byte(got + i));
        }

      got += n;
    }

  if (n < 0 || got != len)
    {
      printf("read %zu of %zu bytes, then %s\n", got, len,
             n < 0 ? strerror(errno) : "EOF");
    }

  assert_int_equal(n, 0);
  assert_int_equal(got, len);
}

/****************************************************************************
 * Name: tcp_close_reset
 *
 * Description:
 *   Close fd with SO_LINGER {1, 0}: the connection is reset.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOLINGER
static void tcp_close_reset(FAR struct tcp_close_state_s *st, int fd)
{
  struct linger lg;

  lg.l_onoff  = 1;
  lg.l_linger = 0;
  assert_return_code(setsockopt(fd, SOL_SOCKET, SO_LINGER, &lg,
                                sizeof(lg)), errno);
  tcp_close_fd(st, fd);
}
#endif

/****************************************************************************
 * Name: tcp_close_lo
 *
 * Description:
 *   Take the loopback device down or up.
 *
 ****************************************************************************/

#if defined(CONFIG_NET_LOOPBACK) && defined(CONFIG_NET_SOLINGER)
static void tcp_close_lo(int fd, bool up)
{
  struct ifreq req;

  memset(&req, 0, sizeof(req));
  strlcpy(req.ifr_name, "lo", sizeof(req.ifr_name));
  assert_return_code(ioctl(fd, SIOCGIFFLAGS, &req), errno);
  if (up)
    {
      req.ifr_flags |= IFF_UP;
    }
  else
    {
      req.ifr_flags &= ~IFF_UP;
    }

  assert_return_code(ioctl(fd, SIOCSIFFLAGS, &req), errno);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_close_setup
 ****************************************************************************/

int test_tcp_close_setup(FAR void **state)
{
  FAR struct tcp_close_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TCP_CLOSE_MAXFDS; i++)
    {
      st->fds[i] = -1;
    }

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_close_teardown
 ****************************************************************************/

int test_tcp_close_teardown(FAR void **state)
{
  FAR struct tcp_close_state_s *st = *state;
  int i;

  for (i = 0; i < TCP_CLOSE_MAXFDS; i++)
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
 * Name: test_tcp_close_shutdown_queued
 *
 * Description:
 *   The client fills the window of the server and its own write queue,
 *   then calls shutdown(SHUT_WR).  The server must read every byte, in
 *   order, then EOF: the FIN goes after the queued data.
 *
 ****************************************************************************/

void test_tcp_close_shutdown_queued(FAR void **state)
{
  FAR struct tcp_close_state_s *st = *state;
  struct tcp_close_pair_s pair;
  size_t sent;
  int round;

  for (round = 0; round < TCP_CLOSE_ROUNDS; round++)
    {
      tcp_close_pair(st, &pair);
      sent = tcp_close_fill(pair.client);
      assert_return_code(shutdown(pair.client, SHUT_WR), errno);
      tcp_close_drain(pair.server, sent);
      tcp_close_fd(st, pair.server);
      tcp_close_fd(st, pair.client);
    }
}

/****************************************************************************
 * Name: test_tcp_close_shutdown_close
 *
 * Description:
 *   As above, but the client also closes its socket right after
 *   shutdown().  The server must read every byte, then EOF.  Then the
 *   server resets the connection: the connection of the client must be
 *   freed at once, not only after TIME_WAIT.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOLINGER
void test_tcp_close_shutdown_close(FAR void **state)
{
  FAR struct tcp_close_state_s *st = *state;
  struct tcp_close_pair_s pair;
  size_t sent;

  tcp_close_pair(st, &pair);
  sent = tcp_close_fill(pair.client);
  assert_return_code(shutdown(pair.client, SHUT_WR), errno);
  tcp_close_fd(st, pair.client);
  tcp_close_drain(pair.server, sent);
  tcp_close_reset(st, pair.server);
  assert_true(tcp_close_gone(pair.cport, pair.sport));
  assert_true(tcp_close_gone(pair.sport, pair.cport));
}
#endif

/****************************************************************************
 * Name: test_tcp_close_unread
 *
 * Description:
 *   Both ends send to each other until neither takes more, so that each
 *   has data of its own queued and data of the peer unread.  Then both
 *   close.  A close with unread data resets the connection (RFC 1122,
 *   4.2.2.13): both connections must be freed at once.
 *
 ****************************************************************************/

void test_tcp_close_unread(FAR void **state)
{
  FAR struct tcp_close_state_s *st = *state;
  struct tcp_close_pair_s pair;
  uint8_t buf[TCP_CLOSE_CHUNK];
  size_t total = 0;
  ssize_t a;
  ssize_t b;
  int idle = 0;

  tcp_close_pair(st, &pair);
  assert_return_code(fcntl(pair.client, F_SETFL,
                           fcntl(pair.client, F_GETFL) | O_NONBLOCK),
                     errno);
  assert_return_code(fcntl(pair.server, F_SETFL,
                           fcntl(pair.server, F_GETFL) | O_NONBLOCK),
                     errno);

  memset(buf, 'u', sizeof(buf));
  while (idle < 2 && total < TCP_CLOSE_MAXFILL)
    {
      a = send(pair.client, buf, sizeof(buf), 0);
      b = send(pair.server, buf, sizeof(buf), 0);
      if (a > 0 || b > 0)
        {
          total += (a > 0 ? a : 0) + (b > 0 ? b : 0);
          idle = 0;
        }
      else
        {
          idle++;
          usleep(50000);
        }
    }

  tcp_close_fd(st, pair.client);
  tcp_close_fd(st, pair.server);
  assert_true(tcp_close_gone(pair.cport, pair.sport));
  assert_true(tcp_close_gone(pair.sport, pair.cport));
}

/****************************************************************************
 * Name: test_tcp_close_netdown
 *
 * Description:
 *   The client closes its socket and waits for the server to close.  Then
 *   the loopback device goes down and up, and 200 ms later the server
 *   resets the connection.  The connection of the client must be freed:
 *   its close callback ends at the device down.
 *
 ****************************************************************************/

#if defined(CONFIG_NET_LOOPBACK) && defined(CONFIG_NET_SOLINGER)
void test_tcp_close_netdown(FAR void **state)
{
  FAR struct tcp_close_state_s *st = *state;
  struct tcp_close_pair_s pair;
  char c;

  tcp_close_pair(st, &pair);
  tcp_close_fd(st, pair.client);
  assert_int_equal(recv(pair.server, &c, 1, 0), 0);
  usleep(100000);

  tcp_close_lo(pair.server, false);
  tcp_close_lo(pair.server, true);
  usleep(200000);
  tcp_close_reset(st, pair.server);
  assert_true(tcp_close_gone(pair.cport, pair.sport));
}
#endif

/****************************************************************************
 * Name: test_tcp_close_timewait
 *
 * Description:
 *   Leave TCP_CLOSE_NTW connections in TIME_WAIT (the client closes
 *   first).  Then a new client fills the window of the server and its own
 *   write queue, and closes at once: the server must read every byte, then
 *   EOF.  The connections in TIME_WAIT must not hold the callbacks that
 *   this close needs.
 *
 ****************************************************************************/

void test_tcp_close_timewait(FAR void **state)
{
  FAR struct tcp_close_state_s *st = *state;
  struct tcp_close_pair_s pair;
  uint8_t buf[TCP_CLOSE_TWDATA];
  size_t sent;
  int conn;

  memset(buf, 't', sizeof(buf));
  for (conn = 0; conn < TCP_CLOSE_NTW; conn++)
    {
      tcp_close_pair(st, &pair);
      assert_int_equal(send(pair.client, buf, sizeof(buf), 0), sizeof(buf));
      assert_int_equal(recv(pair.server, buf, sizeof(buf), MSG_WAITALL),
                       sizeof(buf));
      assert_int_equal(send(pair.server, buf, sizeof(buf), 0), sizeof(buf));
      assert_int_equal(recv(pair.client, buf, sizeof(buf), MSG_WAITALL),
                       sizeof(buf));
      tcp_close_fd(st, pair.client);
      usleep(20000);
      tcp_close_fd(st, pair.server);
    }

  tcp_close_pair(st, &pair);
  sent = tcp_close_fill(pair.client);
  tcp_close_fd(st, pair.client);
  tcp_close_drain(pair.server, sent);
  tcp_close_fd(st, pair.server);
}
