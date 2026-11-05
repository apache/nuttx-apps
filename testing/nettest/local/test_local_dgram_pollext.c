/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_pollext.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOCAL_DGRAM_STRESSLEN  64
#define LOCAL_DGRAM_STRESSNUM  100
#define LOCAL_DGRAM_STRESSTIME 120

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int local_dgram_do_bind(FAR struct local_dgram_job_s *job)
{
  struct sockaddr_un addr;
  socklen_t addrlen = local_dgram_addr(&addr, job->path, false);

  unlink(job->path);
  return bind(job->fd, (FAR struct sockaddr *)&addr, addrlen);
}

static int local_dgram_do_connect(FAR struct local_dgram_job_s *job)
{
  return local_dgram_connect_to(job->fd, job->path, false);
}

static int local_dgram_do_unspec(FAR struct local_dgram_job_s *job)
{
  struct sockaddr addr;

  memset(&addr, 0, sizeof(addr));
  addr.sa_family = AF_UNSPEC;
  return connect(job->fd, &addr, sizeof(addr));
}

static int local_dgram_do_signal(FAR struct local_dgram_job_s *job)
{
  return pthread_kill(job->target, SIGUSR1);
}

static int local_dgram_do_poll(FAR struct local_dgram_job_s *job)
{
  struct pollfd pfd;
  int ret;

  pfd.fd       = job->fd;
  pfd.events   = POLLIN;
  pfd.revents  = 0;
  ret          = poll(&pfd, 1, job->arg);
  job->revents = pfd.revents;
  return ret;
}

/* Send 20 datagrams, 20 ms apart */

static int local_dgram_do_send20(FAR struct local_dgram_job_s *job)
{
  char buf[16];
  int i;

  for (i = 0; i < 20; i++)
    {
      snprintf(buf, sizeof(buf), "m%d", i);
      if (send(job->fd, buf, strlen(buf), 0) < 0)
        {
          return -1;
        }

      usleep(20000);
    }

  return 0;
}

/* Send numbered datagrams of different lengths to one receiver */

static int local_dgram_do_stress(FAR struct local_dgram_job_s *job)
{
  unsigned char buf[LOCAL_DGRAM_STRESSLEN];
  size_t len;
  int i;

  for (i = 0; i < LOCAL_DGRAM_STRESSNUM; i++)
    {
      len = 4 + i % (sizeof(buf) - 4);
      buf[0] = job->arg;
      buf[1] = i;
      buf[2] = len;
      buf[3] = 0;
      memset(buf + 4, job->arg + i, len - 4);
      if (local_dgram_sendto(job->fd, job->path, buf, len, 0) < 0)
        {
          return -1;
        }
    }

  return 0;
}

static void local_dgram_nosignal(int signo)
{
}

static long local_dgram_msec(FAR const struct timespec *t0)
{
  struct timespec t1;

  clock_gettime(CLOCK_MONOTONIC, &t1);
  return (t1.tv_sec - t0->tv_sec) * 1000 +
         (t1.tv_nsec - t0->tv_nsec) / 1000000;
}

static int local_dgram_epwait(int epfd, int ms, FAR uint32_t *events)
{
  struct epoll_event ev;
  int ret;

  memset(&ev, 0, sizeof(ev));
  ret = epoll_wait(epfd, &ev, 1, ms);
  assert_return_code(ret, errno);
  *events = ret > 0 ? ev.events : 0;
  return ret;
}

