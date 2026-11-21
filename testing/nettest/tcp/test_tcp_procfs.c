/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_procfs.c
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

/* /proc/net/tcp with many IPv4 TCP connections on the loopback address:
 * a read of the file to its end ends, and lists each connection once, also
 * when connections go away during the read.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
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
#include <sys/socket.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

#ifdef TEST_TCP_PROCFS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCP_PROCFS_PATH    "/proc/net/tcp"
#define TCP_PROCFS_BUF     1024
#define TCP_PROCFS_CONNS   300  /* Connections closed by the client */
#define TCP_PROCFS_OWN     (2 * TCP_PROCFS_CONNS)
#define TCP_PROCFS_PAIRS   8    /* Open pairs closed during the read */
#define TCP_PROCFS_MAXFDS  (2 * TCP_PROCFS_PAIRS + 6)

/* A read with more lines than this does not end: other cases can leave a
 * few thousand connections in TIME_WAIT.
 */

#define TCP_PROCFS_MAXLINE 8192

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The sockets of the case: the teardown closes the ones that a failed case
 * leaves open.
 */

struct tcp_procfs_state_s
{
  int fds[TCP_PROCFS_MAXFDS];
};

/* The lines of the connections of one listener in a read of the file: the
 * lines with its port as the local or the remote port (only the remote
 * port, the client side, with client true), and the lines of a connection
 * that came before.
 */

