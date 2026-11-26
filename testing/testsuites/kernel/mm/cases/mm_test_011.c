/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_011.c
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
#include <malloc.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <cmocka.h>

#include <nuttx/mm/mempool.h>

#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MM11_CHUNKSIZE   4
#define MM11_EXPANDSIZE  4096

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *mm11_alloc(FAR void *arg, size_t alignment, size_t size)
{
  return memalign(alignment, size);
}

static void mm11_free(FAR void *arg, FAR void *addr)
{
  free(addr);
}

static size_t mm11_alloc_size(FAR void *arg, FAR void *addr)
{
  return malloc_size(addr);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm11
 ****************************************************************************/

void test_nuttx_mm11(FAR void **state)
{
  static const size_t poolsize[] =
  {
    16, 32, 64
  };

  FAR struct mempool_multiple_s *mpool;
  FAR void *blk;
  FAR char outside[8];
  ssize_t size;

  mpool = mempool_multiple_init("mmtest_mm11", poolsize,
                                sizeof(poolsize) / sizeof(poolsize[0]),
                                mm11_alloc, mm11_alloc_size, mm11_free,
                                NULL, MM11_CHUNKSIZE, MM11_EXPANDSIZE,
                                MM11_CHUNKSIZE);
  assert_non_null(mpool);

  /* A small request is served by the smallest pool. */

  blk = mempool_multiple_alloc(mpool, 8);
  assert_non_null(blk);
  size = mempool_multiple_alloc_size(mpool, blk);
  assert_int_equal(size, 16);
  assert_true(mempool_multiple_member(mpool, blk));
  assert_false(mempool_multiple_member(mpool, outside));
  assert_int_equal(mempool_multiple_free(mpool, blk), 0);

  /* Larger requests move to the matching block size. */

  blk = mempool_multiple_alloc(mpool, 20);
  assert_non_null(blk);
  assert_int_equal(mempool_multiple_alloc_size(mpool, blk), 32);
  assert_int_equal(mempool_multiple_free(mpool, blk), 0);

  blk = mempool_multiple_alloc(mpool, 50);
  assert_non_null(blk);
  assert_int_equal(mempool_multiple_alloc_size(mpool, blk), 64);
  assert_int_equal(mempool_multiple_free(mpool, blk), 0);

  /* Releasing a foreign pointer is refused. */

  assert_int_equal(mempool_multiple_free(mpool, outside), -EINVAL);

  mempool_multiple_deinit(mpool);
}
