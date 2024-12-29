/****************************************************************************
 * apps/testing/testsuites/kernel/socket/cases/net_socket_test_005.c
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
#include <nuttx/config.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include "SocketTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_net_socket_test05
 ****************************************************************************/

void test_nuttx_net_socket_test05(FAR void **state)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32_t hl = ntohl(0x12345678);
  syslog(LOG_INFO, "hl %#" PRIx32, hl);
  assert_int_equal(hl, 0x78563412);

  uint32_t nl = htonl(0x12345678);
  syslog(LOG_INFO, "nl %#" PRIx32, nl);
  assert_int_equal(nl, 0x78563412);

  uint16_t hs = ntohs(0x1234);
  syslog(LOG_INFO, "hs %#" PRIx16, hs);
  assert_int_equal(hs, 0x3412);

  uint16_t ns = htons(0x1234);
  syslog(LOG_INFO, "ns %#" PRIx16, ns);
  assert_int_equal(ns, 0x3412);
#else
  uint32_t hl = ntohl(0x12345678);
  syslog(LOG_INFO, "hl %#" PRIx32, hl);
  assert_int_equal(hl, 0x12345678);

  uint32_t nl = htonl(0x12345678);
  syslog(LOG_INFO, "nl %#" PRIx32, nl);
  assert_int_equal(nl, 0x12345678);

  uint16_t hs = ntohs(0x1234);
  syslog(LOG_INFO, "hs %#" PRIx16, hs);
  assert_int_equal(hs, 0x1234);

  uint16_t ns = htons(0x1234);
  syslog(LOG_INFO, "ns %#" PRIx16, ns);
  assert_int_equal(ns, 0x1234);
#endif
}
