/****************************************************************************
 * apps/testing/nettest/can/test_can.h
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

#ifndef __APPS_TESTING_NETTEST_CAN_TEST_CAN_H
#define __APPS_TESTING_NETTEST_CAN_TEST_CAN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_can_bind_*
 *
 * Description:
 *   bind() of a CAN_RAW socket: only a CAN device, or index 0 (all CAN
 *   devices), can be bound.  An unbound socket cannot send.
 *
 ****************************************************************************/

void test_can_bind_nodev(FAR void **state);
void test_can_bind_any(FAR void **state);
void test_can_bind_can0(FAR void **state);
void test_can_bind_todev(FAR void **state);
void test_can_bind_sendto(FAR void **state);
void test_can_bind_lo(FAR void **state);

#endif /* __APPS_TESTING_NETTEST_CAN_TEST_CAN_H */
