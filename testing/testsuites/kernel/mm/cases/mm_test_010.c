/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_010.c
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
#include <nuttx/compiler.h>

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

#include <nuttx/mm/mempool.h>
#include <nuttx/mm/mm.h>

#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MM10_BLKSIZE   32
#define MM10_PREALLOC  4

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Static pool with four preallocated blocks.  maxalloc stops the pool
 * from growing, so the next allocation after all blocks are taken must
 * fail.
 */

MEMPOOL_DEFINE(g_mm10_static, MM10_BLKSIZE, MM10_PREALLOC, MM10_PREALLOC,
               0);

/* Dynamic pool without a static buffer: the initial blocks are allocated
 * from the user heap by mempool_init().
 */

static struct mempool_s g_mm10_dyn =
{
  .initialbase = NULL,
  .initialsize = MM10_PREALLOC * MEMPOOL_REALBLOCKSIZE(MM10_BLKSIZE) +
                 MEMPOOL_HEADER_SIZE,
  .blocksize   = MM10_BLKSIZE,
  .expandsize  = 0,
  .maxalloc    = 0,
  .wait        = false,
  .name        = "mmtest_mm10",
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm10
 ****************************************************************************/

void test_nuttx_mm10(FAR void **state)
{
  FAR void *blk[MM10_PREALLOC];
  FAR void *extra;
  struct mempoolinfo_s info;
  int i;
  int j;

  /* Static pool: take every block, then hit the maxalloc limit. */

  assert_int_equal(mempool_init(&g_mm10_static), 0);
  assert_int_equal(mempool_info(&g_mm10_static, &info), 0);
  assert_int_equal(info.ordblks, MM10_PREALLOC);
  assert_int_equal(info.aordblks, 0);
  assert_int_equal(info.sizeblks,
                   MEMPOOL_REALBLOCKSIZE(MM10_BLKSIZE));

  for (i = 0; i < MM10_PREALLOC; i++)
    {
      blk[i] = mempool_allocate(&g_mm10_static, 0);
      assert_non_null(blk[i]);
    }

  assert_int_equal(mempool_navail(&g_mm10_static), 0);

  extra = mempool_allocate(&g_mm10_static, 0);
  assert_null(extra);

  for (i = 0; i < MM10_PREALLOC; i++)
    {
      mempool_release(&g_mm10_static, blk[i]);
    }

  assert_int_equal(mempool_navail(&g_mm10_static), MM10_PREALLOC);
  assert_int_equal(mempool_deinit(&g_mm10_static), 0);

  /* Dynamic pool: the initial blocks come from the heap. */

  assert_int_equal(mempool_init(&g_mm10_dyn), 0);
  assert_int_equal(mempool_info(&g_mm10_dyn, &info), 0);
  assert_int_equal(info.ordblks, MM10_PREALLOC);
  assert_int_equal(info.aordblks, 0);

  for (i = 0; i < MM10_PREALLOC; i++)
    {
      blk[i] = mempool_zallocate(&g_mm10_dyn, 0);
      assert_non_null(blk[i]);
    }

  for (i = 0; i < MM10_PREALLOC; i++)
    {
      for (j = 0; j < MM10_BLKSIZE; j++)
        {
          assert_int_equal(((FAR uint8_t *)blk[i])[j], 0);
        }
    }

  /* The pool cannot expand and does not wait. */

  extra = mempool_allocate(&g_mm10_dyn, 0);
  assert_null(extra);

  /* Deinit is refused while blocks are in use. */

  assert_int_equal(mempool_deinit(&g_mm10_dyn), -EBUSY);

  for (i = 0; i < MM10_PREALLOC; i++)
    {
      mempool_release(&g_mm10_dyn, blk[i]);
    }

  assert_int_equal(mempool_deinit(&g_mm10_dyn), 0);
}
