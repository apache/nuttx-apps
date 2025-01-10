/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/lstat_test.c
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
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
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
 * Name: test_nuttx_syscall_lstat01
 ****************************************************************************/

void test_nuttx_syscall_lstat01(FAR void **state)
{
  uid_t user_id;
  gid_t group_id;
  struct stat stat_buf;
  int ret;

  char filename[64] = "";
  char symlink_path[64] = "/Lstat01_tst_syml";

  sprintf(filename, "%s_file", __func__);
  user_id = getuid();
  group_id = getgid();

  ret = open(filename, O_CREAT | O_WRONLY, 0644);
  assert_true(ret > 0);

  ret = close(ret);
  assert_int_equal(ret, 0);

  ret = symlink(filename, symlink_path);
  assert_int_equal(ret, 0);

  memset(&stat_buf, 0, sizeof(stat_buf));

  ret = lstat(symlink_path, &stat_buf);
  assert_int_equal(ret, 0);

  assert_true(stat_buf.st_gid == group_id);
  assert_true(stat_buf.st_uid == user_id);
  assert_true((stat_buf.st_mode & S_IFMT) == S_IFLNK);
  assert_int_equal(unlink(symlink_path), 0);
  assert_int_equal(unlink(filename), 0);
}
