/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_pair.c
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

/* socketpair(AF_UNIX, SOCK_DGRAM): send() and recv() of whole datagrams in
 * both directions.  The ends are r2 and r3.  Also sendmsg() of a datagram
 * in many parts, to a pair and to a bound socket.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOCAL_DGRAM_PAIRBUF 16
#define LOCAL_DGRAM_NIOV    12
#define LOCAL_DGRAM_IOVLEN  500
#define LOCAL_DGRAM_WHOLE   400

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Make the pair r2, r3 */

static void local_dgram_pair(FAR struct nettest_local_dgram_state_s *st)
{
  int sv[2];
  int ret;

  ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
  assert_return_code(ret, errno);
  st->r2 = sv[0];
  st->r3 = sv[1];
}

/* recv() one datagram without a wait into size bytes, and check its length
 * and data.
 */

static void local_dgram_pairrecv(int fd, FAR const char *want, size_t size)
{
  char buf[LOCAL_DGRAM_PAIRBUF];
  ssize_t ret;

  memset(buf, 0, sizeof(buf));
  ret = recv(fd, buf, size, MSG_DONTWAIT);
  assert_int_equal(ret, strlen(want));
  assert_memory_equal(buf, want, ret);
}

/* sendmsg() of LOCAL_DGRAM_IOVLEN bytes in LOCAL_DGRAM_NIOV parts, two of
 * them empty: to path, or to the peer if path is NULL.  Then recv() it on
 * rfd as one datagram.
 */

static void local_dgram_iov(int fd, FAR const char *path, int rfd)
{
  unsigned char data[LOCAL_DGRAM_IOVLEN];
  unsigned char buf[LOCAL_DGRAM_IOVLEN + 16];
  struct iovec iov[LOCAL_DGRAM_NIOV];
  struct sockaddr_un addr;
  struct msghdr msg;
  size_t pos = 0;
  ssize_t ret;
  int i;

  for (i = 0; i < sizeof(data); i++)
    {
      data[i] = i % 251;
    }

  for (i = 0; i < LOCAL_DGRAM_NIOV; i++)
    {
      iov[i].iov_base = NULL;
      iov[i].iov_len  = 0;
      if (i != 3 && i != 7)
        {
          iov[i].iov_base = data + pos;
          iov[i].iov_len  = LOCAL_DGRAM_IOVLEN / (LOCAL_DGRAM_NIOV - 2);
          pos += iov[i].iov_len;
        }
    }

  memset(&msg, 0, sizeof(msg));
  if (path != NULL)
    {
      msg.msg_name    = &addr;
      msg.msg_namelen = local_dgram_addr(&addr, path, false);
    }

  msg.msg_iov    = iov;
  msg.msg_iovlen = LOCAL_DGRAM_NIOV;
  ret = sendmsg(fd, &msg, 0);
  assert_int_equal(ret, sizeof(data));

  assert_true(local_dgram_readable(rfd, 1000));
  ret = recv(rfd, buf, sizeof(buf), 0);
  assert_int_equal(ret, sizeof(data));
  assert_memory_equal(buf, data, sizeof(data));
}

/* Send one datagram of LOCAL_DGRAM_WHOLE bytes and wait for space */

