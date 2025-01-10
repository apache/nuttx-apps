/****************************************************************************
 * apps/testing/testsuites/kernel/socket/cmocka_socket_test.c
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
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include "SocketTest.h"
#include <cmocka.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmocka_sched_test_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  /* Add Test Cases */

  const struct CMUnitTest nuttx_socket_test_suites[] =
  {
      cmocka_unit_test_setup_teardown(test_nuttx_net_socket_test05, NULL,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_nuttx_net_socket_test06, NULL,
                                      NULL),
#if defined(CONFIG_NET_LOOPBACK) || defined(CONFIG_NET_USRSOCK)
      cmocka_unit_test_setup_teardown(test_nuttx_net_socket_test08, NULL,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_nuttx_net_socket_test09, NULL,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_nuttx_net_socket_test10, NULL,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_nuttx_net_socket_test11, NULL,
                                      NULL),
#endif
  };

  /* Run Test cases */

  cmocka_run_group_tests(nuttx_socket_test_suites, NULL, NULL);

  return 0;
}
