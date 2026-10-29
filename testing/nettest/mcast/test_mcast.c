/****************************************************************************
 * apps/testing/nettest/mcast/test_mcast.c
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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_mcast.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest mcast_tests[] =
    {
      cmocka_unit_test(test_mcast_names),
#ifdef CONFIG_NET_IGMP
      cmocka_unit_test_setup_teardown(test_mcast_join_ipv4,
                                      test_mcast_ipv4_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_leave_ipv4,
                                      test_mcast_ipv4_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_optlen_ipv4,
                                      test_mcast_ipv4_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_family_ipv4,
                                      test_mcast_ipv4_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_source_ipv4,
                                      test_mcast_ipv4_setup,
                                      test_mcast_common_teardown),
#endif
#ifdef CONFIG_NET_MLD
      cmocka_unit_test_setup_teardown(test_mcast_join_ipv6,
                                      test_mcast_ipv6_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_leave_ipv6,
                                      test_mcast_ipv6_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_optlen_ipv6,
                                      test_mcast_ipv6_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_family_ipv6,
                                      test_mcast_ipv6_setup,
                                      test_mcast_common_teardown),
      cmocka_unit_test_setup_teardown(test_mcast_source_ipv6,
                                      test_mcast_ipv6_setup,
                                      test_mcast_common_teardown),
#endif
    };

  return cmocka_run_group_tests(mcast_tests, test_mcast_group_setup,
                                test_mcast_group_teardown);
}
