/****************************************************************************
 * apps/testing/nettest/tun/test_tun.h
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

#ifndef __APPS_TESTING_NETTEST_TUN_TEST_TUN_H
#define __APPS_TESTING_NETTEST_TUN_TEST_TUN_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_tun_tcp_setup
 ****************************************************************************/

int test_tun_tcp_setup(FAR void **state);

/****************************************************************************
 * Name: test_tun_tcp_client_setup
 ****************************************************************************/

int test_tun_tcp_client_setup(FAR void **state);

/****************************************************************************
 * Name: test_tun_tcp_teardown
 ****************************************************************************/

int test_tun_tcp_teardown(FAR void **state);

/****************************************************************************
 * Name: test_tun_tcp_connect_again
 ****************************************************************************/

void test_tun_tcp_connect_again(FAR void **state);

/****************************************************************************
 * Name: test_tun_tcp_connect_intr
 ****************************************************************************/

void test_tun_tcp_connect_intr(FAR void **state);

/****************************************************************************
 * Name: test_tun_tcp_lossy
 ****************************************************************************/

void test_tun_tcp_lossy(FAR void **state);

/****************************************************************************
 * Name: test_tun_tcp_partack
 ****************************************************************************/

void test_tun_tcp_partack(FAR void **state);

#endif /* __APPS_TESTING_NETTEST_TUN_TEST_TUN_H */
