/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_backlog.c
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

/* The backlog of an IPv4 TCP listener on the loopback address: listen()
 * again on a listener with connections that wait in its backlog, and the
 * close of a listener while connections to it come in.  The race cases
 * pass when all their rounds end.
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
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

#ifdef CONFIG_NET_TCPBACKLOG

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCP_BACKLOG_MAXFDS   8
#define TCP_BACKLOG_CONNECT  2000  /* ms for a connect() that must end */
#define TCP_BACKLOG_TRY      500   /* ms for a connect() of a race client */
#define TCP_BACKLOG_POLL     100   /* ms of a poll() of the busy server */
#define TCP_BACKLOG_ROUNDS   10    /* Rounds of the listen() again case */
#define TCP_BACKLOG_RACES    50    /* Rounds of a race case */
#define TCP_BACKLOG_BUSY     2     /* Clients that keep the input busy */
#define TCP_BACKLOG_MAXRACE  6     /* Clients of one race round */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The threads of a race case: connections to a busy listener that keep
 * the loopback input busy, and the clients of one round.  The teardown
 * waits for the threads that a failed case leaves.
 */

struct tcp_backlog_race_s
{
  struct sockaddr_in busy;
  struct sockaddr_in target;
  volatile bool stop;
  int busyfd;
  int nbusy;
  int nclients;
  bool server;
  pthread_t servertid;
  pthread_t busytid[TCP_BACKLOG_BUSY];
  pthread_t clienttid[TCP_BACKLOG_MAXRACE];
};

/* The sockets and threads of one case: the teardown closes the sockets
 * and stops the threads that a failed case leaves.
 */

