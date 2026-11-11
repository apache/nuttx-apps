/****************************************************************************
 * apps/testing/nettest/tcp/test_tcp.c
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
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_tcp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tcp_tests[] =
    {
#ifdef CONFIG_NET_IPv4
      cmocka_unit_test_setup_teardown(test_tcp_connect_ipv4,
                                      test_tcp_connect_ipv4_setup,
                                      test_tcp_common_teardown),
#  ifdef CONFIG_NET_SOCKOPTS
      cmocka_unit_test_setup_teardown(test_tcp_bind_reuseaddr_timewait,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_bind_reuseaddr_connected,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_bind_reuseaddr_listener,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_accept_inherit,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
#  endif
      cmocka_unit_test_setup_teardown(test_tcp_listen_again,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
      cmocka_unit_test_setup_teardown(test_tcp_listen_connected,
                                      test_tcp_bind_listen_setup,
                                      test_tcp_bind_listen_teardown),
#endif
#ifdef CONFIG_NET_IPv6
      cmocka_unit_test_setup_teardown(test_tcp_connect_ipv6,
                                      test_tcp_connect_ipv6_setup,
                                      test_tcp_common_teardown),
#endif
    };

  return cmocka_run_group_tests(tcp_tests, test_tcp_group_setup,
                                test_tcp_group_teardown);
}
