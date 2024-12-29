/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/memcmp_test.c
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
 * Name: TestNuttxSyscallFill
 ****************************************************************************/

static void test_nuttx_syscall_lfill(char *str, int len)
{
  int i;
  for (i = 0; i < len; i++)
    *str++ = 'a';
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
  return;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_verifymemcmp
 ****************************************************************************/

static void test_nuttx_syscall_verifymemcmp(char *p, char *q, int len)
{
  test_nuttx_syscall_lfill(p, len);
  test_nuttx_syscall_lfill(q, len);

  assert_int_equal(memcmp(p, q, len), 0);

  p[len - 1] = 0;
  assert_true(memcmp(p, q, len) < 0);

  p[len - 1] = 'a';
  p[0] = 0;
  assert_true(memcmp(p, q, len) < 0);

  p[0] = 'a';
  q[len - 1] = 0;
  assert_true(memcmp(p, q, len) > 0);

  q[len - 1] = 'a';
  q[0] = 0;
  assert_true(memcmp(p, q, len) > 0);

  q[0] = 'a';
  assert_int_equal(memcmp(p, q, len), 0);
}

/****************************************************************************
 * Pubilc Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_memcmptest01
 ****************************************************************************/

void test_nuttx_syscall_memcmptest01(FAR void **state)
{
  char *buf = NULL;
  buf = malloc(BSIZE);
  assert_non_null(buf);
  test_nuttx_syscall_setup(buf);

  test_nuttx_syscall_verifymemcmp(&buf[100], &buf[800], LEN);
  test_nuttx_syscall_verifymemcmp(&buf[800], &buf[100], LEN);

  free(buf);
}
