/****************************************************************************
 * apps/testing/nettest/local/test_local_dgram_connect.c
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
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_local_dgram.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_dgram_connect
 *
 * Description:
 *   connect() to a bound datagram socket returns 0.
 *
 ****************************************************************************/

void test_local_dgram_connect(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int ret;

  ret = local_dgram_connect_to(st->s, st->p1, false);
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Name: test_local_dgram_peername
 *
 * Description:
 *   getpeername() gives the name of the receiver.
 *
 ****************************************************************************/

void test_local_dgram_peername(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t len = sizeof(addr);
  int ret;

  memset(&addr, 0, sizeof(addr));
  ret = getpeername(st->s, (FAR struct sockaddr *)&addr, &len);
  assert_return_code(ret, errno);
  assert_int_equal(addr.sun_family, AF_UNIX);
  assert_string_equal(addr.sun_path, st->p1);
}

/****************************************************************************
 * Name: test_local_dgram_send
 *
 * Description:
 *   send() without an address reaches the receiver.
 *
 ****************************************************************************/

void test_local_dgram_send(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  ret = send(st->s, "one", 3, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 3);
  local_dgram_expect(st->r1, "one");
}

/****************************************************************************
 * Name: test_local_dgram_write
 *
 * Description:
 *   write() reaches the receiver.
 *
 ****************************************************************************/

void test_local_dgram_write(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  ret = write(st->s, "two", 3);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 3);
  local_dgram_expect(st->r1, "two");
}

/****************************************************************************
 * Name: test_local_dgram_sendto_null
 *
 * Description:
 *   sendto() with a NULL address reaches the receiver.
 *
 ****************************************************************************/

void test_local_dgram_sendto_null(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  ret = sendto(st->s, "three", 5, 0, NULL, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 5);
  local_dgram_expect(st->r1, "three");
}

/****************************************************************************
 * Name: test_local_dgram_peeronly
 *
 * Description:
 *   A connected socket takes datagrams from its peer only.
 *
 ****************************************************************************/

void test_local_dgram_peeronly(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  socklen_t len;
  ssize_t ret;

  /* r2 at p2 is connected to r3 at p3; u is a stranger */

  st->r2 = local_dgram_bind(st->p2, false);
  assert_return_code(st->r2, errno);
  st->r3 = local_dgram_bind(st->p3, false);
  assert_return_code(st->r3, errno);
  st->u = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert_return_code(st->u, errno);
  ret = local_dgram_connect_to(st->r2, st->p3, false);
  assert_return_code(ret, errno);

  /* The sendto() of the stranger can fail (EPERM) or not, but the
   * datagram must not arrive.
   */

  len = local_dgram_addr(&addr, st->p2, false);
  sendto(st->u, "stranger", 8, 0, (FAR struct sockaddr *)&addr, len);
  ret = sendto(st->r3, "peer", 4, 0, (FAR struct sockaddr *)&addr, len);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 4);
  local_dgram_expect(st->r2, "peer");
  assert_false(local_dgram_readable(st->r2, 50));
}

/****************************************************************************
 * Name: test_local_dgram_unspec
 *
 * Description:
 *   connect(AF_UNSPEC) removes the destination: send() fails.
 *
 ****************************************************************************/

void test_local_dgram_unspec(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  ssize_t ret;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNSPEC;
  ret = connect(st->s, (FAR struct sockaddr *)&addr, sizeof(sa_family_t));
  assert_return_code(ret, errno);

  errno = 0;
  ret = send(st->s, "x", 1, 0);
  assert_int_equal(ret, -1);
  assert_true(errno == EDESTADDRREQ || errno == ENOTCONN);
}

/****************************************************************************
 * Name: test_local_dgram_reconnect
 *
 * Description:
 *   connect() to a second receiver: send() goes there.
 *
 ****************************************************************************/

void test_local_dgram_reconnect(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  st->r3 = local_dgram_bind(st->p3, false);
  assert_return_code(st->r3, errno);
  ret = local_dgram_connect_to(st->s, st->p3, false);
  assert_return_code(ret, errno);

  ret = send(st->s, "seven", 5, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 5);
  local_dgram_expect(st->r3, "seven");
  assert_false(local_dgram_readable(st->r1, 50));
}

