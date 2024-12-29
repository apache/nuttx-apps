/****************************************************************************
 * apps/testing/testsuites/kernel/socket/cases/net_socket_test_006.c
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
#include <netinet/in.h>
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
 * Name: test_nuttx_net_socket_test06
 ****************************************************************************/

void test_nuttx_net_socket_test06(FAR void **state)
{
  struct in_addr in;
  int ret = inet_pton(AF_INET, "300.10.10.10", &in);
  syslog(LOG_INFO, "ret: %d", ret);
  assert_int_equal(ret, 0);

  ret = inet_pton(AF_INET, "10.11.12.13", &in);
  syslog(LOG_INFO, "ret: %d", ret);
  assert_int_equal(ret, 1);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  syslog(LOG_INFO, "in.s_addr: %#" PRIx32, in.s_addr);
  assert_int_equal(in.s_addr, 0x0d0c0b0a);
#else
  syslog(LOG_INFO, "in.s_addr: %#" PRIx32, in.s_addr);
  assert_int_equal(in.s_addr, 0x0a0b0c0d);
#endif
  /* Currently nuttx does not support the following interfaces inet_lnaof,
   * inet_netof, inet_makeaddr
   */

  /* host order */

  /* in_addr_t lna = inet_lnaof(in);
   * syslog(LOG_INFO, "lna: %#"PRIx32, lna);
   * assert_int_equal(lna, 0x000b0c0d);
   */

  /* host order */

  /* in_addr_t net = inet_netof(in);
   * syslog(LOG_INFO, "net: %#"PRIx32, net);
   * assert_int_equal(net, 0x0000000a);

   * in = inet_makeaddr(net, lna);
   * #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
   *   syslog(LOG_INFO, "in.s_addr: %#"PRIx32, in.s_addr);
   *   assert_int_equal(in.s_addr, 0x0d0c0b0a);
   * #else
   *   syslog(LOG_INFO, "in.s_addr: %#"PRIx32, in.s_addr);
   *   assert_int_equal(in.s_addr, 0x0a0b0c0d);
   * #endif
   */

  in_addr_t net = inet_network("300.10.10.10");
  syslog(LOG_INFO, "net: %#" PRIx32, net);
  assert_int_equal((int32_t)net, -1);

  /* host order */

  net = inet_network("10.11.12.13");
  syslog(LOG_INFO, "host order net: %#" PRIx32, net);
  assert_int_equal(net, 0x0a0b0c0d);

  const char *p = inet_ntoa(in);
  syslog(LOG_INFO, "inet_ntoa p: %s", p);
  assert_int_equal(strcmp(p, "10.11.12.13"), 0);

  char buf[32];
  p = inet_ntop(AF_INET, &in, buf, sizeof(buf));
  syslog(LOG_INFO, "inet_ntop p: %s", p);
  assert_string_equal(p, buf);
  assert_int_equal(strcmp(p, "10.11.12.13"), 0);
}