struct tcp_backlog_state_s
{
  int fds[TCP_BACKLOG_MAXFDS];
  struct tcp_backlog_race_s race;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_backlog_keep
 ****************************************************************************/

static int tcp_backlog_keep(FAR struct tcp_backlog_state_s *st, int fd)
{
  int i;

  assert_true(fd >= 0);
  for (i = 0; i < TCP_BACKLOG_MAXFDS; i++)
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
 * Name: tcp_backlog_fd
 ****************************************************************************/

static void tcp_backlog_fd(FAR struct tcp_backlog_state_s *st, int fd)
{
  int i;

  for (i = 0; i < TCP_BACKLOG_MAXFDS; i++)
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
 * Name: tcp_backlog_listen
 *
 * Description:
 *   Make a listener on 127.0.0.1 with a free port.  addr gets its address.
 *
 ****************************************************************************/

static int tcp_backlog_listen(FAR struct sockaddr_in *addr, int backlog)
{
  socklen_t addrlen;
  int fd;

  fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_true(fd >= 0);
  addrlen = nettest_lo_addr((FAR struct sockaddr *)addr, AF_INET);
  addr->sin_port = 0;
  assert_return_code(bind(fd, (FAR struct sockaddr *)addr, addrlen),
                     errno);
  assert_return_code(listen(fd, backlog), errno);
  assert_return_code(getsockname(fd, (FAR struct sockaddr *)addr,
                                 &addrlen), errno);
  return fd;
}

/****************************************************************************
 * Name: tcp_backlog_connect
 *
 * Description:
 *   Connect fd to addr and wait at most ms for the end of the handshake.
 *   Return 0 or an errno value (ETIMEDOUT if it did not end).  fd stays
 *   non-blocking.
 *
 ****************************************************************************/

static int tcp_backlog_connect(int fd, FAR const struct sockaddr_in *addr,
                               int ms)
{
  struct pollfd pfd;
  socklen_t len;
  int err;

  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0)
    {
      return errno;
    }

  if (connect(fd, (FAR const struct sockaddr *)addr, sizeof(*addr)) == 0)
    {
      return 0;
    }

  if (errno != EINPROGRESS)
    {
      return errno;
    }

  pfd.fd      = fd;
  pfd.events  = POLLOUT;
  pfd.revents = 0;
  if (poll(&pfd, 1, ms) <= 0)
    {
      return ETIMEDOUT;
    }

  err = 0;
  len = sizeof(err);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    {
      return errno;
    }

  return err;
}

/****************************************************************************
 * Name: tcp_backlog_port
 *
 * Description:
 *   Return the local port (peer false) or the remote port (peer true) of
 *   fd in host order, or -1.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOLINGER
static int tcp_backlog_port(int fd, bool peer)
{
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int ret;

  if (peer)
    {
      ret = getpeername(fd, (FAR struct sockaddr *)&addr, &addrlen);
    }
  else
    {
      ret = getsockname(fd, (FAR struct sockaddr *)&addr, &addrlen);
    }

  return ret < 0 ? -1 : ntohs(addr.sin_port);
}
#endif

/****************************************************************************
 * Name: tcp_backlog_try
 *
 * Description:
 *   One connection of a race client: connect to addr, wait at most
 *   TCP_BACKLOG_TRY ms, then close.  Any result is good: the listener may
 *   be gone, or its backlog full.
 *
 ****************************************************************************/

static void tcp_backlog_try(FAR const struct sockaddr_in *addr)
{
  int fd;

  fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd >= 0)
    {
      tcp_backlog_connect(fd, addr, TCP_BACKLOG_TRY);
      close(fd);
    }
}

/****************************************************************************
 * Name: tcp_backlog_busy_server
 *
 * Description:
 *   Accept and close the connections to the busy listener until the case
 *   stops.  poll() ends within TCP_BACKLOG_POLL ms, so the thread sees the
 *   stop; the listener is non-blocking, so accept() does not wait.
 *
 ****************************************************************************/

static FAR void *tcp_backlog_busy_server(FAR void *arg)
{
  FAR struct tcp_backlog_race_s *race = arg;
  struct pollfd pfd;
  int fd;

  while (!race->stop)
    {
      pfd.fd      = race->busyfd;
      pfd.events  = POLLIN;
      pfd.revents = 0;
      if (poll(&pfd, 1, TCP_BACKLOG_POLL) > 0)
        {
          fd = accept4(race->busyfd, NULL, NULL, SOCK_CLOEXEC);
          if (fd >= 0)
            {
              close(fd);
            }
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: tcp_backlog_busy_client
 ****************************************************************************/

static FAR void *tcp_backlog_busy_client(FAR void *arg)
{
  FAR struct tcp_backlog_race_s *race = arg;

  while (!race->stop)
    {
      tcp_backlog_try(&race->busy);
    }

  return NULL;
}

/****************************************************************************
 * Name: tcp_backlog_race_client
 ****************************************************************************/

static FAR void *tcp_backlog_race_client(FAR void *arg)
{
  FAR struct tcp_backlog_race_s *race = arg;

  tcp_backlog_try(&race->target);
  return NULL;
}

/****************************************************************************
 * Name: tcp_backlog_race_join
 *
 * Description:
 *   Wait for the clients of a race round.
 *
 ****************************************************************************/

static void tcp_backlog_race_join(FAR struct tcp_backlog_race_s *race)
{
  while (race->nclients > 0)
    {
      pthread_join(race->clienttid[--race->nclients], NULL);
    }
}

/****************************************************************************
 * Name: tcp_backlog_race_stop
 *
 * Description:
 *   Wait for the clients of a race case, stop its busy threads and wait for
 *   them.
 *
 ****************************************************************************/

static void tcp_backlog_race_stop(FAR struct tcp_backlog_race_s *race)
{
  int i;

  tcp_backlog_race_join(race);
  race->stop = true;
  for (i = 0; i < race->nbusy; i++)
    {
      pthread_join(race->busytid[i], NULL);
    }

  race->nbusy = 0;
  if (race->server)
    {
      pthread_join(race->servertid, NULL);
      race->server = false;
    }
}

/****************************************************************************
 * Name: tcp_backlog_race
 *
 * Description:
 *   Each round: a listener; nclients threads connect to it at once; with
 *   relisten, listen() again with a backlog of 1 while they connect; then
 *   the close of the listener.  Meanwhile busy threads keep the loopback
 *   input busy with connections to another listener.  So the stack frees
 *   connections of the listener (half-open ones, and the pending ones in
 *   its backlog) while the input path allocates, looks up and adds
 *   connections.  Each round must end.
 *
 ****************************************************************************/

static void tcp_backlog_race(FAR struct tcp_backlog_state_s *st,
                             int nclients, int backlog, bool relisten)
{
  FAR struct tcp_backlog_race_s *race = &st->race;
  int listener;
  int round;
  int i;

  race->busyfd = tcp_backlog_keep(st, tcp_backlog_listen(&race->busy, 8));
  assert_return_code(fcntl(race->busyfd, F_SETFL,
                           fcntl(race->busyfd, F_GETFL) | O_NONBLOCK),
                     errno);

  race->stop = false;
  assert_int_equal(pthread_create(&race->servertid, NULL,
                                  tcp_backlog_busy_server, race), 0);
  race->server = true;

  for (i = 0; i < TCP_BACKLOG_BUSY; i++)
    {
      assert_int_equal(pthread_create(&race->busytid[i], NULL,
                                      tcp_backlog_busy_client, race), 0);
      race->nbusy++;
    }

  for (round = 0; round < TCP_BACKLOG_RACES; round++)
    {
      listener = tcp_backlog_keep(st, tcp_backlog_listen(&race->target,
                                                         backlog));
      for (i = 0; i < nclients; i++)
        {
          assert_int_equal(pthread_create(&race->clienttid[i], NULL,
                                          tcp_backlog_race_client, race),
                           0);
          race->nclients++;
        }

      if (relisten)
        {
          usleep(1000 * (round % 4));
          assert_return_code(listen(listener, 1), errno);
        }

      tcp_backlog_fd(st, listener);
      tcp_backlog_race_join(race);
    }

  tcp_backlog_race_stop(race);
  tcp_backlog_fd(st, race->busyfd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_backlog_setup
 ****************************************************************************/

int test_tcp_backlog_setup(FAR void **state)
{
  FAR struct tcp_backlog_state_s *st = calloc(1, sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TCP_BACKLOG_MAXFDS; i++)
    {
      st->fds[i] = -1;
    }

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_backlog_teardown
 ****************************************************************************/

int test_tcp_backlog_teardown(FAR void **state)
{
  FAR struct tcp_backlog_state_s *st = *state;
  int i;

  tcp_backlog_race_stop(&st->race);
  for (i = 0; i < TCP_BACKLOG_MAXFDS; i++)
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
 * Name: test_tcp_backlog_relisten_rst
 *
 * Description:
 *   listen() again moves the connections that wait in the backlog to the
 *   new backlog.  A moved connection that its peer then resets must leave
 *   the backlog: client A connects to a listener of backlog 4, listen()
 *   again gives a backlog of 1, A resets its connection (SO_LINGER 0).
 *   Then the connect() of client C must end within TCP_BACKLOG_CONNECT ms
 *   (a backlog still full of A drops the SYN of C), and accept() gives C.
 *   accept() may first give the reset connection of A.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOLINGER
void test_tcp_backlog_relisten_rst(FAR void **state)
{
  FAR struct tcp_backlog_state_s *st = *state;
  struct sockaddr_in addr;
  struct linger linger;
  struct pollfd pfd;
  int listener;
  int aport;
  int cport;
  int port;
  int round;
  int fd;
  int a;
  int c;
  int i;

  for (round = 0; round < TCP_BACKLOG_ROUNDS; round++)
    {
      listener = tcp_backlog_keep(st, tcp_backlog_listen(&addr, 4));

      a = tcp_backlog_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC,
                                      0));
      assert_int_equal(tcp_backlog_connect(a, &addr, TCP_BACKLOG_CONNECT),
                       0);
      aport = tcp_backlog_port(a, false);
      usleep(100 * 1000);

      assert_return_code(listen(listener, 1), errno);

      linger.l_onoff  = 1;
      linger.l_linger = 0;
      assert_return_code(setsockopt(a, SOL_SOCKET, SO_LINGER, &linger,
                                    sizeof(linger)), errno);
      tcp_backlog_fd(st, a);
      usleep(100 * 1000);

      c = tcp_backlog_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC,
                                      0));
      assert_int_equal(tcp_backlog_connect(c, &addr, TCP_BACKLOG_CONNECT),
                       0);
      cport = tcp_backlog_port(c, false);

      /* accept() gives C, maybe after the reset connection of A */

      for (i = 0; i < 2; i++)
        {
          pfd.fd      = listener;
          pfd.events  = POLLIN;
          pfd.revents = 0;
          assert_int_equal(poll(&pfd, 1, TCP_BACKLOG_CONNECT), 1);

          fd = tcp_backlog_keep(st, accept4(listener, NULL, NULL,
                                            SOCK_CLOEXEC));
          port = tcp_backlog_port(fd, true);
          tcp_backlog_fd(st, fd);
          if (port == cport)
            {
              break;
            }

          assert_true(port == aport || port < 0);
        }

      assert_true(i < 2);
      tcp_backlog_fd(st, c);
      tcp_backlog_fd(st, listener);
    }
}
#endif

/****************************************************************************
 * Name: test_tcp_backlog_relisten_race
 *
 * Description:
 *   TCP_BACKLOG_RACES rounds: a listener of backlog 8, 6 clients connect
 *   at once, listen() again with a backlog of 1 while they connect, then
 *   the close of the listener, with the loopback input busy.  The pending
 *   connections of the old backlog and of the listener are freed while
 *   the input path adds new ones.  Each round must end.
 *
 ****************************************************************************/

void test_tcp_backlog_relisten_race(FAR void **state)
{
  tcp_backlog_race(*state, 6, 8, true);
}

/****************************************************************************
 * Name: test_tcp_backlog_close_race
 *
 * Description:
 *   TCP_BACKLOG_RACES rounds: a listener of backlog 4, 4 clients connect
 *   at once, and the listener closes while their handshakes run, with the
 *   loopback input busy.  The close frees the half-open connections of the
 *   listener while the input path allocates and looks up connections.
 *   Each round must end.
 *
 ****************************************************************************/

void test_tcp_backlog_close_race(FAR void **state)
{
  tcp_backlog_race(*state, 4, 4, false);
}

#endif /* CONFIG_NET_TCPBACKLOG */
