/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_009.c
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
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MM_ALIGN_COUNT 1024

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The offset of m is alignof(max_align_t) */

struct mm_align_s
{
  char c;
  max_align_t m;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mm_misaligned(FAR void *ptr, size_t align)
{
  /* The compiler assumes that malloc() memory is aligned: read the address
   * back through a volatile, so that it does not remove the check.
   */

  FAR void *volatile p = ptr;

  return (uintptr_t)p % align != 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm09
 *
 * Description:
 *   malloc(), calloc() and realloc() must return memory aligned to
 *   alignof(max_align_t) (C11 7.22.3).
 ****************************************************************************/

void test_nuttx_mm09(FAR void **state)
{
  size_t align = offsetof(struct mm_align_s, m);
  FAR void **ptrs;
  FAR void *p;
  int bad = 0;
  int i;

  ptrs = calloc(MM_ALIGN_COUNT, sizeof(*ptrs));
  assert_non_null(ptrs);

  for (i = 0; i < MM_ALIGN_COUNT; i++)
    {
      ptrs[i] = malloc(1 + (i * 37) % 300);
      bad += mm_misaligned(ptrs[i], align);

      if (i % 3 == 0)
        {
          free(ptrs[i]);
          ptrs[i] = NULL;
        }
    }

  for (i = 0; i < MM_ALIGN_COUNT; i++)
    {
      if (ptrs[i] != NULL)
        {
          p = realloc(ptrs[i], 1 + (i * 53) % 600);
        }
      else
        {
          p = calloc(1, 1 + (i * 11) % 400);
        }

      if (p != NULL)
        {
          bad += mm_misaligned(p, align);
          ptrs[i] = p;
        }
    }

  for (i = 0; i < MM_ALIGN_COUNT; i++)
    {
      free(ptrs[i]);
    }

  free(ptrs);
  assert_int_equal(bad, 0);
}
