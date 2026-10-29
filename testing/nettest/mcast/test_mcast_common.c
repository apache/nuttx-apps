/****************************************************************************
 * apps/testing/nettest/mcast/test_mcast_common.c
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

#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cmocka.h>

#include "test_mcast.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_MSG      "multicast message"
#define TEST_TIMEOUT  1000  /* Milliseconds */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_mcast_group_setup
 ****************************************************************************/

int test_mcast_group_setup(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state;

  mcast_state = zalloc(sizeof(*mcast_state));
  assert_non_null(mcast_state);

  mcast_state->ifindex = if_nametoindex("lo");
  mcast_state->rfd     = -1;
  mcast_state->sfd     = -1;
  assert_int_not_equal(mcast_state->ifindex, 0);

  *state = mcast_state;
  return 0;
}

/****************************************************************************
 * Name: test_mcast_group_teardown
 ****************************************************************************/

int test_mcast_group_teardown(FAR void **state)
{
  free(*state);
  return 0;
}

/****************************************************************************
 * Name: test_mcast_common_teardown
 ****************************************************************************/

int test_mcast_common_teardown(FAR void **state)
{
  FAR struct nettest_mcast_state_s *mcast_state = *state;

  if (mcast_state->rfd >= 0)
    {
      close(mcast_state->rfd);
      mcast_state->rfd = -1;
    }

  if (mcast_state->sfd >= 0)
    {
      close(mcast_state->sfd);
      mcast_state->sfd = -1;
    }

  return 0;
}

/****************************************************************************
 * Name: test_mcast_send
 ****************************************************************************/

void test_mcast_send(int fd, FAR const struct sockaddr *addr,
                     socklen_t addrlen)
{
  ssize_t ret;

  ret = sendto(fd, TEST_MSG, strlen(TEST_MSG), 0, addr, addrlen);
  assert_int_equal(ret, strlen(TEST_MSG));
}

/****************************************************************************
 * Name: test_mcast_recv
 ****************************************************************************/

bool test_mcast_recv(int fd)
{
  struct pollfd pfd;
  char buf[32];
  ssize_t ret;

  pfd.fd     = fd;
  pfd.events = POLLIN;
  if (poll(&pfd, 1, TEST_TIMEOUT) != 1)
    {
      return false;
    }

  ret = recv(fd, buf, sizeof(buf), 0);
  return ret == (ssize_t)strlen(TEST_MSG) &&
         memcmp(buf, TEST_MSG, ret) == 0;
}

/****************************************************************************
 * Name: test_mcast_names
 *
 * Description:
 *   The MCAST_* options work at the IPPROTO_IP and the IPPROTO_IPV6 level,
 *   so their values must differ from all IP_* and IPV6_* options.
 *
 ****************************************************************************/

void test_mcast_names(FAR void **state)
{
  static const int mcast[] =
  {
    MCAST_JOIN_GROUP, MCAST_BLOCK_SOURCE, MCAST_UNBLOCK_SOURCE,
    MCAST_LEAVE_GROUP, MCAST_JOIN_SOURCE_GROUP, MCAST_LEAVE_SOURCE_GROUP
  };

  static const int other[] =
  {
    IP_MULTICAST_IF, IP_MULTICAST_TTL, IP_MULTICAST_LOOP,
    IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP, IP_UNBLOCK_SOURCE,
    IP_BLOCK_SOURCE, IP_ADD_SOURCE_MEMBERSHIP, IP_DROP_SOURCE_MEMBERSHIP,
    IP_MSFILTER, IP_MULTICAST_ALL, IP_PKTINFO, IP_TOS, IP_TTL,
    IPV6_JOIN_GROUP, IPV6_LEAVE_GROUP, IPV6_MULTICAST_HOPS,
    IPV6_MULTICAST_IF, IPV6_MULTICAST_LOOP, IPV6_UNICAST_HOPS,
    IPV6_V6ONLY, IPV6_PKTINFO, IPV6_RECVPKTINFO, IPV6_TCLASS,
    IPV6_RECVHOPLIMIT, IPV6_HOPLIMIT
  };

  struct group_source_req gsr;
  struct group_req gr;
  size_t i;
  size_t j;

  assert_true(sizeof(gr.gr_group) == sizeof(struct sockaddr_storage));
  assert_true(sizeof(gsr.gsr_group) == sizeof(struct sockaddr_storage));
  assert_true(sizeof(gsr.gsr_source) == sizeof(struct sockaddr_storage));

  for (i = 0; i < nitems(mcast); i++)
    {
      for (j = i + 1; j < nitems(mcast); j++)
        {
          assert_int_not_equal(mcast[i], mcast[j]);
        }

      for (j = 0; j < nitems(other); j++)
        {
          assert_int_not_equal(mcast[i], other[j]);
        }
    }
}
