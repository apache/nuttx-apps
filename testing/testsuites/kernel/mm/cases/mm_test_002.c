/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_002.c
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
#include <stdlib.h>
#include <syslog.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "MmTest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm02
 ****************************************************************************/

void test_nuttx_mm02(FAR void **state)
{
  int i;
  int flag = 0;
  char *pm1;
  char *pm2;
  unsigned long memsize;

  memsize = mmtest_get_memsize();
  pm2 = pm1 = calloc(memsize, 1);
  assert_non_null(pm2);

  for (i = 0; i < memsize; i++)
    {
      if (*pm2++ != 0)
        {
          flag = 1;
        }
    }

  pm2 = pm1;
  for (i = 0; i < memsize; i++)
    *pm2++ = 'X';
  pm2 = pm1;
  for (i = 0; i < memsize; i++)
    {
      if (*pm2++ != 'X')
        {
          flag = 1;
        }
    }

  free(pm1);
  assert_int_equal(flag, 0);
}