/****************************************************************************
 * Name: test_local_dgram_missing
 *
 * Description:
 *   connect() to a name that does not exist fails with ENOENT.
 *
 ****************************************************************************/

void test_local_dgram_missing(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int ret;

  errno = 0;
  ret = local_dgram_connect_to(st->s, st->pf, false);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, ENOENT);
}

/****************************************************************************
 * Name: test_local_dgram_notsock
 *
 * Description:
 *   connect() to a regular file fails with ECONNREFUSED.
 *
 ****************************************************************************/

void test_local_dgram_notsock(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  int ret;

  ret = open(st->pf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (ret < 0)
    {
      skip();
    }

  close(ret);
  errno = 0;
  ret = local_dgram_connect_to(st->s, st->pf, false);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, ECONNREFUSED);
}

/****************************************************************************
 * Name: test_local_dgram_closed
 *
 * Description:
 *   send() after the receiver is closed fails with ECONNREFUSED.
 *
 ****************************************************************************/

void test_local_dgram_closed(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  close(st->r1);
  st->r1 = -1;
  unlink(st->p1);

  errno = 0;
  ret = send(st->s, "x", 1, 0);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, ECONNREFUSED);
}

/****************************************************************************
 * Name: test_local_dgram_abstract
 *
 * Description:
 *   connect() and send() to an abstract name.
 *
 ****************************************************************************/

void test_local_dgram_abstract(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  ssize_t ret;

  st->r2 = local_dgram_bind(st->abs, true);
  if (st->r2 < 0)
    {
      skip();
    }

  ret = local_dgram_connect_to(st->s, st->abs, true);
  assert_return_code(ret, errno);
  ret = send(st->s, "eight", 5, 0);
  assert_return_code(ret, errno);
  assert_int_equal(ret, 5);
  local_dgram_expect(st->r2, "eight");
}

/****************************************************************************
 * Name: test_local_dgram_addrlen
 *
 * Description:
 *   bind(), connect() and sendto() use addrlen bytes of the address.  No
 *   NUL follows the name, and other bytes come after it.
 *
 ****************************************************************************/

void test_local_dgram_addrlen(FAR void **state)
{
  FAR struct nettest_local_dgram_state_s *st = *state;
  struct sockaddr_un addr;
  struct sockaddr_un name;
  socklen_t addrlen;
  socklen_t namelen;
  size_t len;
  int ret;

  /* A pathname with no NUL in addrlen */

  len = strlen(st->p2);
  memset(&addr, 'x', sizeof(addr));
  addr.sun_family = AF_UNIX;
  memcpy(addr.sun_path, st->p2, len);
  addrlen = offsetof(struct sockaddr_un, sun_path) + len;

  unlink(st->p2);
  st->r2 = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert_return_code(st->r2, errno);
  ret = bind(st->r2, (FAR struct sockaddr *)&addr, addrlen);
  assert_return_code(ret, errno);

  memset(&name, 0, sizeof(name));
  namelen = sizeof(name);
  ret = getsockname(st->r2, (FAR struct sockaddr *)&name, &namelen);
  assert_return_code(ret, errno);
  assert_string_equal(name.sun_path, st->p2);

  ret = sendto(st->s, "path", 4, 0, (FAR struct sockaddr *)&addr, addrlen);
  assert_int_equal(ret, 4);
  local_dgram_expect(st->r2, "path");

  ret = connect(st->s, (FAR struct sockaddr *)&addr, addrlen);
  assert_return_code(ret, errno);
  assert_int_equal(send(st->s, "conn", 4, 0), 4);
  local_dgram_expect(st->r2, "conn");

  /* An abstract name: it ends at addrlen */

  len = strlen(st->abs);
  memset(&addr, 'x', sizeof(addr));
  addr.sun_family = AF_UNIX;
  addr.sun_path[0] = '\0';
  memcpy(addr.sun_path + 1, st->abs, len);
  addrlen = offsetof(struct sockaddr_un, sun_path) + 1 + len;

  st->r3 = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert_return_code(st->r3, errno);
  ret = bind(st->r3, (FAR struct sockaddr *)&addr, addrlen);
  assert_return_code(ret, errno);

  ret = sendto(st->r1, "abs", 3, 0, (FAR struct sockaddr *)&addr, addrlen);
  assert_int_equal(ret, 3);
  local_dgram_expect(st->r3, "abs");
}
