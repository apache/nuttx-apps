/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp.h
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

#ifndef __APPS_TESTING_NETTEST_TCP_TEST_TCP_H
#define __APPS_TESTING_NETTEST_TCP_TEST_TCP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct nettest_tcp_state_s
{
  int       client_fd;
  pthread_t server_tid;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_tcp_group_setup
 ****************************************************************************/

int test_tcp_group_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_group_teardown
 ****************************************************************************/

int test_tcp_group_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_common_teardown
 ****************************************************************************/

int test_tcp_common_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tcp_connect_ipv4_setup
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
int test_tcp_connect_ipv4_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_connect_ipv4
 ****************************************************************************/

void test_tcp_connect_ipv4(FAR void **state);
#endif

/****************************************************************************
 * Name: test_tcp_connect_ipv6_setup
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
int test_tcp_connect_ipv6_setup(FAR void **state);

/****************************************************************************
 * Name: test_tcp_connect_ipv6
 ****************************************************************************/

void test_tcp_connect_ipv6(FAR void **state);
#endif
#endif /* __APPS_TESTING_NETTEST_TCP_TEST_TCP_H */
