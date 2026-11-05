/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_regress.c
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

/* What works without connect() and poll() of a datagram socket: bind(),
 * sendto() and recv() of datagram sockets, and POLLHUP of pipes, FIFOs and
 * stream sockets.  The datagram cases in this file call neither connect()
 * nor poll().
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOCAL_DGRAM_RECLEN 32
#define LOCAL_DGRAM_RECNUM 50

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Wait for one datagram with recv() only */

static void local_dgram_recvstr(int fd, FAR const char *want)
{
  char buf[128];
  ssize_t n;

  n = recv(fd, buf, sizeof(buf) - 1, 0);
  assert_return_code(n, errno);
  buf[n] = '\0';
  assert_string_equal(buf, want);
}

/* recv(MSG_DONTWAIT) must find nothing */

static void local_dgram_recvnone(int fd)
{
  char buf[16];

  errno = 0;
  assert_int_equal(recv(fd, buf, sizeof(buf), MSG_DONTWAIT), -1);
  assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
}

#ifdef CONFIG_NET_LOCAL_STREAM
static int local_dgram_do_streamconn(FAR struct local_dgram_job_s *job)
{
  struct sockaddr_un addr;
  socklen_t addrlen = local_dgram_addr(&addr, job->path, false);

  return connect(job->fd, (FAR struct sockaddr *)&addr, addrlen);
}

/* Send records of one letter on a stream socket */

static int local_dgram_do_records(FAR struct local_dgram_job_s *job)
{
  char buf[LOCAL_DGRAM_RECLEN];
  int i;

  memset(buf, job->arg, sizeof(buf));
  for (i = 0; i < LOCAL_DGRAM_RECNUM; i++)
    {
      if (send(job->fd, buf, sizeof(buf), 0) != sizeof(buf))
        {
          return -1;
        }
    }

  return 0;
}

/* Make a connected pair of stream sockets: u, the client, and r2, the
 * accepted socket.  r3 is the listener.
 */

static void
local_dgram_streampair(FAR struct nettest_local_dgram_state_s *st)
{
  struct local_dgram_job_s job;
  struct sockaddr_un addr;
  socklen_t addrlen;
  int ret;

  addrlen = local_dgram_addr(&addr, st->p3, false);
  unlink(st->p3);

  st->r3 = socket(AF_UNIX, SOCK_STREAM, 0);
  assert_return_code(st->r3, errno);
  ret = bind(st->r3, (FAR struct sockaddr *)&addr, addrlen);
  assert_return_code(ret, errno);
  ret = listen(st->r3, 2);
  assert_return_code(ret, errno);

  st->u = socket(AF_UNIX, SOCK_STREAM, 0);
  assert_return_code(st->u, errno);

  local_dgram_job_start(&job, local_dgram_do_streamconn, st->u, st->p3, 0);
  st->r2 = accept(st->r3, NULL, NULL);
  local_dgram_job_join(&job);
  assert_return_code(st->r2, errno);
  assert_int_equal(job.ret, 0);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_reg_inuse
 *
 * Description:
 *   bind() to a name that is in use fails with EADDRINUSE.
 *
 ****************************************************************************/

void test_local_dgram_reg_inuse(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t addrlen = local_dgram_addr(&addr, st->p1, false);

  errno = 0;
  assert_int_equal(bind(st->s, (FAR struct sockaddr *)&addr, addrlen), -1);
  assert_int_equal(errno, EADDRINUSE);

  /* The socket is not changed by the error */

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "free", 4, 0), 4);
  local_dgram_recvstr(st->r1, "free");
}

/****************************************************************************
 * Name: test_local_dgram_reg_names
 *
 * Description:
 *   getsockname() of a bound socket gives its name, and of an unbound socket
 *   gives no name.
 *
 ****************************************************************************/