static void local_dgram_epadd(int epfd, int fd, uint32_t events)
{
  struct epoll_event ev;
  int ret;

  memset(&ev, 0, sizeof(ev));
  ev.events  = events;
  ev.data.fd = fd;
  ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_connect_empty
 *
 * Description:
 *   connect() to an address with no name fails: it must not match an unbound
 *   socket.
 *
 ****************************************************************************/

void test_local_dgram_connect_empty(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  FAR struct sockaddr *sa = (FAR struct sockaddr *)&addr;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;

  assert_int_equal(connect(st->s, sa, sizeof(sa_family_t)), -1);
  assert_int_equal(connect(st->s, sa, sizeof(sa_family_t) + 1), -1);
  assert_int_equal(connect(st->s, sa, sizeof(addr)), -1);
}

/****************************************************************************
 * Name: test_local_dgram_poll_unbound
 *
 * Description:
 *   An unbound socket can send and cannot receive.
 *
 ****************************************************************************/

void test_local_dgram_poll_unbound(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(local_dgram_revents(st->s, POLLIN, 0), 0);
  assert_int_equal(local_dgram_revents(st->s, POLLIN, 100), 0);
  assert_int_equal(local_dgram_revents(st->s, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->s, POLLIN | POLLOUT, 0), POLLOUT);
}

/****************************************************************************
 * Name: test_local_dgram_poll_bound
 *
 * Description:
 *   A bound socket with no datagram reports POLLOUT only.
 *
 ****************************************************************************/

void test_local_dgram_poll_bound(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
  assert_int_equal(local_dgram_revents(st->r1, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN | POLLOUT, 0),
                   POLLOUT);
  assert_int_equal(local_dgram_revents(st->r1, 0, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_level
 *
 * Description:
 *   POLLIN stays until the last datagram is read.
 *
 ****************************************************************************/

void test_local_dgram_poll_level(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(send(st->s, "one", 3, 0), 3);
  assert_int_equal(send(st->s, "two", 3, 0), 3);
  assert_int_equal(send(st->s, "three", 5, 0), 5);

  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);
  local_dgram_expect(st->r1, "one");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);
  local_dgram_expect(st->r1, "two");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);
  local_dgram_expect(st->r1, "three");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_trunc
 *
 * Description:
 *   A short recv() discards the rest of the datagram, and the next datagram
 *   stays whole.
 *
 ****************************************************************************/

void test_local_dgram_poll_trunc(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char big[64];
  char buf[4];

  memset(big, 'x', sizeof(big));
  assert_int_equal(send(st->s, big, sizeof(big), 0), sizeof(big));
  assert_int_equal(send(st->s, "next", 4, 0), 4);

  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), 0), sizeof(buf));
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);
  local_dgram_expect(st->r1, "next");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_peek
 *
 * Description:
 *   recv(MSG_PEEK) leaves the datagram, so POLLIN stays.
 *
 ****************************************************************************/

void test_local_dgram_poll_peek(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  assert_int_equal(send(st->s, "peek", 4, 0), 4);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), MSG_PEEK), 4);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);
  local_dgram_expect(st->r1, "peek");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_mask
 *
 * Description:
 *   poll() reports only the events that the caller asks for.
 *
 ****************************************************************************/

void test_local_dgram_poll_mask(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(send(st->s, "mask", 4, 0), 4);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  assert_int_equal(local_dgram_revents(st->r1, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->r1, 0, 0), 0);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN | POLLOUT, 0),
                   POLLIN | POLLOUT);
  assert_int_equal(local_dgram_revents(st->s, POLLIN, 0), 0);
  local_dgram_expect(st->r1, "mask");
}

/****************************************************************************
 * Name: test_local_dgram_poll_fds
 *
 * Description:
 *   poll() of an array: a negative fd is skipped, and each socket gives its
 *   own events.
 *
 ****************************************************************************/

void test_local_dgram_poll_fds(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct pollfd pfd[4];
  int ret;

  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);

  assert_int_equal(send(st->s, "fds", 3, 0), 3);
  assert_true(local_dgram_readable(st->r1, 1000));

  memset(pfd, 0, sizeof(pfd));
  pfd[0].fd     = -1;
  pfd[0].events = POLLIN;
  pfd[1].fd     = st->r1;
  pfd[1].events = POLLIN;
  pfd[2].fd     = st->s;
  pfd[2].events = POLLOUT;
  pfd[3].fd     = st->r2;
  pfd[3].events = POLLIN;

  ret = poll(pfd, 4, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 2);
  assert_int_equal(pfd[0].revents, 0);
  assert_int_equal(pfd[1].revents, POLLIN);
  assert_int_equal(pfd[2].revents, POLLOUT);
  assert_int_equal(pfd[3].revents, 0);
  local_dgram_expect(st->r1, "fds");
}

/****************************************************************************
 * Name: test_local_dgram_poll_same
 *
 * Description:
 *   One socket twice in the array of one poll().
 *
 ****************************************************************************/

