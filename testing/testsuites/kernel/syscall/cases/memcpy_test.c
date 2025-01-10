/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/memcpy_test.c
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
#define LEN 100

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_fill
 ****************************************************************************/

static void test_nuttx_syscall_fill(char *str, int len)
{
  int i;
  for (i = 0; i < len; i++)
    *str++ = 'a';
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_check
 ****************************************************************************/

static int test_nuttx_syscall_check(char *str, int len)
{
  int i;
  for (i = 0; i < len; i++)
      if (*str++ != 'a')
          return (-1);

  return 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_setup
 ****************************************************************************/

static void test_nuttx_syscall_setup(char *buf)
{
  int i;
  for (i = 0; i < BSIZE; i++)
    buf[i] = 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_verifymemcpy
 ****************************************************************************/

static void test_nuttx_syscall_verifymemcpy(char *p, char *q, int len)
{
  test_nuttx_syscall_fill(p, len);
  memcpy(q, p, LEN);

  assert_int_equal(test_nuttx_syscall_check(q, len), 0);

  assert_true(*(p - 1) == '\0');
  assert_true(p[LEN] == '\0');

  assert_true(*(q - 1) == '\0');
  assert_true(q[LEN] == '\0');
}

/****************************************************************************
 * Pubilc Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_memcpytest01
 ****************************************************************************/

void test_nuttx_syscall_memcpytest01(FAR void **state)
{
  char *buf = NULL;
  buf = malloc(BSIZE);
  assert_non_null(buf);
  test_nuttx_syscall_setup(buf);

  test_nuttx_syscall_verifymemcpy(&buf[100], &buf[800], LEN);
  test_nuttx_syscall_verifymemcpy(&buf[800], &buf[100], LEN);
  free(buf);
}
