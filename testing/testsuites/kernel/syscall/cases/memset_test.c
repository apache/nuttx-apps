/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/memset_test.c
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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define BSIZE 4096

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_fill
 ****************************************************************************/

static void test_nuttx_syscall_fill(char *buf)
{
  int i;
  for (i = 0; i < BSIZE; i++)
    buf[i] = 'a';
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_check
 ****************************************************************************/

static int test_nuttx_syscall_check(char *str)
{
  int i = 0;
  while (!*str++)
    i++;
  return i;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_memsettest01
 ****************************************************************************/

void test_nuttx_syscall_memsettest01(FAR void **state)
{
  char *buf = NULL;
  buf = malloc(BSIZE);
  assert_non_null(buf);
  test_nuttx_syscall_fill(buf);

  int i;
  int j;
  char *p = &buf[400];

  for (i = 0; i < 200; i++)
    {
      test_nuttx_syscall_fill(buf);
      memset(p, 0, i);
      j = test_nuttx_syscall_check(p);

      assert_true(j == i);
      assert_true(*(p - 1) && p[i]);
    }

  assert_int_equal(i, 200);
  free(buf);
}
