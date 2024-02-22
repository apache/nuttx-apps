/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cmocka_mm_test.c
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmocka_sched_test_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  /* Add Test Cases */

  const struct CMUnitTest nuttx_mm_test_suites[] = {
      cmocka_unit_test_setup_teardown(test_nuttx_mm01,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm02,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm03,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm04,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm05,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm06,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm07,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
      cmocka_unit_test_setup_teardown(test_nuttx_mm08,
                                      test_nuttx_mmsetup,
                                      test_nuttx_mmteardown),
  };

  /* Run Test cases */

  cmocka_run_group_tests(nuttx_mm_test_suites, NULL, NULL);

  return 0;
}
