/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_012.c
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
#include <limits.h>
#include <malloc.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmocka.h>

#include <nuttx/mm/mempool.h>
#include <nuttx/mm/mm.h>

#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MM12_BLKSIZE   64
#define MM12_PREALLOC  6
#define MM12_NALLOC    2

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct mempool_s g_mm12_pool =
{
  .initialbase = NULL,
  .initialsize = MM12_PREALLOC * MEMPOOL_REALBLOCKSIZE(MM12_BLKSIZE) +
                 MEMPOOL_HEADER_SIZE,
  .blocksize   = MM12_BLKSIZE,
  .expandsize  = 0,
  .maxalloc    = 0,
  .wait        = false,
  .name        = "mmtest_mm12",
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm12
 ****************************************************************************/

void test_nuttx_mm12(FAR void **state)
{
  FAR void *blk[MM12_NALLOC];
  struct mempoolinfo_s info;
  struct mallinfo_task taskinfo;
  struct malltask task;
  struct mm_memdump_s dump;
  int i;

  assert_int_equal(mempool_init(&g_mm12_pool), 0);

  for (i = 0; i < MM12_NALLOC; i++)
    {
      blk[i] = mempool_allocate(&g_mm12_pool, 0);
      assert_non_null(blk[i]);
    }

  /* mempool_info reports the current allocation counters. */

  assert_int_equal(mempool_info(&g_mm12_pool, &info), 0);
  assert_int_equal(info.aordblks, MM12_NALLOC);
  assert_int_equal(info.ordblks, MM12_PREALLOC - MM12_NALLOC);
  assert_int_equal(info.maxalloc, 0);
  assert_int_equal(info.sizeblks,
                   MEMPOOL_REALBLOCKSIZE(MM12_BLKSIZE));

  /* mempool_info_task: allocated and free counters. */

  task.pid = PID_MM_ALLOC;
#if CONFIG_MM_BACKTRACE >= 0
  task.seqmin = 0;
  task.seqmax = ULONG_MAX;
#endif
  taskinfo = mempool_info_task(&g_mm12_pool, &task);
  assert_int_equal(taskinfo.aordblks, MM12_NALLOC);

  task.pid = PID_MM_FREE;
  taskinfo = mempool_info_task(&g_mm12_pool, &task);
  assert_int_equal(taskinfo.aordblks, MM12_PREALLOC - MM12_NALLOC);

  /* Exercise the dump paths for allocated and free blocks. */

  dump.pid = PID_MM_ALLOC;
  mempool_memdump(&g_mm12_pool, &dump);
  dump.pid = PID_MM_FREE;
  mempool_memdump(&g_mm12_pool, &dump);

  for (i = 0; i < MM12_NALLOC; i++)
    {
      mempool_release(&g_mm12_pool, blk[i]);
    }

  assert_int_equal(mempool_deinit(&g_mm12_pool), 0);
}