void test_local_dgram_poll_same(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct pollfd pfd[2];
  int ret;

  memset(pfd, 0, sizeof(pfd));
  pfd[0].fd     = st->r1;
  pfd[0].events = POLLIN;
  pfd[1].fd     = st->r1;
  pfd[1].events = POLLIN;

  ret = poll(pfd, 2, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 0);

  assert_int_equal(send(st->s, "same", 4, 0), 4);
  assert_true(local_dgram_readable(st->r1, 1000));

  ret = poll(pfd, 2, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 2);
  assert_int_equal(pfd[0].revents, POLLIN);
  assert_int_equal(pfd[1].revents, POLLIN);
  local_dgram_expect(st->r1, "same");
}

/****************************************************************************
 * Name: test_local_dgram_poll_sendto
 *
 * Description:
 *   sendto() from a socket that is not connected wakes up poll().
 *
 ****************************************************************************/

void test_local_dgram_poll_sendto(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  int revents;

  local_dgram_job_start(&job, local_dgram_do_sendto, st->s, st->p1, 100);
  revents = local_dgram_revents(st->r1, POLLIN, 3000);
  local_dgram_job_join(&job);

  assert_int_equal(job.ret, 5);
  assert_int_equal(revents, POLLIN);
  local_dgram_expect(st->r1, "later");
}

/****************************************************************************
 * Name: test_local_dgram_poll_many
 *
 * Description:
 *   poll() and recv() in a loop take a series of datagrams in order.
 *
 ****************************************************************************/

void test_local_dgram_poll_many(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  char want[16];
  char buf[16];
  int revents = 0;
  ssize_t ret = 0;
  int bad = -1;
  int i;

  local_dgram_job_start(&job, local_dgram_do_send20, st->s, NULL, 0);
  for (i = 0; i < 20 && bad < 0; i++)
    {
      struct pollfd pfd;

      pfd.fd      = st->r1;
      pfd.events  = POLLIN;
      pfd.revents = 0;
      poll(&pfd, 1, 3000);
      revents = pfd.revents;
      if (revents != POLLIN)
        {
          bad = i;
          break;
        }

      ret = recv(st->r1, buf, sizeof(buf) - 1, 0);
      snprintf(want, sizeof(want), "m%d", i);
      if (ret != (ssize_t)strlen(want) || strncmp(buf, want, ret) != 0)
        {
          bad = i;
        }
    }

  local_dgram_job_join(&job);
  assert_int_equal(job.ret, 0);
  assert_int_equal(bad, -1);
}

/****************************************************************************
 * Name: test_local_dgram_poll_timeout
 *
 * Description:
 *   The timeout of poll(): 0 returns at once, a value waits, and -1 returns
 *   at once when a datagram waits.
 *
 ****************************************************************************/

void test_local_dgram_poll_timeout(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct timespec t0;

  clock_gettime(CLOCK_MONOTONIC, &t0);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
  assert_in_range(local_dgram_msec(&t0), 0, 150);

  clock_gettime(CLOCK_MONOTONIC, &t0);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 300), 0);
  assert_in_range(local_dgram_msec(&t0), 250, 3000);

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "t", 1, 0), 1);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, -1), POLLIN);
  local_dgram_expect(st->r1, "t");
}

/****************************************************************************
 * Name: test_local_dgram_poll_eintr
 *
 * Description:
 *   A signal ends poll() with EINTR, and the socket still works after that.
 *
 ****************************************************************************/

void test_local_dgram_poll_eintr(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  struct sigaction act;
  struct sigaction old;
  struct pollfd pfd;
  int err;
  int ret;

  memset(&act, 0, sizeof(act));
  act.sa_handler = local_dgram_nosignal;
  sigemptyset(&act.sa_mask);
  ret = sigaction(SIGUSR1, &act, &old);
  assert_return_code(ret, errno);

  pfd.fd      = st->r1;
  pfd.events  = POLLIN;
  pfd.revents = 0;

  local_dgram_job_start(&job, local_dgram_do_signal, -1, NULL, 200);
  ret = poll(&pfd, 1, 3000);
  err = errno;
  local_dgram_job_join(&job);
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(job.ret, 0);
  assert_int_equal(ret, -1);
  assert_int_equal(err, EINTR);

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "i", 1, 0), 1);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r1, "i");
}

/****************************************************************************
 * Name: test_local_dgram_ppoll
 *
 * Description:
 *   ppoll() with a timeout and a signal mask.
 *
 ****************************************************************************/