static int local_dgram_do_sendwhole(FAR struct local_dgram_job_s *job)
{
  char buf[LOCAL_DGRAM_WHOLE];

  memset(buf, 'w', sizeof(buf));
  return send(job->fd, buf, sizeof(buf), 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_pair_send
 *
 * Description:
 *   send() on a datagram socket pair keeps the boundaries of the datagrams,
 *   also of an empty one, in both directions.  A short buffer gets the
 *   start of a datagram and the rest is lost.
 *
 ****************************************************************************/

void test_local_dgram_pair_send(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  local_dgram_pair(st);

  assert_int_equal(send(st->r2, "one", 3, 0), 3);
  assert_int_equal(send(st->r2, "", 0, 0), 0);
  assert_int_equal(send(st->r2, "three", 5, 0), 5);
  assert_int_equal(send(st->r2, "abcdef", 6, 0), 6);
  assert_int_equal(send(st->r2, "x", 1, 0), 1);

  local_dgram_pairrecv(st->r3, "one", LOCAL_DGRAM_PAIRBUF);
  local_dgram_pairrecv(st->r3, "", LOCAL_DGRAM_PAIRBUF);
  local_dgram_pairrecv(st->r3, "three", LOCAL_DGRAM_PAIRBUF);
  local_dgram_pairrecv(st->r3, "abc", 3);
  local_dgram_pairrecv(st->r3, "x", LOCAL_DGRAM_PAIRBUF);

  errno = 0;
  assert_int_equal(recv(st->r3, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);

  assert_int_equal(write(st->r3, "back", 4), 4);
  local_dgram_pairrecv(st->r2, "back", LOCAL_DGRAM_PAIRBUF);
}

/****************************************************************************
 * Name: test_local_dgram_pair_full
 *
 * Description:
 *   A send that must not wait fails with EAGAIN when the peer is full, and
 *   leaves no part of a datagram: each datagram comes out whole and in
 *   order.  poll() gives POLLOUT only when the peer has space again.
 *
 ****************************************************************************/

void test_local_dgram_pair_full(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[LOCAL_DGRAM_PAIRBUF];
  int count;

  local_dgram_pair(st);

  count = local_dgram_fill(st->r2, NULL);
  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), 0);
  local_dgram_unfill(st->r3, count);

  errno = 0;
  assert_int_equal(recv(st->r3, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), POLLOUT);

  assert_int_equal(send(st->r2, "next", 4, MSG_DONTWAIT), 4);
  local_dgram_pairrecv(st->r3, "next", LOCAL_DGRAM_PAIRBUF);
}

/****************************************************************************
 * Name: test_local_dgram_pair_close
 *
 * Description:
 *   A datagram sent before the peer closes can still be read.  A send to a
 *   closed peer fails with ECONNREFUSED.
 *
 ****************************************************************************/

void test_local_dgram_pair_close(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_pair(st);

  assert_int_equal(send(st->r2, "last", 4, 0), 4);
  assert_int_equal(close(st->r2), 0);
  st->r2 = -1;
  local_dgram_pairrecv(st->r3, "last", LOCAL_DGRAM_PAIRBUF);

  errno = 0;
  assert_int_equal(send(st->r3, "gone", 4, 0), -1);
  assert_int_equal(errno, ECONNREFUSED);
}

/****************************************************************************
 * Name: test_local_dgram_poll_pair
 *
 * Description:
 *   poll() of a socketpair(SOCK_DGRAM).
 *
 ****************************************************************************/

void test_local_dgram_poll_pair(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_pair(st);

  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->r3, POLLOUT, 0), POLLOUT);
  assert_int_equal(local_dgram_revents(st->r3, POLLIN, 0), 0);

  assert_int_equal(send(st->r2, "pa", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->r3, POLLIN, 1000), POLLIN);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
  local_dgram_expect(st->r3, "pa");
  assert_int_equal(local_dgram_revents(st->r3, POLLIN, 0), 0);

  assert_int_equal(send(st->r3, "ir", 2, 0), 2);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 1000), POLLIN);
  local_dgram_expect(st->r2, "ir");
}

/****************************************************************************
 * Name: test_local_dgram_pair_iov
 *
 * Description:
 *   sendmsg() of one datagram in 12 parts, two of them empty, on a
 *   datagram socket pair, in both directions.
 *
 ****************************************************************************/

void test_local_dgram_pair_iov(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_pair(st);
  local_dgram_iov(st->r2, NULL, st->r3);
  local_dgram_iov(st->r3, NULL, st->r2);
}

/****************************************************************************
 * Name: test_local_dgram_send_iov
 *
 * Description:
 *   sendmsg() of one datagram in 12 parts, two of them empty, to a bound
 *   socket.
 *
 ****************************************************************************/

void test_local_dgram_send_iov(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  local_dgram_iov(st->s, st->p1, st->r1);
}

/****************************************************************************
 * Name: test_local_dgram_pair_whole
 *
 * Description:
 *   A reader that does not wait never gets the start of a datagram without
 *   the rest, also when the send of that datagram waits for space.  The
 *   reader reads the full peer with no pause, while the send waits in
 *   another thread.
 *
 ****************************************************************************/

void test_local_dgram_pair_whole(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  unsigned char buf[LOCAL_DGRAM_WHOLE + 16];
  ssize_t ret;
  int flags;
  int count;
  int tries = 0;
  int got = 0;
  int i;

  local_dgram_pair(st);
  flags = fcntl(st->r3, F_GETFL);
  assert_return_code(flags, errno);
  assert_return_code(fcntl(st->r3, F_SETFL, flags | O_NONBLOCK), errno);

  count = local_dgram_fill(st->r2, NULL);
  local_dgram_job_start(&job, local_dgram_do_sendwhole, st->r2, NULL, 0);
  usleep(200000);

  for (; ; )
    {
      ret = recv(st->r3, buf, sizeof(buf), 0);
      if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) &&
          tries++ < 100)
        {
          usleep(10000);
          continue;
        }

      if (ret != LOCAL_DGRAM_FILLLEN || got == count ||
          (buf[0] | buf[1] << 8) != got)
        {
          break;
        }

      got++;
    }

  /* End the send if it still waits */

  close(st->r3);
  st->r3 = -1;
  local_dgram_job_join(&job);

  assert_int_equal(got, count);
  assert_int_equal(ret, LOCAL_DGRAM_WHOLE);
  assert_int_equal(job.ret, LOCAL_DGRAM_WHOLE);
  for (i = 0; i < LOCAL_DGRAM_WHOLE; i++)
    {
      assert_int_equal(buf[i], 'w');
    }
}
