/****************************************************************************
 * apps/testing/testsuites/kernel/mutex/cmocka_mutex_test.c
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
#include "MutexTest.h"
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

  const struct CMUnitTest nuttx_mutex_test_suites[] =
  {
      cmocka_unit_test_setup_teardown(test_nuttx_mutex_test01, NULL, NULL),
#ifdef CONFIG_PTHREAD_MUTEX_ROBUST
      cmocka_unit_test_setup_teardown(test_nuttx_mutex_test19, NULL, NULL),
#endif
      cmocka_unit_test_setup_teardown(test_nuttx_mutex_test20, NULL, NULL),
  };

  /* Run Test cases */

  cmocka_run_group_tests(nuttx_mutex_test_suites, NULL, NULL);

  return 0;
}
