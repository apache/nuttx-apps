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
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include "SchedTest.h"
#include <cmocka.h>

/****************************************************************************
 * Name: cmocka_sched_test_main
 ****************************************************************************/
int main(int argc, char *argv[])
{
  /* Add Test Cases */
  const struct CMUnitTest NuttxSchedTestSuites[] = {
      cmocka_unit_test(TestNuttxSchedPthread01),
      cmocka_unit_test(TestNuttxSchedPthread02),
      cmocka_unit_test(TestNuttxSchedPthread03),
      cmocka_unit_test(TestNuttxSchedPthread04),
      cmocka_unit_test(TestNuttxSchedPthread05),
      cmocka_unit_test(TestNuttxSchedPthread06),
      cmocka_unit_test(TestNuttxSchedPthread07),
      cmocka_unit_test(TestNuttxSchedPthread08),
      cmocka_unit_test(TestNuttxSchedPthread09),
      cmocka_unit_test(TestNuttxSchedTask01),
      cmocka_unit_test(TestNuttxSchedTask02),
      cmocka_unit_test(TestNuttxSchedTask03),
      cmocka_unit_test(TestNuttxSchedTask04),
      cmocka_unit_test(TestNuttxSchedTask05),
      cmocka_unit_test(TestNuttxSchedTask06),
      cmocka_unit_test(TestNuttxSchedTask07),
  };

  /* Run Test cases */
  cmocka_run_group_tests(NuttxSchedTestSuites, TestNuttxSchedTestGroupSetup, TestNuttxSchedTestGroupTearDown);
  return 0;
}