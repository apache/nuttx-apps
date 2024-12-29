/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_mkdir_test.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <syslog.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_mkdir01
 ****************************************************************************/

void test_nuttx_fs_mkdir01(FAR void **state)
{
  int status;

  /* do mkdir */

  status = mkdir("testdir1", 0700);
  assert_int_equal(status, 0);

  /* do rmdir */

  assert_int_equal(rmdir("testdir1"), 0);

  /* do mkdir */

  status = mkdir("234123412341234", 0700);
  assert_int_equal(status, 0);

  /* do rmdir */

  assert_int_equal(rmdir("234123412341234"), 0);

  /* do mkdir */

  status = mkdir("asdfasdfASDFASDF", 0700);
  assert_int_equal(status, 0);

  /* do rmdir */

  assert_int_equal(rmdir("asdfasdfASDFASDF"), 0);

  /* do mkdir */

  status = mkdir("ASDFASD@%#%54365465654#@%#%@#", 0700);
  assert_int_equal(status, 0);

  /* do rmdir */

  assert_int_equal(rmdir("ASDFASD@%#%54365465654#@%#%@#"), 0);
}
