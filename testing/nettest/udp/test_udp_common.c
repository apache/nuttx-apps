/****************************************************************************
 * apps/testing/nettest/udp/test_udp_common.c
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
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>
#include <sys/socket.h>

#include "test_udp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_udp_setup
 ****************************************************************************/

int test_udp_setup(FAR void **state)
{
  FAR struct test_udp_state_s *udp_state;

  udp_state = zalloc(sizeof(*udp_state));
  assert_non_null(udp_state);
  *state = udp_state;
  return 0;
}

/****************************************************************************
 * Name: test_udp_teardown
 ****************************************************************************/

int test_udp_teardown(FAR void **state)
{
  FAR struct test_udp_state_s *udp_state = *state;
  int i;

  for (i = 0; i < udp_state->nsd; i++)
    {
      close(udp_state->sd[i]);
    }

  free(udp_state);
  return 0;
}

/****************************************************************************
 * Name: test_udp_set_v6only
 ****************************************************************************/

int test_udp_set_v6only(int sd, int val)
{
  return setsockopt(sd, IPPROTO_IPV6, IPV6_V6ONLY, &val, sizeof(val));
}

/****************************************************************************
 * Name: test_udp_get_v6only
 ****************************************************************************/

int test_udp_get_v6only(int sd)
{
  socklen_t len = sizeof(int);
  int val = -1;

  if (getsockopt(sd, IPPROTO_IPV6, IPV6_V6ONLY, &val, &len) < 0)
    {
      return -errno;
    }

  return val;
}

/****************************************************************************
 * Name: test_udp_socket
 ****************************************************************************/

int test_udp_socket(FAR void **state, int domain, int type, int v6only,
                    bool reuse)
{
  FAR struct test_udp_state_s *udp_state = *state;
  int one = 1;
  int ret;
  int sd;

  assert_true(udp_state->nsd < TEST_UDP_NSOCKS);
  sd = socket(domain, type | SOCK_CLOEXEC, 0);
  assert_return_code(sd, errno);
  udp_state->sd[udp_state->nsd++] = sd;

  if (v6only >= 0)
    {
      ret = test_udp_set_v6only(sd, v6only);
      assert_return_code(ret, errno);
    }

  if (reuse)
    {
      ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
      assert_return_code(ret, errno);
    }

  return sd;
}

/****************************************************************************
 * Name: test_udp_bind
 ****************************************************************************/

int test_udp_bind(int sd, int domain, int port, FAR int *err)
{
  struct sockaddr_in6 addr6;
  struct sockaddr_in addr4;
  int ret;

  if (domain == AF_INET)
    {
      memset(&addr4, 0, sizeof(addr4));
      addr4.sin_family      = AF_INET;
      addr4.sin_port        = htons(port);
      addr4.sin_addr.s_addr = htonl(INADDR_ANY);
      ret = bind(sd, (FAR struct sockaddr *)&addr4, sizeof(addr4));
    }
  else
    {
      memset(&addr6, 0, sizeof(addr6));
      addr6.sin6_family = AF_INET6;
      addr6.sin6_port   = htons(port);
      addr6.sin6_addr   = in6addr_any;
      ret = bind(sd, (FAR struct sockaddr *)&addr6, sizeof(addr6));
    }

  *err = ret < 0 ? errno : 0;
  return ret;
}

/****************************************************************************
 * Name: test_udp_send4
 ****************************************************************************/

void test_udp_send4(int port)
{
  struct sockaddr_in addr;
  int ret;
  int sd;

  sd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  assert_return_code(sd, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ret = sendto(sd, "v4", 2, 0, (FAR struct sockaddr *)&addr, sizeof(addr));
  close(sd);
  assert_int_equal(ret, 2);
  usleep(TEST_UDP_WAIT_US);
}

/****************************************************************************
 * Name: test_udp_send6
 ****************************************************************************/

void test_udp_send6(int port)
{
  struct sockaddr_in6 addr;
  int ret;
  int sd;

  sd = socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  assert_return_code(sd, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(port);
  addr.sin6_addr   = in6addr_loopback;
  ret = sendto(sd, "v6", 2, 0, (FAR struct sockaddr *)&addr, sizeof(addr));
  close(sd);
  assert_int_equal(ret, 2);
  usleep(TEST_UDP_WAIT_US);
}

/****************************************************************************
 * Name: test_udp_recv
 ****************************************************************************/

int test_udp_recv(int sd, FAR struct sockaddr_in6 *from)
{
  socklen_t len = sizeof(*from);
  char buf[16];
  int ret;

  memset(from, 0, sizeof(*from));
  ret = recvfrom(sd, buf, sizeof(buf), MSG_DONTWAIT,
                 (FAR struct sockaddr *)from, &len);
  return ret < 0 ? -errno : ret;
}