void test_local_dgram_reg_names(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t addrlen;
  int ret;

  memset(&addr, 0, sizeof(addr));
  addrlen = sizeof(addr);
  ret = getsockname(st->r1, (FAR struct sockaddr *)&addr, &addrlen);
  assert_return_code(ret, errno);
  assert_int_equal(addr.sun_family, AF_UNIX);
  assert_string_equal(addr.sun_path, st->p1);

  memset(&addr, 0, sizeof(addr));
  addrlen = sizeof(addr);
  ret = getsockname(st->s, (FAR struct sockaddr *)&addr, &addrlen);
  assert_return_code(ret, errno);
  assert_int_equal(addr.sun_family, AF_UNIX);
  assert_true(addrlen <= sizeof(sa_family_t) || addr.sun_path[0] == '\0');
}

/****************************************************************************
 * Name: test_local_dgram_reg_sendrecv
 *
 * Description:
 *   sendto() and recv(), with the datagram first and with recv() first.
 *
 ****************************************************************************/

void test_local_dgram_reg_sendrecv(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "first", 5, 0), 5);
  local_dgram_recvstr(st->r1, "first");

  local_dgram_job_start(&job, local_dgram_do_sendto, st->s, st->p1, 100);
  local_dgram_recvstr(st->r1, "later");
  local_dgram_job_join(&job);
  assert_int_equal(job.ret, 5);
}

/****************************************************************************
 * Name: test_local_dgram_reg_source
 *
 * Description:
 *   recvfrom() gives no name for an unbound sender.
 *
 ****************************************************************************/

void test_local_dgram_reg_source(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t addrlen;
  char buf[16];
  ssize_t ret;

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "a", 1, 0), 1);
  memset(&addr, 0, sizeof(addr));
  addrlen = sizeof(addr);
  ret = recvfrom(st->r1, buf, sizeof(buf), 0, (FAR struct sockaddr *)&addr,
                 &addrlen);
  assert_int_equal(ret, 1);
  assert_true(addrlen <= sizeof(sa_family_t) || addr.sun_path[0] == '\0');
}

/****************************************************************************
 * Name: test_local_dgram_reg_order
 *
 * Description:
 *   Datagrams keep their order and their limits.
 *
 ****************************************************************************/

void test_local_dgram_reg_order(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "one", 3, 0), 3);
  assert_int_equal(local_dgram_sendto(st->s, st->p1, "two", 3, 0), 3);
  assert_int_equal(local_dgram_sendto(st->s, st->p1, "three", 5, 0), 5);
  local_dgram_recvstr(st->r1, "one");
  local_dgram_recvstr(st->r1, "two");
  local_dgram_recvstr(st->r1, "three");
  local_dgram_recvnone(st->r1);
}

/****************************************************************************
 * Name: test_local_dgram_reg_trunc
 *
 * Description:
 *   A short recv() discards the rest of the datagram.
 *
 ****************************************************************************/

void test_local_dgram_reg_trunc(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char big[64];
  char buf[4];

  memset(big, 'x', sizeof(big));
  assert_int_equal(local_dgram_sendto(st->s, st->p1, big, sizeof(big), 0),
                   sizeof(big));
  assert_int_equal(local_dgram_sendto(st->s, st->p1, "next", 4, 0), 4);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), 0), sizeof(buf));
  local_dgram_recvstr(st->r1, "next");
  local_dgram_recvnone(st->r1);
}

/****************************************************************************
 * Name: test_local_dgram_reg_zero
 *
 * Description:
 *   A datagram with no data: recv() returns 0.
 *
 ****************************************************************************/

void test_local_dgram_reg_zero(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "", 0, 0), 0);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), 0), 0);
  assert_int_equal(local_dgram_sendto(st->s, st->p1, "next", 4, 0), 4);
  local_dgram_recvstr(st->r1, "next");
}

/****************************************************************************
 * Name: test_local_dgram_reg_big
 *
 * Description:
 *   A datagram too large for the receiver leaves the next datagram whole.
 *
 ****************************************************************************/

void test_local_dgram_reg_big(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  ret = local_dgram_sendto(st->s, st->p1, st->big, LOCAL_DGRAM_BIGLEN, 0);
  if (ret < 0)
    {
      assert_int_equal(errno, EMSGSIZE);
    }
  else
    {
      ret = recv(st->r1, st->big, LOCAL_DGRAM_BIGLEN, 0);
      assert_int_equal(ret, LOCAL_DGRAM_BIGLEN);
    }

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "next", 4, 0), 4);
  local_dgram_recvstr(st->r1, "next");
  local_dgram_recvnone(st->r1);
}