void test_local_dgram_ppoll(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct timespec ts;
  struct pollfd pfd;
  sigset_t mask;
  int ret;

  sigemptyset(&mask);
  ts.tv_sec   = 0;
  ts.tv_nsec  = 100000000;
  pfd.fd      = st->r1;
  pfd.events  = POLLIN;
  pfd.revents = 0;

  ret = ppoll(&pfd, 1, &ts, &mask);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 0);

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "p", 1, 0), 1);
  ts.tv_sec = 1;
  ret = ppoll(&pfd, 1, &ts, NULL);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 1);
  assert_int_equal(pfd.revents, POLLIN);
  local_dgram_expect(st->r1, "p");
}

/****************************************************************************
 * Name: test_local_dgram_select_rw
 *
 * Description:
 *   select() and pselect() with read, write and exception sets.
 *
 ****************************************************************************/

void test_local_dgram_select_rw(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  struct timespec ts;
  struct timeval tv;
  fd_set rset;
  fd_set wset;
  fd_set eset;
  int nfds = (st->r1 > st->s ? st->r1 : st->s) + 1;
  int ret;

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_ZERO(&eset);
  FD_SET(st->r1, &rset);
  FD_SET(st->r1, &wset);
  FD_SET(st->s, &wset);
  FD_SET(st->r1, &eset);
  tv.tv_sec  = 0;
  tv.tv_usec = 0;
  ret = select(nfds, &rset, &wset, &eset, &tv);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 2);
  assert_false(FD_ISSET(st->r1, &rset));
  assert_true(FD_ISSET(st->r1, &wset));
  assert_true(FD_ISSET(st->s, &wset));
  assert_false(FD_ISSET(st->r1, &eset));

  assert_int_equal(send(st->s, "rw", 2, 0), 2);
  assert_true(local_dgram_readable(st->r1, 1000));

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_SET(st->r1, &rset);
  FD_SET(st->s, &wset);
  ts.tv_sec  = 0;
  ts.tv_nsec = 0;
  ret = pselect(nfds, &rset, &wset, NULL, &ts, NULL);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 2);
  assert_true(FD_ISSET(st->r1, &rset));
  assert_true(FD_ISSET(st->s, &wset));
  local_dgram_expect(st->r1, "rw");

  /* A send wakes up select() */

  FD_ZERO(&rset);
  FD_SET(st->r1, &rset);
  tv.tv_sec  = 3;
  tv.tv_usec = 0;
  local_dgram_job_start(&job, local_dgram_do_send, st->s, NULL, 100);
  ret = select(st->r1 + 1, &rset, NULL, NULL, &tv);
  local_dgram_job_join(&job);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 1);
  assert_true(FD_ISSET(st->r1, &rset));
  local_dgram_expect(st->r1, "later");
}

/****************************************************************************
 * Name: test_local_dgram_epoll
 *
 * Description:
 *   epoll: level events, a wake up, EPOLLOUT and EPOLL_CTL_DEL.
 *
 ****************************************************************************/

