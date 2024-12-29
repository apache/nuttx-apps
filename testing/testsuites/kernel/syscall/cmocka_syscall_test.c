/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cmocka_syscall_test.c
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
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <syslog.h>
#include <dirent.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "SyscallTest.h"
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

  const struct CMUnitTest nuttx_syscall_test_suites[] =
  {
      /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_bind01,
       * test_nuttx_syscall_test_group_setup,
       * test_nuttx_syscall_test_group_teardown),
       */

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_chdir01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_chdir02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

      /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_accept01,
       * test_nuttx_syscall_test_group_setup,
       * test_nuttx_syscall_test_group_teardown),
       */

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getitimer01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_clockgettime01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_clocknanosleep01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_clocksettime01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_close01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_close02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_close03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_creat01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_creat02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

      /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_fcntl01,
       * NULL, NULL),
       */

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fcntl02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fcntl03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fcntl04,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

  /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_fcntl05, NULL,
   * NULL),
   */

#ifndef CONFIG_ARCH_SIM
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fcntl06,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
#endif
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fstatfs01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fsync01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
#ifdef CONFIG_NET
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fsync02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fsync03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getpeername01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getsockopt01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

      /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_recvfromtest01,
       * test_nuttx_syscall_test_group_setup,
       * test_nuttx_syscall_test_group_teardown),
       */

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_setsockopt01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_listen01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

  /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_socketpair01,
   * test_nuttx_syscall_test_group_setup,
   * test_nuttx_syscall_test_group_teardown),
   */

#ifdef CONFIG_NET_LOCAL_DGRAM
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_socketpair02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
#endif
#endif

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_ftruncate01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getcwd01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getcwd02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getpid01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getppid01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_gethostname01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_gettimeofday01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_lseek01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_lseek07,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_lstat01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup04,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup05,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup201,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_dup202,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_fpathconf01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getegid01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getegid02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_geteuid01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

      /* not implement
       * cmocka_unit_test_setup_teardown(test_nuttx_syscall_geteuid02,
       * estnuttxsyscalltestgroupsetup,
       * test_nuttx_syscall_test_group_teardown),
       */

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getgid01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getgid02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_getuid01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

      /* not implement
       * cmocka_unit_test_setup_teardown(test_nuttx_syscall_getuid02,
       * test_nuttx_syscall_test_group_setup,
       * test_nuttx_syscall_test_group_teardown),
       */

      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_pathconf01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_pipe01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_pipe02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_pread01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_pwrite01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_pwrite02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_rmdir01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_rmdir02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_truncate01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_unlink01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_nansleep01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_nansleep02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_time01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_time02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_timercreate01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_timerdelete01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_timergettime01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_mkdir01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_mkdir02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_mkdir03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_sched01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_sched02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_sched03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_sched04,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_write01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_write02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_write03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_read01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_read02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_read03,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_read04,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_symlink01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_symlink02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),

  /* cmocka_unit_test_setup_teardown(test_nuttx_syscall_connect01,
   * test_nuttx_syscall_test_group_setup,
   * test_nuttx_syscall_test_group_teardown),
   */

#if !defined(CONFIG_ARCH_SIM) && defined(CONFIG_NET_IPv4)
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_sockettest01,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
      cmocka_unit_test_setup_teardown(
          test_nuttx_syscall_sockettest02,
          test_nuttx_syscall_test_group_setup,
          test_nuttx_syscall_test_group_teardown),
#endif
  };

  /* Run Test cases */

  cmocka_run_group_tests(nuttx_syscall_test_suites, NULL, NULL);
  return 0;
}
