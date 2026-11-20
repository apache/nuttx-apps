/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_sndtimeo.c
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

/* send() with SO_SNDTIMEO on a loopback connection whose reader does not
 * read: the sends that time out in the wait for an I/O buffer must not
 * keep I/O buffers after the sockets close.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <cmocka.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_IOBINFO     "/proc/iobinfo"
#define TEST_SNDTIMEO    100   /* ms */
#define TEST_TIMEOUTS    10    /* sends that time out */
#define TEST_MAXSENDS    2000
#define TEST_SENDSIZE    4096
#define TEST_POLL        1000  /* ms, loopback answers at once */
#define TEST_RECOVER     30    /* tries of 100 ms */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_sndtimeo_state_s
{
  int fds[3];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_sndtimeo_nfree
 *
 * Description:
 *   Return the free I/O buffers (nfree of /proc/iobinfo), or -1.
 *
 ****************************************************************************/

static int tcp_sndtimeo_nfree(void)
{
  char buf[128];
  FAR char *line;
  ssize_t n;
  int nfree;
  int fd;

  fd = open(TEST_IOBINFO, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return -1;
    }

  n = read(fd, buf, sizeof(buf) - 1);
  close(fd);
  if (n <= 0)
    {
      return -1;
    }

  buf[n] = '\0';
  line = strchr(buf, '\n');
  if (line == NULL || sscanf(line + 1, "%*d %d", &nfree) != 1)
    {
      return -1;
    }

  return nfree;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_sndtimeo_setup
 ****************************************************************************/

int test_tcp_sndtimeo_setup(FAR void **state)
{
  FAR struct tcp_sndtimeo_state_s *st = malloc(sizeof(*st));

  assert_non_null(st);
  st->fds[0] = -1;
  st->fds[1] = -1;
  st->fds[2] = -1;
  *state = st;
  return 0;
}

/****************************************************************************
 * Name: test_tcp_sndtimeo_teardown
 ****************************************************************************/

int test_tcp_sndtimeo_teardown(FAR void **state)
{
  FAR struct tcp_sndtimeo_state_s *st = *state;
  int i;

  for (i = 0; i < 3; i++)
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
 * Name: test_tcp_sndtimeo_iob
 *
 * Description:
 *   The sender fills the I/O buffers that a send can take, then
 *   TEST_TIMEOUTS sends time out.  After both ends close, the free I/O
 *   buffers come back to the number before the test.  The case is
 *   skipped without /proc/iobinfo, or when a send fails with ENOMEM.
 *
 ****************************************************************************/

void test_tcp_sndtimeo_iob(FAR void **state)
{
  FAR struct tcp_sndtimeo_state_s *st = *state;
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  struct timeval tv;
  struct pollfd pfd;
  FAR char *buf;
  int timeouts = 0;
  int sndbuf;
  int before;
  int after;
  int i;

  before = tcp_sndtimeo_nfree();
  if (before < 0)
    {
      skip();
    }

  buf = malloc(TEST_SENDSIZE);
  assert_non_null(buf);
  memset(buf, 'w', TEST_SENDSIZE);

  st->fds[0] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(st->fds[0], errno);
  nettest_lo_addr((FAR struct sockaddr *)&addr, AF_INET);
  addr.sin_port = 0;
  assert_return_code(bind(st->fds[0], (FAR struct sockaddr *)&addr,
                          sizeof(addr)), errno);
  assert_return_code(listen(st->fds[0], 1), errno);
  assert_return_code(getsockname(st->fds[0], (FAR struct sockaddr *)&addr,
                                 &addrlen), errno);

  st->fds[1] = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  assert_return_code(st->fds[1], errno);
  assert_return_code(connect(st->fds[1], (FAR struct sockaddr *)&addr,
                             addrlen), errno);
  pfd.fd      = st->fds[0];
  pfd.events  = POLLIN;
  pfd.revents = 0;
  assert_int_equal(poll(&pfd, 1, TEST_POLL), 1);
  st->fds[2] = accept4(st->fds[0], NULL, NULL, SOCK_CLOEXEC);
  assert_return_code(st->fds[2], errno);

  tv.tv_sec  = 0;
  tv.tv_usec = TEST_SNDTIMEO * 1000;
  assert_return_code(setsockopt(st->fds[1], SOL_SOCKET, SO_SNDTIMEO, &tv,
                                sizeof(tv)), errno);

  /* The sends must wait for an I/O buffer, not for the send buffer.  A
   * stack without SO_SNDBUF has no send limit.
   */

  sndbuf = TEST_MAXSENDS * TEST_SENDSIZE;
  if (setsockopt(st->fds[1], SOL_SOCKET, SO_SNDBUF, &sndbuf,
                 sizeof(sndbuf)) < 0)
    {
      assert_int_equal(errno, ENOPROTOOPT);
    }

  /* The reader does not read.  The sends fill the write buffers, then
   * each send waits for an I/O buffer and times out.
   */

  for (i = 0; i < TEST_MAXSENDS && timeouts < TEST_TIMEOUTS; i++)
    {
      ssize_t ret = send(st->fds[1], buf, TEST_SENDSIZE, 0);

      if (ret < 0 && errno == ENOMEM)
        {
          /* No free callback or write buffer (connections of earlier
           * cases in TIME_WAIT can keep the callbacks): not this case.
           */

          free(buf);
          skip();
        }

      if (ret < 0)
        {
          assert_int_equal(errno, EAGAIN);
          timeouts++;
        }
    }

  free(buf);

  /* The sends did not take the I/O buffers down to the throttle (a send
   * limit or a large pool): they did not wait for an I/O buffer.
   */

  if (timeouts < TEST_TIMEOUTS ||
      tcp_sndtimeo_nfree() > CONFIG_IOB_THROTTLE)
    {
      skip();
    }

  for (i = 0; i < 3; i++)
    {
      close(st->fds[i]);
      st->fds[i] = -1;
    }

  /* The connections free their buffers when they end */

  for (i = 0; i < TEST_RECOVER; i++)
    {
      after = tcp_sndtimeo_nfree();
      if (after >= before)
        {
          break;
        }

      usleep(100 * 1000);
    }

  assert_in_range(after, before, INT_MAX);
}