void test_local_dgram_epoll(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  uint32_t events;
  int ret;

  st->u = epoll_create1(EPOLL_CLOEXEC);
  assert_return_code(st->u, errno);
  local_dgram_epadd(st->u, st->r1, EPOLLIN);

  assert_int_equal(local_dgram_epwait(st->u, 0, &events), 0);

  assert_int_equal(send(st->s, "ep", 2, 0), 2);
  assert_int_equal(local_dgram_epwait(st->u, 1000, &events), 1);
  assert_int_equal(events, EPOLLIN);
  assert_int_equal(local_dgram_epwait(st->u, 0, &events), 1);
  assert_int_equal(events, EPOLLIN);
  local_dgram_expect(st->r1, "ep");
  assert_int_equal(local_dgram_epwait(st->u, 0, &events), 0);

  local_dgram_job_start(&job, local_dgram_do_send, st->s, NULL, 100);
  ret = local_dgram_epwait(st->u, 3000, &events);
  local_dgram_job_join(&job);
  assert_int_equal(ret, 1);
  assert_int_equal(events, EPOLLIN);
  local_dgram_expect(st->r1, "later");

  ret = epoll_ctl(st->u, EPOLL_CTL_DEL, st->r1, NULL);
  assert_return_code(ret, errno);
  assert_int_equal(send(st->s, "ep", 2, 0), 2);
  assert_true(local_dgram_readable(st->r1, 1000));
  assert_int_equal(local_dgram_epwait(st->u, 0, &events), 0);
  local_dgram_expect(st->r1, "ep");

  local_dgram_epadd(st->u, st->s, EPOLLOUT);
  assert_int_equal(local_dgram_epwait(st->u, 1000, &events), 1);
  assert_int_equal(events, EPOLLOUT);
  ret = epoll_ctl(st->u, EPOLL_CTL_DEL, st->s, NULL);
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Name: test_local_dgram_epoll_bind
 *
 * Description:
 *   A socket joins an epoll set unbound, is bound after that, and leaves the
 *   set.  Setup saw no FIFO, and teardown sees one.
 *
 ****************************************************************************/

void test_local_dgram_epoll_bind(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  uint32_t events;
  int ret;

  st->u = epoll_create1(EPOLL_CLOEXEC);
  assert_return_code(st->u, errno);
  local_dgram_epadd(st->u, st->s, EPOLLIN);
  assert_int_equal(local_dgram_epwait(st->u, 0, &events), 0);

  local_dgram_job_start(&job, local_dgram_do_bind, st->s, st->p2, 0);
  local_dgram_job_join(&job);
  assert_int_equal(job.ret, 0);

  assert_int_equal(local_dgram_sendto(st->r1, st->p2, "b", 1, 0), 1);
  ret = local_dgram_epwait(st->u, 100, &events);
  assert_return_code(ret, errno);

  ret = epoll_ctl(st->u, EPOLL_CTL_DEL, st->s, NULL);
  assert_return_code(ret, errno);

  /* The bound socket joins the set again and reports the datagram */

  local_dgram_epadd(st->u, st->s, EPOLLIN);
  assert_int_equal(local_dgram_epwait(st->u, 1000, &events), 1);
  assert_int_equal(events, EPOLLIN);
  local_dgram_expect(st->s, "b");
  ret = epoll_ctl(st->u, EPOLL_CTL_DEL, st->s, NULL);
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Name: test_local_dgram_epoll_close
 *
 * Description:
 *   A socket is closed while it is in an epoll set.
 *
 ****************************************************************************/

void test_local_dgram_epoll_close(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  uint32_t events;
  int ret;

  st->u = epoll_create1(EPOLL_CLOEXEC);
  assert_return_code(st->u, errno);
  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);
  local_dgram_epadd(st->u, st->r2, EPOLLIN);
  assert_int_equal(local_dgram_epwait(st->u, 0, &events), 0);

  ret = close(st->r2);
  st->r2 = -1;
  assert_return_code(ret, errno);
  ret = local_dgram_epwait(st->u, 0, &events);
  assert_return_code(ret, errno);

  ret = close(st->u);
  st->u = -1;
  assert_return_code(ret, errno);

  /* The name can be bound again */

  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);
}

/****************************************************************************
 * Name: test_local_dgram_poll_bindwait
 *
 * Description:
 *   Another thread binds a socket while poll() waits on it.
 *
 ****************************************************************************/

void test_local_dgram_poll_bindwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  struct pollfd pfd;
  int err;
  int ret;

  pfd.fd      = st->s;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  local_dgram_job_start(&job, local_dgram_do_bind, st->s, st->p2, 100);
  ret = poll(&pfd, 1, 500);
  err = errno;
  local_dgram_job_join(&job);

  assert_int_equal(job.ret, 0);
  assert_return_code(ret, err);

  assert_int_equal(local_dgram_sendto(st->r1, st->p2, "bw", 2, 0), 2);
  local_dgram_expect(st->s, "bw");
}

/****************************************************************************
 * Name: test_local_dgram_poll_connwait
 *
 * Description:
 *   Another thread connects a bound socket while poll() waits on it, and
 *   poll() of the connected socket still reports datagrams from its peer.
 *
 ****************************************************************************/

