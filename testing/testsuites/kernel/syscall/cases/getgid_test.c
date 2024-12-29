/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/getgid_test.c
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
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <pwd.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_getgid01
 ****************************************************************************/

void test_nuttx_syscall_getgid01(FAR void **state)
{
  int lc;
  int ret;
  for (lc = 0; lc < 5; lc++)
    {
      ret = getgid();
      assert_int_not_equal(ret, -1);
    }
}

/****************************************************************************
 * Name: test_nuttx_syscall_getgid02
 ****************************************************************************/

void test_nuttx_syscall_getgid02(FAR void **state)
{
  int lc;
  int ret;
  uid_t uid;
  struct passwd *pwent;

  for (lc = 0; lc < 5; lc++)
    {
      ret = getgid();
      uid = getuid();
      pwent = getpwuid(uid);
      assert_non_null(pwent);
      assert_int_equal(pwent->pw_gid, ret);
    }
}
