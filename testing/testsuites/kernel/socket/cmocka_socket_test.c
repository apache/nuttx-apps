/*
 * Copyright (C) 2020 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include "SocketTest.h"
#include <cmocka.h>

/****************************************************************************
 * Name: cmocka_sched_test_main
 ****************************************************************************/
int main(int argc, char *argv[])
{

  /* Add Test Cases */
  const struct CMUnitTest NuttxSocketTestSuites[] = {
      cmocka_unit_test_setup_teardown(TestNuttxNetSocketTest05, NULL, NULL),
      cmocka_unit_test_setup_teardown(TestNuttxNetSocketTest06, NULL, NULL),
#if defined(CONFIG_NET_LOOPBACK) || defined(CONFIG_NET_USRSOCK)
      cmocka_unit_test_setup_teardown(TestNuttxNetSocketTest08, NULL, NULL),
      cmocka_unit_test_setup_teardown(TestNuttxNetSocketTest09, NULL, NULL),
      cmocka_unit_test_setup_teardown(TestNuttxNetSocketTest10, NULL, NULL),
      cmocka_unit_test_setup_teardown(TestNuttxNetSocketTest11, NULL, NULL),
#endif
  };

  /* Run Test cases */
  cmocka_run_group_tests(NuttxSocketTestSuites, NULL, NULL);

  return 0;
}