/****************************************************************************
 * Name: test_local_dgram_reg_dontwait
 *
 * Description:
 *   recv(MSG_DONTWAIT) takes a datagram that waits.
 *
 ****************************************************************************/

void test_local_dgram_reg_dontwait(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  local_dgram_recvnone(st->r1);
  assert_int_equal(local_dgram_sendto(st->s, st->p1, "dw", 2, 0), 2);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), MSG_DONTWAIT), 2);
  local_dgram_recvnone(st->r1);
}

/****************************************************************************
 * Name: test_local_dgram_reg_peek
 *
 * Description:
 *   recv(MSG_PEEK) leaves the datagram.
 *
 ****************************************************************************/

void test_local_dgram_reg_peek(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  assert_int_equal(local_dgram_sendto(st->s, st->p1, "peek", 4, 0), 4);
  assert_int_equal(recv(st->r1, buf, sizeof(buf), MSG_PEEK), 4);
  local_dgram_recvstr(st->r1, "peek");
  local_dgram_recvnone(st->r1);
}

/****************************************************************************
 * Name: test_local_dgram_reg_missing
 *
 * Description:
 *   sendto() of a name with no socket fails, and send() with no address
 *   fails.
 *
 ****************************************************************************/

void test_local_dgram_reg_missing(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  errno = 0;
  assert_int_equal(local_dgram_sendto(st->s, st->p2, "x", 1, 0), -1);
  assert_int_equal(errno, ENOENT);

  errno = 0;
  assert_int_equal(send(st->s, "x", 1, 0), -1);
  assert_true(errno == EDESTADDRREQ || errno == ENOTCONN);
}

/****************************************************************************
 * Name: test_local_dgram_reg_unbound
 *
 * Description:
 *   recv(MSG_DONTWAIT) of an unbound socket returns an error and does not
 *   wait.
 *
 ****************************************************************************/

void test_local_dgram_reg_unbound(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  char buf[16];

  assert_int_equal(recv(st->s, buf, sizeof(buf), MSG_DONTWAIT), -1);
}

/****************************************************************************
 * Name: test_local_dgram_reg_self
 *
 * Description:
 *   A socket sends to its own name.
 *
 ****************************************************************************/

void test_local_dgram_reg_self(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;

  assert_int_equal(local_dgram_sendto(st->r1, st->p1, "me", 2, 0), 2);
  local_dgram_recvstr(st->r1, "me");
}

/****************************************************************************
 * Name: test_local_dgram_reg_msg
 *
 * Description:
 *   sendmsg() with two parts makes one datagram.
 *
 ****************************************************************************/

void test_local_dgram_reg_msg(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  struct msghdr msg;
  struct iovec iov[2];
  char part1[] = "ab";
  char part2[] = "cd";
  char buf[8];
  ssize_t ret;

  iov[0].iov_base = part1;
  iov[0].iov_len  = 2;
  iov[1].iov_base = part2;
  iov[1].iov_len  = 2;
  memset(&msg, 0, sizeof(msg));
  msg.msg_name    = &addr;
  msg.msg_namelen = local_dgram_addr(&addr, st->p1, false);
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 2;
  ret = sendmsg(st->s, &msg, 0);
  assert_int_equal(ret, 4);

  memset(buf, 0, sizeof(buf));
  iov[0].iov_base = buf;
  iov[0].iov_len  = sizeof(buf) - 1;
  msg.msg_name    = NULL;
  msg.msg_namelen = 0;
  msg.msg_iovlen  = 1;
  ret = recvmsg(st->r1, &msg, 0);
  assert_int_equal(ret, 4);
  assert_string_equal(buf, "abcd");
}

/****************************************************************************
 * Name: test_local_dgram_reg_cycle
 *
 * Description:
 *   Many sockets on one name: nothing stays taken.
 *
 ****************************************************************************/

