/****************************************************************************
 * apps/testing/testsuites/kernel/sched/cmocka_sched_test.c
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
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include "SchedTest.h"
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

  const struct CMUnitTest nuttx_sched_test_suites[] =
  {
      cmocka_unit_test(test_nuttx_sched_pthread01),
      cmocka_unit_test(test_nuttx_sched_pthread02),
      cmocka_unit_test(test_nuttx_sched_pthread03),
      cmocka_unit_test(test_nuttx_sched_pthread04),
      cmocka_unit_test(test_nuttx_sched_pthread05),
      cmocka_unit_test(test_nuttx_sched_pthread06),
      cmocka_unit_test(test_nuttx_sched_pthread07),
      cmocka_unit_test(test_nuttx_sched_pthread08),
      cmocka_unit_test(test_nuttx_sched_pthread09),
      cmocka_unit_test(test_nuttx_sched_task01),
      cmocka_unit_test(test_nuttx_sched_task02),
      cmocka_unit_test(test_nuttx_sched_task03),
      cmocka_unit_test(test_nuttx_sched_task04),
      cmocka_unit_test(test_nuttx_sched_task05),
      cmocka_unit_test(test_nuttx_sched_task06),
      cmocka_unit_test(test_nuttx_sched_task07),
  };

  /* Run Test cases */

  cmocka_run_group_tests(nuttx_sched_test_suites,
                         test_nuttx_sched_test_group_setup,
                         test_nuttx_sched_test_group_teardown);
  return 0;
}
