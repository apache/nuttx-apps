/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_wait.c
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

/* A send() or recv() of a local socket that waits: for space in a full
 * receiver, and when a signal comes.  A signal ends the call with EINTR:
 * another thread sends SIGUSR1 (a handler without SA_RESTART) after 200 ms.
 * If the signal does not end the call, a later action ends the wait, so
 * that the case fails and does not hang.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void local_dgram_intr_handler(int signo)
{
}

static int local_dgram_do_intr(FAR struct local_dgram_job_s *job)
{
  return pthread_kill(job->target, SIGUSR1);
}

/* Install the handler of SIGUSR1, without SA_RESTART */

static void local_dgram_intr_setup(FAR struct sigaction *old)
{
  struct sigaction act;
  int ret;

  memset(&act, 0, sizeof(act));
  act.sa_handler = local_dgram_intr_handler;
  sigemptyset(&act.sa_mask);
  ret = sigaction(SIGUSR1, &act, old);
  assert_return_code(ret, errno);
}

/* Read all that is in the socket without a wait.  Return the count. */

static int local_dgram_drainall(int fd)
{
  char buf[LOCAL_DGRAM_FILLLEN];
  int count = 0;

  while (recv(fd, buf, sizeof(buf), MSG_DONTWAIT) >= 0)
    {
      count++;
    }

  return count;
}

/* Send one datagram of LOCAL_DGRAM_FILLLEN bytes and wait for space: to
 * path, or to the peer if path is NULL.
 */

static int local_dgram_do_sendfull(FAR struct local_dgram_job_s *job)
{
  char buf[LOCAL_DGRAM_FILLLEN];

  memset(buf, 'l', sizeof(buf));
  if (job->path != NULL)
    {
      return local_dgram_sendto(job->fd, job->path, buf, sizeof(buf), 0);
    }

  return send(job->fd, buf, sizeof(buf), 0);
}

/* A thread sends to a full receiver, this thread sends it SIGUSR1 after
 * 200 ms and reads the receiver empty 1.3 s later.  The send waits in the
 * thread: a signal handler of a flat build runs on the stack of the thread
 * that waits, and that stack has no cmocka frames.  Return the count that
 * was read; job gets the result of the send.
 */

static int local_dgram_sendintr(FAR struct local_dgram_job_s *job, int fd,
                                FAR const char *path, int rfd)
{
  struct sigaction old;
  int count;

  local_dgram_intr_setup(&old);
  local_dgram_job_start(job, local_dgram_do_sendfull, fd, path, 0);
  usleep(200000);
  pthread_kill(job->thread, SIGUSR1);
  usleep(1300000);
  count = local_dgram_drainall(rfd);
  local_dgram_job_join(job);
  sigaction(SIGUSR1, &old, NULL);
  return count;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_poll_full
 *
 * Description:
 *   A non-blocking send to a full receiver fails with EAGAIN and leaves no
 *   part of a datagram: each datagram it took comes out whole.
 *
 ****************************************************************************/

void test_local_dgram_poll_full(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int count;

  count = local_dgram_fill(st->s, st->p1);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), POLLIN);

  local_dgram_unfill(st->r1, count);
  assert_int_equal(local_dgram_revents(st->r1, POLLIN, 0), 0);
  assert_int_equal(local_dgram_revents(st->s, POLLOUT, 0), POLLOUT);

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "next", 4, 0), 4);
  local_dgram_expect(st->r1, "next");
}

/****************************************************************************
 * Name: test_local_dgram_poll_fullwait
 *
 * Description:
 *   A blocking send to a full receiver ends when the receiver reads.
 *
 ****************************************************************************/

void test_local_dgram_poll_fullwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  int count;

  count = local_dgram_fill(st->s, st->p1);

  local_dgram_job_start(&job, local_dgram_do_sendto, st->s, st->p1, 0);
  usleep(200000);
  local_dgram_unfill(st->r1, count);
  local_dgram_expect(st->r1, "later");
  local_dgram_job_join(&job);
  assert_int_equal(job.ret, 5);
}

/****************************************************************************
 * Name: test_local_dgram_recv_intr
 *
 * Description:
 *   A signal ends recv() of a bound datagram socket that waits for a
 *   datagram.  The datagram that comes later is not lost.
 *
 ****************************************************************************/

