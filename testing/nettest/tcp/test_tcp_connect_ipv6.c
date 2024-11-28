/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp_connect_ipv6.c
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

#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>
#include <sys/time.h>

#include "test_tcp.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_LOOP_CNT    5
#define TEST_BUFFER_SIZE 80
#define TEST_MSG         "loopback message"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_connect_ipv6_setup
 ****************************************************************************/

int test_tcp_connect_ipv6_setup(FAR void **state)
{
  FAR struct nettest_tcp_state_s *tcp_state = *state;
  struct timeval tv;
  int ret;

  tcp_state->client_fd = socket(PF_INET6, SOCK_STREAM, 0);
  assert_true(tcp_state->client_fd > 0);

  tv.tv_sec  = 0;
  tv.tv_usec = 10000;
  ret = setsockopt(tcp_state->client_fd, SOL_SOCKET,
                   SO_RCVTIMEO, &tv, sizeof(tv));
  assert_return_code(ret, errno);

  ret = setsockopt(tcp_state->client_fd, SOL_SOCKET,
                   SO_SNDTIMEO, &tv, sizeof(tv));
  assert_return_code(ret, errno);

  return 0;
}

/****************************************************************************
 * Name: test_tcp_connect_ipv6
 ****************************************************************************/

void test_tcp_connect_ipv6(FAR void **state)
{
  FAR struct nettest_tcp_state_s *tcp_state = *state;
  struct sockaddr_in6 myaddr;
  char outbuf[TEST_BUFFER_SIZE];
  char inbuf[TEST_BUFFER_SIZE];
  int addrlen;
  int ret;
  int len;

  addrlen = nettest_lo_addr((FAR struct sockaddr *)&myaddr, AF_INET6);

  ret = connect(tcp_state->client_fd, (FAR struct sockaddr *)&myaddr,
                addrlen);
  assert_return_code(ret, errno);

  for (int i = 0; i < TEST_LOOP_CNT; i++)
    {
      strcpy(outbuf, TEST_MSG);
      len = strlen(outbuf);
      ret = send(tcp_state->client_fd, outbuf, len, 0);
      assert_true(ret == len);

      ret = recv(tcp_state->client_fd, inbuf, TEST_BUFFER_SIZE, 0);
      assert_true(ret == len);

      ret = strncmp(inbuf, TEST_MSG, len);
      assert_true(ret == 0);
    }
}
