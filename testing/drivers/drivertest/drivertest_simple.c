/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_simple.c
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

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void drivertest_simple_assert(FAR void **state)
{
  UNUSED(state);
  assert_int_equal(0, 0);
}

static void drivertest_simple_assert_string(FAR void **state)
{
  UNUSED(state);
  assert_string_not_equal("hello", "world");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * drivertest_simple_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test(drivertest_simple_assert),
    cmocka_unit_test(drivertest_simple_assert_string),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