void test_local_dgram_poll_connwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  struct pollfd pfd;
  int revents;
  int err;
  int ret;

  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);

  pfd.fd      = st->r2;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  local_dgram_job_start(&job, local_dgram_do_connect, st->r2, st->p1, 100);
  ret = poll(&pfd, 1, 300);
  err = errno;
  local_dgram_job_join(&job);

  assert_int_equal(job.ret, 0);
  assert_return_code(ret, err);

  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
  local_dgram_job_start(&job, local_dgram_do_sendto, st->r1, st->p2, 100);
  revents = local_dgram_revents(st->r2, POLLIN, 3000);
  local_dgram_job_join(&job);

  assert_int_equal(job.ret, 5);
  assert_int_equal(revents, POLLIN);
  local_dgram_expect(st->r2, "later");
}

/****************************************************************************
 * Name: test_local_dgram_poll_unspecwait
 *
 * Description:
 *   Another thread dissolves the peer of a socket while poll() waits on it.
 *
 ****************************************************************************/

void test_local_dgram_poll_unspecwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  struct pollfd pfd;
  int err;
  int ret;

  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);
  ret = local_dgram_connect_to(st->r2, st->p1, false);
  assert_return_code(ret, errno);

  pfd.fd      = st->r2;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  local_dgram_job_start(&job, local_dgram_do_unspec, st->r2, NULL, 100);
  ret = poll(&pfd, 1, 300);
  err = errno;
  local_dgram_job_join(&job);

  assert_int_equal(job.ret, 0);
  assert_return_code(ret, err);

  /* It has no peer now, so it takes a datagram from any socket */

  assert_int_equal(local_dgram_sendto(st->s, st->p2, "u", 1, 0), 1);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r2, "u");
}

/****************************************************************************
 * Name: test_local_dgram_poll_closepeer
 *
 * Description:
 *   The receiver of a connected sender closes.  The sender is still valid.
 *
 ****************************************************************************/

