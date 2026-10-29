/****************************************************************************
 * apps/testing/nettest/mcast/test_mcast_ipv4.c
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

#define TEST_GROUP "239.1.2.3"
#define TEST_PORT  5472

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_mcast_ipv4_group
 ****************************************************************************/

static void test_mcast_ipv4_group(FAR struct group_req *gr,
                                  unsigned int ifindex)
{
  FAR struct sockaddr_in *group = (FAR struct sockaddr_in *)&gr->gr_group;

  memset(gr, 0, sizeof(*gr));
  gr->gr_interface = ifindex;
  group->sin_family = AF_INET;
  inet_pton(AF_INET, TEST_GROUP, &group->sin_addr);
}

/****************************************************************************
 * Name: test_mcast_ipv4_send
 ****************************************************************************/

static void test_mcast_ipv4_send(int fd)
{
  struct sockaddr_in addr;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(TEST_PORT);
  inet_pton(AF_INET, TEST_GROUP, &addr.sin_addr);
  test_mcast_send(fd, (FAR struct sockaddr *)&addr, sizeof(addr));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_mcast_ipv4_setup
 ****************************************************************************/

int test_mcast_ipv4_setup(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct sockaddr_in addr;
  int one = 1;
  int ret;

  mcast_state->rfd = socket(AF_INET, SOCK_DGRAM, 0);
  assert_true(mcast_state->rfd >= 0);
  mcast_state->sfd = socket(AF_INET, SOCK_DGRAM, 0);
  assert_true(mcast_state->sfd >= 0);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(TEST_PORT);
  ret = bind(mcast_state->rfd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  /* Send from 127.0.0.1 on "lo".  IP_MULTICAST_LOOP gives the datagram to
   * the local sockets.
   */

  addr.sin_port        = 0;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ret = bind(mcast_state->sfd, (FAR struct sockaddr *)&addr, sizeof(addr));
  assert_return_code(ret, errno);

  ret = setsockopt(mcast_state->sfd, IPPROTO_IP, IP_MULTICAST_LOOP,
                   &one, sizeof(one));
  assert_return_code(ret, errno);

  return 0;
}

/****************************************************************************
 * Name: test_mcast_join_ipv4
 *
 * Description:
 *   MCAST_JOIN_GROUP joins the group: a datagram to the group arrives.
 *
 ****************************************************************************/

void test_mcast_join_ipv4(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_req gr;
  int ret;

  test_mcast_ipv4_group(&gr, mcast_state->ifindex);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);

  test_mcast_ipv4_send(mcast_state->sfd);
  assert_true(test_mcast_recv(mcast_state->rfd));

  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_LEAVE_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);
}

/****************************************************************************
 * Name: test_mcast_leave_ipv4
 *
 * Description:
 *   After MCAST_LEAVE_GROUP no datagram to the group arrives.  A second
 *   leave fails as a second IP_DROP_MEMBERSHIP.
 *
 ****************************************************************************/

void test_mcast_leave_ipv4(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct ip_mreq mreq;
  struct group_req gr;
  int err;
  int ret;

  test_mcast_ipv4_group(&gr, mcast_state->ifindex);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_LEAVE_GROUP,
                   &gr, sizeof(gr));
  assert_return_code(ret, errno);

  test_mcast_ipv4_send(mcast_state->sfd);
  assert_false(test_mcast_recv(mcast_state->rfd));

  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_LEAVE_GROUP,
                   &gr, sizeof(gr));
  assert_int_equal(ret, -1);
  err = errno;

  inet_pton(AF_INET, TEST_GROUP, &mreq.imr_multiaddr);
  mreq.imr_interface.s_addr = htonl(INADDR_LOOPBACK);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   &mreq, sizeof(mreq));
  assert_return_code(ret, errno);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   &mreq, sizeof(mreq));
  assert_int_equal(ret, -1);
  assert_int_equal(errno, err);
}

/****************************************************************************
 * Name: test_mcast_optlen_ipv4
 ****************************************************************************/

void test_mcast_optlen_ipv4(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_req gr;
  int ret;

  test_mcast_ipv4_group(&gr, mcast_state->ifindex);
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr) - 1);
  assert_int_equal(ret, -1);
  assert_int_equal(errno, EINVAL);
}

/****************************************************************************
 * Name: test_mcast_family_ipv4
 ****************************************************************************/

void test_mcast_family_ipv4(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_req gr;
  int ret;

  test_mcast_ipv4_group(&gr, mcast_state->ifindex);
  gr.gr_group.ss_family = AF_INET6;
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_JOIN_GROUP,
                   &gr, sizeof(gr));
  assert_int_equal(ret, -1);
  assert_int_equal(errno, EINVAL);
}

/****************************************************************************
 * Name: test_mcast_source_ipv4
 *
 * Description:
 *   IGMPv2 has no source filter: MCAST_JOIN_SOURCE_GROUP fails.
 *
 ****************************************************************************/

void test_mcast_source_ipv4(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;
  struct group_source_req gsr;
  int ret;

  memset(&gsr, 0, sizeof(gsr));
  gsr.gsr_interface = mcast_state->ifindex;
  gsr.gsr_group.ss_family  = AF_INET;
  gsr.gsr_source.ss_family = AF_INET;
  ret = setsockopt(mcast_state->rfd, IPPROTO_IP, MCAST_JOIN_SOURCE_GROUP,
                   &gsr, sizeof(gsr));
  assert_int_equal(ret, -1);
  assert_int_equal(errno, ENOSYS);
}