void test_local_dgram_reg_cycle(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int i;

  for (i = 0; i < 30; i++)
    {
      st->r2 = local_dgram_bind(st->p2, false);
      assert_return_code(st->r2, errno);
      assert_int_equal(local_dgram_sendto(st->s, st->p2, "c", 1, 0), 1);
      local_dgram_recvstr(st->r2, "c");
      assert_int_equal(close(st->r2), 0);
      st->r2 = -1;
    }
}

/****************************************************************************
 * Name: test_local_dgram_stream_names
 *
 * Description:
 *   An unbound stream client has no name, on either end of the connection.
 *
 ****************************************************************************/

void test_local_dgram_stream_names(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t addrlen;
  int ret;

  local_dgram_streampair(st);

  memset(&addr, 0, sizeof(addr));
  addrlen = sizeof(addr);
  ret = getsockname(st->u, (FAR struct sockaddr *)&addr, &addrlen);
  assert_return_code(ret, errno);
  assert_true(addrlen <= sizeof(sa_family_t) || addr.sun_path[0] == '\0');

  memset(&addr, 0, sizeof(addr));
  addrlen = sizeof(addr);
  ret = getpeername(st->r2, (FAR struct sockaddr *)&addr, &addrlen);
  assert_return_code(ret, errno);
  assert_true(addrlen <= sizeof(sa_family_t) || addr.sun_path[0] == '\0');

  memset(&addr, 0, sizeof(addr));
  addrlen = sizeof(addr);
  ret = getpeername(st->u, (FAR struct sockaddr *)&addr, &addrlen);
  assert_return_code(ret, errno);
  assert_string_equal(addr.sun_path, st->p3);
#else
  skip();
#endif
}

/****************************************************************************
 * Name: test_local_dgram_stream_unspec
 *
 * Description:
 *   connect(AF_UNSPEC) of a stream socket fails.
 *
 ****************************************************************************/

void test_local_dgram_stream_unspec(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr addr;

  st->u = socket(AF_UNIX, SOCK_STREAM, 0);
  assert_return_code(st->u, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sa_family = AF_UNSPEC;
  assert_int_equal(connect(st->u, &addr, sizeof(addr)), -1);
#else
  skip();
#endif
}

/****************************************************************************
 * Name: test_local_dgram_stream_threads
 *
 * Description:
 *   Two threads send() on one stream socket: each short record stays whole.
 *
 ****************************************************************************/

void test_local_dgram_stream_threads(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job[2];
  char buf[LOCAL_DGRAM_RECLEN];
  int total = 2 * LOCAL_DGRAM_RECNUM;
  int bad = 0;
  int got;
  int i;

  local_dgram_streampair(st);

  local_dgram_job_arg(&job[0], local_dgram_do_records, st->u, NULL, 0, 'a');
  local_dgram_job_arg(&job[1], local_dgram_do_records, st->u, NULL, 0, 'b');

  for (got = 0; got < total; got++)
    {
      size_t len = 0;

      while (len < sizeof(buf))
        {
          ssize_t ret = recv(st->r2, buf + len, sizeof(buf) - len, 0);

          if (ret <= 0)
            {
              bad++;
              break;
            }

          len += ret;
        }

      if (len < sizeof(buf))
        {
          break;
        }

      for (i = 1; i < LOCAL_DGRAM_RECLEN; i++)
        {
          if (buf[i] != buf[0])
            {
              bad++;
              break;
            }
        }
    }

  local_dgram_job_join(&job[0]);
  local_dgram_job_join(&job[1]);
  assert_int_equal(job[0].ret, 0);
  assert_int_equal(job[1].ret, 0);
  assert_int_equal(bad, 0);
  assert_int_equal(got, total);
#else
  skip();
#endif
}

/****************************************************************************
 * Name: test_local_dgram_stream_hup
 *
 * Description:
 *   A stream socket gives POLLHUP when its peer closes: the data first, then
 *   recv() returns 0.
 *
 ****************************************************************************/

void test_local_dgram_stream_hup(FAR void **state)
{
#ifdef CONFIG_NET_LOCAL_STREAM
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  char buf[4];
  int revents;

  local_dgram_streampair(st);

  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
  assert_int_equal(local_dgram_revents(st->r2, POLLOUT, 0), POLLOUT);

  assert_int_equal(send(st->u, "s", 1, 0), 1);
  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 1000), POLLIN);
  assert_int_equal(recv(st->r2, buf, sizeof(buf), 0), 1);

  local_dgram_job_start(&job, local_dgram_do_close, st->u, NULL, 100);
  revents = local_dgram_revents(st->r2, POLLIN, 3000);
  local_dgram_job_join(&job);
  st->u = -1;
  assert_int_equal(job.ret, 0);
  assert_int_not_equal(revents & POLLHUP, 0);
  assert_int_equal(recv(st->r2, buf, sizeof(buf), 0), 0);