void test_local_dgram_recv_intr(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s intr;
  struct local_dgram_job_s later;
  struct sigaction old;
  char buf[16];
  ssize_t ret;
  int err;

  local_dgram_intr_setup(&old);
  local_dgram_job_start(&intr, local_dgram_do_intr, -1, NULL, 200);
  local_dgram_job_start(&later, local_dgram_do_sendto, st->s, st->p1, 1500);
  ret = recv(st->r1, buf, sizeof(buf), 0);
  err = errno;
  local_dgram_job_join(&intr);
  local_dgram_job_join(&later);
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(ret, -1);
  assert_int_equal(err, EINTR);
  assert_int_equal(later.ret, 5);
  local_dgram_expect(st->r1, "later");
}

/****************************************************************************
 * Name: test_local_dgram_send_intr
 *
 * Description:
 *   A signal ends sendto() to a full bound datagram socket.  The receiver
 *   gets no part of that datagram.
 *
 ****************************************************************************/

void test_local_dgram_send_intr(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  int count;

  local_dgram_fill(st->s, st->p1);
  count = local_dgram_sendintr(&job, st->s, st->p1, st->r1);

  assert_int_equal(job.ret, -1);
  assert_int_equal(job.err, EINTR);
  assert_true(count > 0);

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "next", 4, 0), 4);
  local_dgram_expect(st->r1, "next");
}

/****************************************************************************
 * Name: test_local_dgram_pair_intr
 *
 * Description:
 *   A signal ends send() to the full peer of a datagram socket pair.
 *
 ****************************************************************************/

void test_local_dgram_pair_intr(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  int count;
  int sv[2];
  int ret;

  ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
  assert_return_code(ret, errno);
  st->r2 = sv[0];
  st->r3 = sv[1];
  local_dgram_fill(st->r2, NULL);
  count = local_dgram_sendintr(&job, st->r2, NULL, st->r3);

  assert_int_equal(job.ret, -1);
  assert_int_equal(job.err, EINTR);
  assert_true(count > 0);

  assert_int_equal(send(st->r2, "next", 4, 0), 4);
  local_dgram_expect(st->r3, "next");
}

/****************************************************************************
 * Name: test_local_dgram_stream_intr
 *
 * Description:
 *   A signal ends recv() of a stream socket that waits for data.
 *
 ****************************************************************************/

void test_local_dgram_stream_intr(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s intr;
  struct local_dgram_job_s later;
  struct sigaction old;
  char buf[16];
  ssize_t ret;
  int sv[2];
  int err;

  ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert_return_code(ret, errno);
  st->r2 = sv[0];
  st->r3 = sv[1];

  local_dgram_intr_setup(&old);
  local_dgram_job_start(&intr, local_dgram_do_intr, -1, NULL, 200);
  local_dgram_job_start(&later, local_dgram_do_send, st->r3, NULL, 1500);
  ret = recv(st->r2, buf, sizeof(buf), 0);
  err = errno;
  local_dgram_job_join(&intr);
  local_dgram_job_join(&later);
  sigaction(SIGUSR1, &old, NULL);

  assert_int_equal(ret, -1);
  assert_int_equal(err, EINTR);
  assert_int_equal(later.ret, 5);
  assert_int_equal(recv(st->r2, buf, sizeof(buf), 0), 5);
#else
  skip();
#endif
}

/****************************************************************************
 * Name: test_local_dgram_stream_send_intr
 *
 * Description:
 *   A signal ends send() to a full stream socket.
 *
 ****************************************************************************/

void test_local_dgram_stream_send_intr(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  char buf[LOCAL_DGRAM_FILLLEN];
  ssize_t ret;
  int flags;
  int count;
  int sv[2];

  ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
  assert_return_code(ret, errno);
  st->r2 = sv[0];
  st->r3 = sv[1];

  /* Fill the peer with a socket that does not wait */

  flags = fcntl(st->r2, F_GETFL);
  assert_return_code(flags, errno);
  assert_return_code(fcntl(st->r2, F_SETFL, flags | O_NONBLOCK), errno);
  memset(buf, 's', sizeof(buf));
  do
    {
      ret = send(st->r2, buf, sizeof(buf), 0);
    }
  while (ret > 0);

  assert_int_equal(ret, -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
  assert_return_code(fcntl(st->r2, F_SETFL, flags), errno);

  count = local_dgram_sendintr(&job, st->r2, NULL, st->r3);

  assert_int_equal(job.ret, -1);
  assert_int_equal(job.err, EINTR);
  assert_true(count > 0);
#else
  skip();
#endif
}
