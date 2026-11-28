/****************************************************************************
 * apps/testing/fs/refstest/refstest_main.c
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
#include <cmocka.h>

#include "refstest_common.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs01,
                                   refstest_setup, refstest_teardown),
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs02,
                                   refstest_setup, refstest_teardown),
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs03,
                                   refstest_setup, refstest_teardown),
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs04,
                                   refstest_setup, refstest_teardown),
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs05,
                                   refstest_setup, refstest_teardown),
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs06,
                                   refstest_setup, refstest_teardown),
#ifdef CONFIG_FS_RAMMAP
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs07,
                                   refstest_setup, refstest_teardown),
#endif
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs08,
                                   refstest_setup, refstest_teardown),
    cmocka_unit_test_setup_teardown(test_nuttx_fs_refs09,
                                   refstest_setup, refstest_teardown),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
