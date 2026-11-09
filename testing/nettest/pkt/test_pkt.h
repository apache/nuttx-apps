/****************************************************************************
 * apps/testing/nettest/pkt/test_pkt.h
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

#ifndef __APPS_TESTING_NETTEST_PKT_TEST_PKT_H
#define __APPS_TESTING_NETTEST_PKT_TEST_PKT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: test_pkt_lo_*
 *
 * Description:
 *   Packet sockets bound to the loopback device: a frame sent on lo comes
 *   back, and the reassembly timeout of an IPv4 fragment sent on lo does
 *   not stop the kernel.
 *
 ****************************************************************************/

void test_pkt_lo_send(FAR void **state);
void test_pkt_lo_frag(FAR void **state);

#endif /* __APPS_TESTING_NETTEST_PKT_TEST_PKT_H */
