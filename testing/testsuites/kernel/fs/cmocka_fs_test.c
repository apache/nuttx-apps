/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cmocka_fs_test.c
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
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include "fstest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmocka_fs_test_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  /* Add Test Cases */

  const struct CMUnitTest nuttx_fs_test_suites[] = {
      cmocka_unit_test_setup_teardown(test_nuttx_fs_creat01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_dup01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fcntl01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#ifndef CONFIG_ARCH_SIM
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fcntl02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fcntl03,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#endif
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fstat01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#ifndef CONFIG_ARCH_SIM
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fstat02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#endif
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fstatfs01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fsync01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fsync02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_getfilep01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_mkdir01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_open01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_opendir01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_opendir02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_pread01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_pwrite01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#ifndef CONFIG_ARCH_SIM
      cmocka_unit_test_setup_teardown(test_nuttx_fs_read01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#endif
      cmocka_unit_test_setup_teardown(test_nuttx_fs_readdir01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_readlink01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_rename01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_rename02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_rewinddir01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_fmdir01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_rmdir02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_rmdir03,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_seek01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_seek02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#ifndef CONFIG_ARCH_SIM
      cmocka_unit_test_setup_teardown(test_nuttx_fs_stat01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
#endif
      cmocka_unit_test_setup_teardown(test_nuttx_fs_statfs01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_symlink01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_truncate01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_unlink01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_write01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_write02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_write03,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_append01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_sendfile01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_sendfile02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_stream01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_stream02,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_stream03,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_stream04,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_eventfd,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
      cmocka_unit_test_setup_teardown(test_nuttx_fs_poll01,
                                      test_nuttx_fs_fest_group_set_up,
                                      test_nuttx_fs_test_group_tear_down),
  };

  /* Run Test cases */

  cmocka_run_group_tests(nuttx_fs_test_suites, NULL, NULL);
  return 0;
}