void test_local_dgram_poll_closepeer(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int revents;

  assert_int_equal(close(st->r1), 0);
  st->r1 = -1;

  revents = local_dgram_revents(st->s, POLLIN | POLLOUT, 0);
  assert_int_equal(revents & POLLNVAL, 0);
  assert_int_equal(revents & POLLIN, 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_closesend
 *
 * Description:
 *   The sender closes.  The receiver reads the datagram with no POLLHUP.
 *
 ****************************************************************************/

void test_local_dgram_poll_closesend(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(send(st->s, "bye", 3, 0), 3);
  assert_int_equal(close(st->s), 0);
  st->s = -1;

  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r1, "bye");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_threads
 *
 * Description:
 *   Two threads wait in poll() on one socket, and one datagram wakes both.
 *
 ****************************************************************************/

void test_local_dgram_poll_threads(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job[2];
  ssize_t ret;
  int i;

  for (i = 0; i < 2; i++)
    {
      local_dgram_job_arg(&job[i], local_dgram_do_poll, st->r1, NULL, 0,
                          3000);
    }

  usleep(300000);
  ret = send(st->s, "both", 4, 0);

  for (i = 0; i < 2; i++)
    {
      local_dgram_job_join(&job[i]);
    }

  assert_int_equal(ret, 4);
  for (i = 0; i < 2; i++)
    {
      assert_int_equal(job[i].ret, 1);
      assert_int_equal(job[i].revents, POLLIN);
    }

  local_dgram_expect(st->r1, "both");
}

/****************************************************************************
 * Name: test_local_dgram_poll_slots
 *
 * Description:
 *   More waiters than the FIFO may have slots for: the call may fail with
 *   EBUSY, but no slot stays taken after it.
 *
 ****************************************************************************/

void test_local_dgram_poll_slots(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  struct pollfd pfd[4];
  int revents;
  int ret;
  int i;

  for (i = 0; i < 20; i++)
    {
      int j;

      for (j = 0; j < 4; j++)
        {
          pfd[j].fd      = st->r1;
          pfd[j].events  = POLLIN;
          pfd[j].revents = 0;
        }

      errno = 0;
      ret = poll(pfd, 4, 0);
      assert_true(ret == 0 || (ret == -1 && errno == EBUSY));
    }

  local_dgram_job_start(&job, local_dgram_do_send, st->s, NULL, 100);
  revents = local_dgram_revents(st->r1, POLLIN, 3000);
  local_dgram_job_join(&job);
  assert_int_equal(revents, POLLIN);
  local_dgram_expect(st->r1, "later");
}

/****************************************************************************
 * Name: test_local_dgram_poll_cycle
 *
 * Description:
 *   Many poll() calls on one socket and many sockets on one name: nothing
 *   stays taken.
 *
 ****************************************************************************/

void test_local_dgram_poll_cycle(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int i;

  for (i = 0; i < 200; i++)
    {
      assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
      assert_int_equal(local_dgram_revents(st->s, POLLIN, 0), 0);
    }

  for (i = 0; i < 30; i++)
    {
      st->r2 = local_dgram_bind(st->p2, false);
      assert_return_code(st->r2, errno);
      assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
      assert_int_equal(local_dgram_sendto(st->s, st->p2, "c", 1, 0), 1);
      assert_int_equal(local_dgram_revents(st->r2, POLLIN, 1000), POLLIN);
      local_dgram_expect(st->r2, "c");
      assert_int_equal(close(st->r2), 0);
      st->r2 = -1;
    }
}

/****************************************************************************
 * Name: test_local_dgram_poll_big
 *
 * Description:
 *   A datagram too large for the receiver leaves it clean: no event for it,
 *   and the next datagram is whole.
 *
 ****************************************************************************/

void test_local_dgram_poll_big(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  ret = send(st->s, st->big, LOCAL_DGRAM_BIGLEN, 0);
  if (ret < 0)
    {
      assert_int_equal(errno, EMSGSIZE);
      assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
    }
  else
    {
      assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
      ret = recv(st->r1, st->big, LOCAL_DGRAM_BIGLEN, 0);
      assert_int_equal(ret, LOCAL_DGRAM_BIGLEN);
    }

  assert_int_equal(send(st->s, "next", 4, 0), 4);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r1, "next");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_abstract
 *
 * Description:
 *   poll() of a socket with an abstract name.
 *
 ****************************************************************************/

void test_local_dgram_poll_abstract(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t addrlen;
  ssize_t ret;

  st->r2 = local_dgram_bind(st->abs, true);
  assert_return_code(st->r2, errno);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);

  addrlen = local_dgram_addr(&addr, st->abs, true);
  ret = sendto(st->s, "ab", 2, 0, (FAR struct sockaddr *)&addr, addrlen);
  assert_int_equal(ret, 2);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r2, "ab");
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_self
 *
 * Description:
 *   A socket sends to its own name.
 *
 ****************************************************************************/

void test_local_dgram_poll_self(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(local_dgram_sendto(st->r1, st->p1, "me", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r1, "me");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_nonblock
 *
 * Description:
 *   poll() of an O_NONBLOCK socket still waits, and a send wakes it up.
 *
 ****************************************************************************/

void test_local_dgram_poll_nonblock(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  int revents;
  int flags;
  int ret;

  flags = fcntl(st->r1, F_GETFL);
  assert_return_code(flags, errno);
  ret = fcntl(st->r1, F_SETFL, flags | O_NONBLOCK);
  assert_return_code(ret, errno);

  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
  local_dgram_job_start(&job, local_dgram_do_sendto, st->s, st->p1, 100);
  revents = local_dgram_revents(st->r1, POLLIN, 3000);
  local_dgram_job_join(&job);
  assert_int_equal(revents, POLLIN);
  local_dgram_expect(st->r1, "later");
}

/****************************************************************************
 * Name: test_local_dgram_poll_dup
 *
 * Description:
 *   poll() of a dup() of a socket, also after the first descriptor closes.
 *
 ****************************************************************************/

void test_local_dgram_poll_dup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  st->u = dup(st->r1);
  assert_return_code(st->u, errno);

  assert_int_equal(local_dgram_revents(st->u, POLLIN, 0), 0);
  assert_int_equal(send(st->s, "d1", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->u, POLLIN, 1000), POLLIN);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);

  assert_int_equal(close(st->r1), 0);
  st->r1 = -1;
  assert_int_equal(local_dgram_revents(st->u, POLLIN, 0), POLLIN);
  local_dgram_expect(st->u, "d1");
  assert_int_equal(local_dgram_revents(st->u, POLLIN, 0), 0);

  assert_int_equal(send(st->s, "d2", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->u, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->u, "d2");
}

/****************************************************************************
 * Name: test_local_dgram_poll_sendmsg
 *
 * Description:
 *   sendmsg() with two parts makes one datagram.
 *
 ****************************************************************************/

void test_local_dgram_poll_sendmsg(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct msghdr msg;
  struct iovec iov[2];
  char part1[] = "ab";
  char part2[] = "cd";
  ssize_t ret;

  iov[0].iov_base = part1;
  iov[0].iov_len  = 2;
  iov[1].iov_base = part2;
  iov[1].iov_len  = 2;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov    = iov;
  msg.msg_iovlen = 2;

  ret = sendmsg(st->s, &msg, 0);
  assert_int_equal(ret, 4);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r1, "abcd");
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_poll_stress
 *
 * Description:
 *   Two senders on two sockets, one receiver in a poll() and recv() loop.
 *   Each datagram comes out whole, once and in the order of its sender.
 *
 ****************************************************************************/

void test_local_dgram_poll_stress(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job[2];
  unsigned char buf[LOCAL_DGRAM_STRESSLEN + 16];
  int next[2];
  int bad = 0;
  int got = 0;
  int i;

  alarm(LOCAL_DGRAM_STRESSTIME);

  st->u = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert_return_code(st->u, errno);

  next[0] = 0;
  next[1] = 0;
  local_dgram_job_arg(&job[0], local_dgram_do_stress, st->s, st->p1, 0, 0);
  local_dgram_job_arg(&job[1], local_dgram_do_stress, st->u, st->p1, 0, 1);

  while (got < 2 * LOCAL_DGRAM_STRESSNUM)
    {
      struct pollfd pfd;
      ssize_t ret;
      int who;
      int seq;
      int j;

      pfd.fd      = st->r1;
      pfd.events  = POLLIN;
      pfd.revents = 0;
      if (poll(&pfd, 1, 5000) <= 0 || pfd.revents != POLLIN)
        {
          bad++;
          break;
        }

      ret = recv(st->r1, buf, sizeof(buf), 0);
      if (ret < 4)
        {
          bad++;
          break;
        }

      got++;
      who = buf[0];
      seq = buf[1];
      if (who > 1 || seq != next[who] || ret != buf[2])
        {
          bad++;
          continue;
        }

      next[who]++;
      for (j = 4; j < ret; j++)
        {
          if (buf[j] != (unsigned char)(who + seq))
            {
              bad++;
              break;
            }
        }
    }

  for (i = 0; i < 2; i++)
    {
      local_dgram_job_join(&job[i]);
    }

  assert_int_equal(job[0].ret, 0);
  assert_int_equal(job[1].ret, 0);
  assert_int_equal(bad, 0);
  assert_int_equal(got, 2 * LOCAL_DGRAM_STRESSNUM);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
}

/****************************************************************************
 * Name: test_local_dgram_fifo_policy
 *
 * Description:
 *   A FIFO with buffer policy 1 keeps its data with no writer, so a close of
 *   the last writer gives no POLLHUP.
 *
 ****************************************************************************/

void test_local_dgram_fifo_policy(FAR void **state)
{
#ifdef PIPEIOC_POLICY
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  char path[LOCAL_DGRAM_PATHLEN];
  char buf[4];
  int revents;
  int ret;

  local_dgram_mkfifo(path, sizeof(path), st->pf);

  st->r2 = open(path, O_RDONLY | O_NONBLOCK);
  assert_return_code(st->r2, errno);
  ret = ioctl(st->r2, PIPEIOC_POLICY, 1);
  assert_return_code(ret, errno);
  st->r3 = open(path, O_WRONLY | O_NONBLOCK);
  assert_return_code(st->r3, errno);
  assert_int_equal(write(st->r3, "p", 1), 1);

  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), POLLIN);
  assert_int_equal(close(st->r3), 0);
  st->r3 = -1;
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), POLLIN);

  /* A close during the wait gives no event either */

  assert_int_equal(read(st->r2, buf, 1), 1);
  st->r3 = open(path, O_WRONLY | O_NONBLOCK);
  assert_return_code(st->r3, errno);
  local_dgram_job_start(&job, local_dgram_do_close, st->r3, NULL, 100);
  revents = local_dgram_revents(st->r2, POLLIN, 400);
  local_dgram_job_join(&job);
  st->r3 = -1;
  unlink(path);
  assert_int_equal(job.ret, 0);
  assert_int_equal(revents, 0);
#else
  skip();
#endif
}
