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
#include "MmTest.h"
#include <cmocka.h>

/****************************************************************************
 * Name: cmocka_sched_test_main
 ****************************************************************************/
int main(int argc, char *argv[])
{

  /* Add Test Cases */
  const struct CMUnitTest NuttxMmTestSuites[] = {
      cmocka_unit_test_setup_teardown(TestNuttxMm01, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm02, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm03, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm04, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm05, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm06, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm07, TestNuttxMmsetup, TestNuttxMmteardown),
      cmocka_unit_test_setup_teardown(TestNuttxMm08, TestNuttxMmsetup, TestNuttxMmteardown),
  };

  /* Run Test cases */
  cmocka_run_group_tests(NuttxMmTestSuites, NULL, NULL);

  return 0;
}