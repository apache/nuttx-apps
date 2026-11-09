/****************************************************************************
 * apps/testing/nettest/udp/test_udp.c
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

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_udp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_UDP(f) \
  cmocka_unit_test_setup_teardown(f, test_udp_setup, test_udp_teardown)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest udp_tests[] =
    {
      TEST_UDP(test_udp_v6only_default),
      TEST_UDP(test_udp_v6only_mapped),
      TEST_UDP(test_udp_v6only_set0),
      TEST_UDP(test_udp_v6only_set1),
      TEST_UDP(test_udp_v6only_both),
      TEST_UDP(test_udp_v6only_send),
      TEST_UDP(test_udp_v6only_bound),
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_TCPBACKLOG)
      TEST_UDP(test_udp_v6only_tcp_set1),
      TEST_UDP(test_udp_v6only_tcp_set0),
      TEST_UDP(test_udp_v6only_tcp_ipv4),
      TEST_UDP(test_udp_v6only_tcp_bound),
#endif
      TEST_UDP(test_udp_family_alone),
      TEST_UDP(test_udp_family_both),
      TEST_UDP(test_udp_family_v6addr),
      TEST_UDP(test_udp_family_connect),
      TEST_UDP(test_udp_family_sendto6),
      TEST_UDP(test_udp_family_sendto4),
      TEST_UDP(test_udp_bind_v6_v4),
      TEST_UDP(test_udp_bind_v4_v6),
      TEST_UDP(test_udp_bind_v6only_v4),
      TEST_UDP(test_udp_bind_v4_v6only),
      TEST_UDP(test_udp_bind_reuse),
      TEST_UDP(test_udp_bind_v6addr),
    };

  return cmocka_run_group_tests(udp_tests, NULL, NULL);
}