struct tcp_procfs_seen_s
{
  int port;
  bool client;
  int own;
  int dups;
  int nkeys;
  uint32_t keys[TCP_PROCFS_OWN];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_procfs_keep
 ****************************************************************************/

static int tcp_procfs_keep(FAR struct tcp_procfs_state_s *st, int fd)
{
  int i;

  assert_true(fd >= 0);
  for (i = 0; i < TCP_PROCFS_MAXFDS; i++)
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
 * Name: tcp_procfs_fd
 ****************************************************************************/

static void tcp_procfs_fd(FAR struct tcp_procfs_state_s *st, int fd)
{
  int i;

  for (i = 0; i < TCP_PROCFS_MAXFDS; i++)
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
 * Name: tcp_procfs_listen
 *
 * Description:
 *   Make a listener on 127.0.0.1 with a free port.  addr gets its address.
 *
 ****************************************************************************/

static int tcp_procfs_listen(FAR struct tcp_procfs_state_s *st,
                             FAR struct sockaddr_in *addr)
{
  socklen_t addrlen;
  int fd;

  fd = tcp_procfs_keep(st, socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0));
  addrlen = nettest_lo_addr((FAR struct sockaddr *)addr, AF_INET);
  addr->sin_port = 0;
  assert_return_code(bind(fd, (FAR struct sockaddr *)addr, addrlen),
                     errno);
  assert_return_code(listen(fd, 4), errno);
  assert_return_code(getsockname(fd, (FAR struct sockaddr *)addr,
                                 &addrlen), errno);
  return fd;
}

/****************************************************************************
 * Name: tcp_procfs_timewait
 *
 * Description:
 *   Make n connections to the listener at addr, each closed by the client
 *   first, so that each stays in TIME_WAIT on the client side.
 *
 ****************************************************************************/

static void tcp_procfs_timewait(FAR struct tcp_procfs_state_s *st,
                                int listener,
                                FAR const struct sockaddr_in *addr, int n)
{
  int client;
  int server;
  int i;

  for (i = 0; i < n; i++)
    {
      client = tcp_procfs_keep(st, socket(AF_INET,
                                          SOCK_STREAM | SOCK_CLOEXEC, 0));
      assert_return_code(connect(client, (FAR const struct sockaddr *)addr,
                                 sizeof(*addr)), errno);
      server = tcp_procfs_keep(st, accept4(listener, NULL, NULL,
                                           SOCK_CLOEXEC));
      tcp_procfs_fd(st, client);
      tcp_procfs_fd(st, server);
    }
}

/****************************************************************************
 * Name: tcp_procfs_line
 *
 * Description:
 *   Count one line of the file in each of the nseen listeners that it
 *   belongs to.
 *
 ****************************************************************************/

static void tcp_procfs_line(FAR char *line,
                            FAR struct tcp_procfs_seen_s *seen, int nseen)
{
  FAR struct tcp_procfs_seen_s *s;
  FAR char *laddr;
  FAR char *raddr;
  uint32_t key;
  int lport;
  int rport;
  int i;

  laddr = strstr(line, "127.0.0.1:");
  raddr = laddr != NULL ? strstr(laddr + 10, "127.0.0.1:") : NULL;
  if (raddr == NULL)
    {
      return;
    }

  lport = atoi(laddr + 10);
  rport = atoi(raddr + 10);
  key   = ((uint32_t)lport << 16) | (uint32_t)rport;
  for (s = seen; s < seen + nseen; s++)
    {
      if (rport != s->port && (s->client || lport != s->port))
        {
          continue;
        }

      s->own++;
      for (i = 0; i < s->nkeys && s->keys[i] != key; i++)
        {
        }

      if (i < s->nkeys)
        {
          s->dups++;
        }
      else if (s->nkeys < TCP_PROCFS_OWN)
        {
          s->keys[s->nkeys++] = key;
        }
    }
}

/****************************************************************************
 * Name: tcp_procfs_part
 *
 * Description:
 *   One read() of the open file, of at most TCP_PROCFS_BUF bytes: each
 *   read() gives whole lines.  *lines gets the number of lines after the
 *   header.  Return the result of read().
 *
 ****************************************************************************/

static ssize_t tcp_procfs_part(int fd, FAR struct tcp_procfs_seen_s *seen,
                               int nseen, FAR int *lines)
{
  char buf[TCP_PROCFS_BUF];
  FAR char *line;
  FAR char *save;
  ssize_t n;

  n = read(fd, buf, sizeof(buf) - 1);
  if (n <= 0)
    {
      return n;
    }

  buf[n] = '\0';
  for (line = strtok_r(buf, "\n", &save); line != NULL;
       line = strtok_r(NULL, "\n", &save))
    {
      if (strncmp(line, "TCP", 3) != 0)
        {
          (*lines)++;
          tcp_procfs_line(line, seen, nseen);
        }
    }

  return n;
}

/****************************************************************************
 * Name: tcp_procfs_rest
 *
 * Description:
 *   Read the open file to its end, or until it has more than
 *   TCP_PROCFS_MAXLINE lines (a read that does not end).
 *
 ****************************************************************************/

static void tcp_procfs_rest(int fd, FAR struct tcp_procfs_seen_s *seen,
                            int nseen, FAR int *lines)
{
  while (*lines <= TCP_PROCFS_MAXLINE &&
         tcp_procfs_part(fd, seen, nseen, lines) > 0)
    {
    }
}

#endif /* TEST_TCP_PROCFS */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef TEST_TCP_PROCFS

/****************************************************************************
 * Name: test_tcp_procfs_setup
 ****************************************************************************/

int test_tcp_procfs_setup(FAR void **state)
{
  FAR struct tcp_procfs_state_s *st = malloc(sizeof(*st));
  int i;

  assert_non_null(st);
  for (i = 0; i < TCP_PROCFS_MAXFDS; i++)
    {
      st->fds[i] = -1;
    }

  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_procfs_teardown
 ****************************************************************************/

int test_tcp_procfs_teardown(FAR void **state)
{
  FAR struct tcp_procfs_state_s *st = *state;
  int i;

  for (i = 0; i < TCP_PROCFS_MAXFDS; i++)
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
 * Name: test_tcp_procfs_many
 *
 * Description:
 *   TCP_PROCFS_CONNS connections to a listener on 127.0.0.1, each closed
 *   by the client first, so that each stays in TIME_WAIT on the client
 *   side.  Then a read of /proc/net/tcp must end within
 *   TCP_PROCFS_MAXLINE lines (more lines mean that the read starts again
 *   from the head of the table), and must list each connection of the
 *   listener once: at least TCP_PROCFS_CONNS lines and at most
 *   TCP_PROCFS_OWN.  Connections of earlier cases are not counted.
 *
 ****************************************************************************/

void test_tcp_procfs_many(FAR void **state)
{
  FAR struct tcp_procfs_state_s *st = *state;
  FAR struct tcp_procfs_seen_s *seen;
  struct sockaddr_in addr;
  int listener;
  int lines = 0;
  int fd;

  seen = calloc(1, sizeof(*seen));
  assert_non_null(seen);

  listener = tcp_procfs_listen(st, &addr);
  tcp_procfs_timewait(st, listener, &addr, TCP_PROCFS_CONNS);

  seen->port = ntohs(addr.sin_port);
  fd = open(TCP_PROCFS_PATH, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      free(seen);
      fail_msg("open %s: %d", TCP_PROCFS_PATH, errno);
    }

  tcp_procfs_rest(fd, seen, 1, &lines);
  close(fd);

  printf("/proc/net/tcp: %d lines, %d of the listener, %d more than once\n",
         lines, seen->own, seen->dups);
  if (lines > TCP_PROCFS_MAXLINE || seen->own < TCP_PROCFS_CONNS ||
      seen->own > TCP_PROCFS_OWN || seen->dups != 0)
    {
      free(seen);
      fail();
    }

  free(seen);
  tcp_procfs_fd(st, listener);
}

/****************************************************************************
 * Name: test_tcp_procfs_close
 *
 * Description:
 *   Connections that go away while /proc/net/tcp is read must not hide
 *   other ones.  TCP_PROCFS_PAIRS open pairs to a first listener, then
 *   TCP_PROCFS_CONNS connections to a second one, which stay in TIME_WAIT
 *   on the client side: they come after the pairs in the table.  The file
 *   is read until all the lines of the pairs are there, then both sides of
 *   each pair reset their connection (SO_LINGER 0), and the file is read
 *   to its end.  The read must list the client side of each connection to
 *   the second listener once.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_SOLINGER
void test_tcp_procfs_close(FAR void **state)
{
  FAR struct tcp_procfs_state_s *st = *state;
  FAR struct tcp_procfs_seen_s *seen;
  struct sockaddr_in addr1;
  struct sockaddr_in addr2;
  struct linger linger;
  int pairs[2 * TCP_PROCFS_PAIRS];
  int listener1;
  int listener2;
  int lines = 0;
  bool ok;
  int fd;
  int i;

  seen = calloc(2, sizeof(*seen));
  assert_non_null(seen);

  listener1 = tcp_procfs_listen(st, &addr1);
  listener2 = tcp_procfs_listen(st, &addr2);

  for (i = 0; i < TCP_PROCFS_PAIRS; i++)
    {
      pairs[2 * i] = tcp_procfs_keep(st, socket(AF_INET,
                                                SOCK_STREAM | SOCK_CLOEXEC,
                                                0));
      assert_return_code(connect(pairs[2 * i],
                                 (FAR struct sockaddr *)&addr1,
                                 sizeof(addr1)), errno);
      pairs[2 * i + 1] = tcp_procfs_keep(st, accept4(listener1, NULL, NULL,
                                                     SOCK_CLOEXEC));
    }

  tcp_procfs_timewait(st, listener2, &addr2, TCP_PROCFS_CONNS);

  seen[0].port   = ntohs(addr1.sin_port);
  seen[1].port   = ntohs(addr2.sin_port);
  seen[1].client = true;
  fd = open(TCP_PROCFS_PATH, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      free(seen);
      fail_msg("open %s: %d", TCP_PROCFS_PATH, errno);
    }

  /* Read until the lines of both sides of each pair are there */

  while (seen[0].own < 2 * TCP_PROCFS_PAIRS && lines <= TCP_PROCFS_MAXLINE &&
         tcp_procfs_part(fd, seen, 2, &lines) > 0)
    {
    }

  /* The pairs go away: a reset on both sides frees their connections */

  linger.l_onoff  = 1;
  linger.l_linger = 0;
  for (i = 0; i < 2 * TCP_PROCFS_PAIRS; i++)
    {
      assert_return_code(setsockopt(pairs[i], SOL_SOCKET, SO_LINGER,
                                    &linger, sizeof(linger)), errno);
      tcp_procfs_fd(st, pairs[i]);
    }

  usleep(500 * 1000);
  tcp_procfs_rest(fd, seen, 2, &lines);
  close(fd);

  printf("/proc/net/tcp: %d lines, %d of the pairs, %d clients of the "
         "second listener, %d more than once\n",
         lines, seen[0].own, seen[1].own, seen[0].dups + seen[1].dups);
  ok = lines <= TCP_PROCFS_MAXLINE && seen[0].own == 2 * TCP_PROCFS_PAIRS &&
       seen[1].own == TCP_PROCFS_CONNS && seen[0].dups == 0 &&
       seen[1].dups == 0;
  free(seen);
  assert_true(ok);

  tcp_procfs_fd(st, listener1);
  tcp_procfs_fd(st, listener2);
}
#endif

#endif /* TEST_TCP_PROCFS */
