/****************************************************************************
 * apps/testing/nettest/mcast/test_mcast_ipv6.c
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

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_mcast.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_GROUP "ff02::123"
#define TEST_PORT  5473

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_mcast_ipv6_group
 ****************************************************************************/

static void test_mcast_ipv6_group(FAR struct group_req *gr,
                                  unsigned int ifindex)
{
  FAR struct sockaddr_in6 *group = (FAR struct sockaddr_in6 *)&gr->gr_group;

  memset(gr, 0, sizeof(*gr));
  gr->gr_interface = ifindex;
  group->sin6_family = AF_INET6;
  inet_pton(AF_INET6, TEST_GROUP, &group->sin6_addr);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_mcast_ipv6_setup
 ****************************************************************************/

int test_mcast_ipv6_setup(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct sockaddr_in6 addr;
  int one = 1;
  int ret;

  mcast_state->rfd = socket(AF_INET6, SOCK_DGRAM, 0);
  assert_true(mcast_state->rfd >= 0);
  mcast_state->sfd = socket(AF_INET6, SOCK_DGRAM, 0);
  assert_true(mcast_state->sfd >= 0);

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(TEST_PORT);
  ret = bind(mcast_state->rfd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  /* Send from ::1 on "lo".  IPV6_MULTICAST_LOOP gives the datagram to the
   * local sockets.
   */

  addr.sin6_port = 0;
  addr.sin6_addr = in6addr_loopback;
  ret = bind(mcast_state->sfd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  ret = setsockopt(mcast_state->sfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                   &one, sizeof(one));
  assert_return_code(ret, errno);

  return 0;
}

/****************************************************************************
 * Name: test_mcast_join_ipv6
 *
 * Description:
 *   MCAST_JOIN_GROUP and MCAST_LEAVE_GROUP succeed, and a datagram to the
 *   group arrives.
 *
 ****************************************************************************/

void test_mcast_join_ipv6(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct sockaddr_in6 addr;
  struct group_req gr;
  int ret;

  test_mcast_ipv6_group(&gr, mcast_state->ifindex);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port   = htons(TEST_PORT);
  inet_pton(AF_INET6, TEST_GROUP, &addr.sin6_addr);
  test_mcast_send(mcast_state->sfd, (FAR struct sockaddr *)&addr,
                  sizeof(addr));
  assert_true(test_mcast_recv(mcast_state->rfd));

  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_LEAVE_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Name: test_mcast_leave_ipv6
 *
 * Description:
 *   A second MCAST_LEAVE_GROUP fails as a second IPV6_LEAVE_GROUP.
 *
 ****************************************************************************/

void test_mcast_leave_ipv6(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct ipv6_mreq mreq;
  struct group_req gr;
  int err;
  int ret;

  test_mcast_ipv6_group(&gr, mcast_state->ifindex);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_LEAVE_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_LEAVE_GROUP,
                   &gr, sizeof(gr));
  assert_int_equal(ret, -1);
  err = errno;

  inet_pton(AF_INET6, TEST_GROUP, &mreq.ipv6mr_multiaddr);
  mreq.ipv6mr_interface = mcast_state->ifindex;
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                   &mreq, sizeof(mreq));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
                   &mreq, sizeof(mreq));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
                   &mreq, sizeof(mreq));
  assert_int_equal(ret, -1);
  assert_int_equal(errno, err);
}

/****************************************************************************
 * Name: test_mcast_optlen_ipv6
 ****************************************************************************/

void test_mcast_optlen_ipv6(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_req gr;
  int ret;

  test_mcast_ipv6_group(&gr, mcast_state->ifindex);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr) - 1);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, EINVAL);
}

/****************************************************************************
 * Name: test_mcast_family_ipv6
 ****************************************************************************/

void test_mcast_family_ipv6(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_req gr;
  int ret;

  test_mcast_ipv6_group(&gr, mcast_state->ifindex);
  gr.gr_group.ss_family = AF_INET;
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr));
  assert_int_equal(ret, -1);
  assert_int_equal(errno, EADDRNOTAVAIL);
}

/****************************************************************************
 * Name: test_mcast_source_ipv6
 *
 * Description:
 *   MLD has no source filter: MCAST_JOIN_SOURCE_GROUP fails.
 *
 ****************************************************************************/

void test_mcast_source_ipv6(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_source_req gsr;
  int ret;

  memset(&gsr, 0, sizeof(gsr));
  gsr.gsr_interface = mcast_state->ifindex;
  gsr.gsr_group.ss_family  = AF_INET6;
  gsr.gsr_source.ss_family = AF_INET6;
  ret = setsockopt(mcast_state->rfd, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP,
                   &gsr, sizeof(gsr));
  assert_int_equal(ret, -1);
  assert_int_equal(errno, ENOSYS);
}