#else
  skip();
#endif
}

/****************************************************************************
 * Name: test_local_dgram_pipe_hup
 *
 * Description:
 *   A pipe gives POLLHUP when the last writer closes and POLLERR when the
 *   last reader closes.
 *
 ****************************************************************************/

void test_local_dgram_pipe_hup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  char buf[4];
  int fd[2];
  int revents;
  int ret;

  /* The data comes first, and POLLHUP after it */

  ret = pipe(fd);
  assert_return_code(ret, errno);
  st->r2 = fd[0];
  st->r3 = fd[1];

  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);
  assert_int_equal(write(st->r3, "x", 1), 1);
  assert_int_not_equal(local_dgram_revents(st->r2, POLLIN, 0) & POLLIN, 0);
  assert_int_equal(close(st->r3), 0);
  st->r3 = -1;
  assert_int_not_equal(local_dgram_revents(st->r2, POLLIN, 0) & POLLIN, 0);
  assert_int_equal(read(st->r2, buf, sizeof(buf)), 1);
  assert_int_not_equal(local_dgram_revents(st->r2, POLLIN, 0) & POLLHUP, 0);
  assert_int_equal(close(st->r2), 0);
  st->r2 = -1;

  /* A close of the last writer wakes up poll() */

  ret = pipe(fd);
  assert_return_code(ret, errno);
  st->r2 = fd[0];
  st->r3 = fd[1];

  local_dgram_job_start(&job, local_dgram_do_close, st->r3, NULL, 100);
  revents = local_dgram_revents(st->r2, POLLIN, 3000);
  local_dgram_job_join(&job);
  st->r3 = -1;
  assert_int_equal(job.ret, 0);
  assert_int_not_equal(revents & POLLHUP, 0);
  assert_int_equal(close(st->r2), 0);
  st->r2 = -1;

  /* The writer sees POLLERR when the last reader is gone */

  ret = pipe(fd);
  assert_return_code(ret, errno);
  st->r2 = fd[0];
  st->r3 = fd[1];

  assert_int_equal(close(st->r2), 0);
  st->r2 = -1;
  revents = local_dgram_revents(st->r3, POLLOUT, 0);
  assert_int_not_equal(revents & POLLERR, 0);
}

/****************************************************************************
 * Name: test_local_dgram_fifo_hup
 *
 * Description:
 *   A named FIFO gives POLLHUP when its last writer closes.
 *
 ****************************************************************************/

void test_local_dgram_fifo_hup(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct local_dgram_job_s job;
  char path[LOCAL_DGRAM_PATHLEN];
  int revents;

  local_dgram_mkfifo(path, sizeof(path), st->pf);

  st->r2 = open(path, O_RDONLY | O_NONBLOCK);
  assert_return_code(st->r2, errno);
  st->r3 = open(path, O_WRONLY | O_NONBLOCK);
  assert_return_code(st->r3, errno);

  assert_int_equal(local_dgram_revents(st->r2, POLLIN, 0), 0);

  local_dgram_job_start(&job, local_dgram_do_close, st->r3, NULL, 100);
  revents = local_dgram_revents(st->r2, POLLIN, 3000);
  local_dgram_job_join(&job);
  st->r3 = -1;
  unlink(path);
  assert_int_equal(job.ret, 0);
  assert_int_not_equal(revents & POLLHUP, 0);
}
