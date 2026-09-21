/****************************************************************************
 * apps/testing/nettest/timestamp/test_timestamp.h
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

#ifndef __APPS_TESTING_NETTEST_TIMESTAMP_TEST_TIMESTAMP_H
#define __APPS_TESTING_NETTEST_TIMESTAMP_TEST_TIMESTAMP_H

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct nettest_timestamp_state_s
{
  unsigned int ifindex;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_timestamp_group_setup
 ****************************************************************************/

int test_timestamp_group_setup(FAR void **state);

/****************************************************************************
 * Name: test_timestamp_group_teardown
 ****************************************************************************/

int test_timestamp_group_teardown(FAR void **state);

/****************************************************************************
 * Name: test_timestamp_setsockopt
 ****************************************************************************/

void test_timestamp_setsockopt(FAR void **state);

/****************************************************************************
 * Name: test_timestamp_compat
 ****************************************************************************/

void test_timestamp_compat(FAR void **state);

/****************************************************************************
 * Name: test_timestamp_tx
 ****************************************************************************/

void test_timestamp_tx(FAR void **state);

/****************************************************************************
 * Name: test_timestamp_rx
 ****************************************************************************/

void test_timestamp_rx(FAR void **state);

/****************************************************************************
 * Name: test_timestamp_poll
 ****************************************************************************/

void test_timestamp_poll(FAR void **state);

#endif /* __APPS_TESTING_NETTEST_TIMESTAMP_TEST_TIMESTAMP_H */
