/****************************************************************************
 * apps/testing/nettest/mcast/test_mcast.h
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

#ifndef __APPS_TESTING_NETTEST_MCAST_TEST_MCAST_H
#define __APPS_TESTING_NETTEST_MCAST_TEST_MCAST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <sys/socket.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct nettest_mcast_state_s
{
  unsigned int ifindex;  /* Index of "lo" */
  int          rfd;      /* Receiver, bound to the group port */
  int          sfd;      /* Sender, bound to the loopback address */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_mcast_group_setup
 ****************************************************************************/

int test_mcast_group_setup(FAR void **state);

/****************************************************************************
 * Name: test_mcast_group_teardown
 ****************************************************************************/

int test_mcast_group_teardown(FAR void **state);

/****************************************************************************
 * Name: test_mcast_common_teardown
 ****************************************************************************/

int test_mcast_common_teardown(FAR void **state);

/****************************************************************************
 * Name: test_mcast_send
 ****************************************************************************/

void test_mcast_send(int fd, FAR const struct sockaddr *addr,
                     socklen_t addrlen);

/****************************************************************************
 * Name: test_mcast_recv
 *
 * Description:
 *   Return true if the test message arrives on fd within one second.
 *
 ****************************************************************************/

bool test_mcast_recv(int fd);

/****************************************************************************
 * Name: test_mcast_names
 ****************************************************************************/

void test_mcast_names(FAR void **state);

#ifdef CONFIG_NET_IGMP

/****************************************************************************
 * Name: test_mcast_ipv4_setup
 ****************************************************************************/

int test_mcast_ipv4_setup(FAR void **state);

/****************************************************************************
 * Name: test_mcast_join_ipv4
 ****************************************************************************/

void test_mcast_join_ipv4(FAR void **state);

/****************************************************************************
 * Name: test_mcast_leave_ipv4
 ****************************************************************************/

void test_mcast_leave_ipv4(FAR void **state);

/****************************************************************************
 * Name: test_mcast_optlen_ipv4
 ****************************************************************************/

void test_mcast_optlen_ipv4(FAR void **state);

/****************************************************************************
 * Name: test_mcast_family_ipv4
 ****************************************************************************/

void test_mcast_family_ipv4(FAR void **state);

/****************************************************************************
 * Name: test_mcast_source_ipv4
 ****************************************************************************/

void test_mcast_source_ipv4(FAR void **state);
#endif

#ifdef CONFIG_NET_MLD

/****************************************************************************
 * Name: test_mcast_ipv6_setup
 ****************************************************************************/

int test_mcast_ipv6_setup(FAR void **state);

/****************************************************************************
 * Name: test_mcast_join_ipv6
 ****************************************************************************/

void test_mcast_join_ipv6(FAR void **state);

/****************************************************************************
 * Name: test_mcast_leave_ipv6
 ****************************************************************************/

void test_mcast_leave_ipv6(FAR void **state);

/****************************************************************************
 * Name: test_mcast_optlen_ipv6
 ****************************************************************************/

void test_mcast_optlen_ipv6(FAR void **state);

/****************************************************************************
 * Name: test_mcast_family_ipv6
 ****************************************************************************/

void test_mcast_family_ipv6(FAR void **state);

/****************************************************************************
 * Name: test_mcast_source_ipv6
 ****************************************************************************/

void test_mcast_source_ipv6(FAR void **state);
#endif

#endif /* __APPS_TESTING_NETTEST_MCAST_TEST_MCAST_H */
