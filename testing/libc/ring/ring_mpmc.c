/****************************************************************************
 * apps/testing/libc/ring/ring_mpmc.c
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

#include "ring_test.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Per-test static rings, kept off the test stack */

RING_TEST_DEFINE(g_mpmc_basic, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_batch, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_full, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_typed, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_boundary, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_partial, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_wrap, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_cap2, uint32_t, 2);
RING_TEST_DEFINE(g_mpmc_count, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_conc, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_ow, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_ow_wrap, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_ow_batch, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_ow_cap, uint32_t, 8);
RING_TEST_DEFINE(g_mpmc_ow_conc, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_mpmc_skip_conc, uint32_t, RING_TEST_CAP);

/****************************************************************************
 * Private Functions - Single-core functional test
 ****************************************************************************/

static void test_mpmc_basic(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_basic);

  for (i = 0; i < 16; i++)
    {
      val = i + 500;
      n = rt_mpmc_enqueue(&g_mpmc_basic, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(&g_mpmc_basic), 16);

  for (i = 0; i < 16; i++)
    {
      n = rt_mpmc_dequeue(&g_mpmc_basic, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 500);
    }
}

static void test_mpmc_batch(FAR void **state)
{
  uint32_t buf[RING_TEST_BATCH];
  uint32_t out[RING_TEST_BATCH];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_batch);

  for (i = 0; i < RING_TEST_BATCH; i++)
    {
      buf[i] = i + 200;
    }

  n = rt_mpmc_enqueue(&g_mpmc_batch, buf, RING_TEST_BATCH);
  assert_int_equal(n, RING_TEST_BATCH);
  n = rt_mpmc_dequeue(&g_mpmc_batch, out, RING_TEST_BATCH);
  assert_int_equal(n, RING_TEST_BATCH);

  for (i = 0; i < RING_TEST_BATCH; i++)
    {
      assert_int_equal(out[i], i + 200);
    }
}

static void test_mpmc_full(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_full);

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      val = i;
      n = rt_mpmc_enqueue(&g_mpmc_full, &val, 1);
      assert_int_equal(n, 1);
    }

  val = 999;
  n = rt_mpmc_enqueue(&g_mpmc_full, &val, 1);
  assert_int_equal(n, 0);

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      n = rt_mpmc_dequeue(&g_mpmc_full, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i);
    }

  assert_int_equal(rt_count(&g_mpmc_full), 0);
}

static void test_mpmc_typed_api(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t batch[4];
  uint32_t obatch[4];
  uint32_t n;
  uint32_t i;

  ring_test_init(&g_mpmc_typed);

  val = 42;
  n = rt_mpmc_enqueue(&g_mpmc_typed, &val, 1);
  assert_int_equal(n, 1);
  n = rt_mpmc_dequeue(&g_mpmc_typed, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 42);

  for (i = 0; i < 4; i++)
    {
      batch[i] = i + 500;
    }

  n = rt_mpmc_enqueue(&g_mpmc_typed, batch, 4);
  assert_int_equal(n, 4);
  n = rt_mpmc_dequeue(&g_mpmc_typed, obatch, 4);
  assert_int_equal(n, 4);

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(obatch[i], i + 500);
    }
}

static void test_mpmc_alloc_free(FAR void **state)
{
  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  assert_non_null(ring_test_alloc(r, RING_TEST_CAP, sizeof(uint32_t)));

  val = 77;
  n = rt_mpmc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_mpmc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 77);

  ring_free(r->r);
}

/****************************************************************************
 * Private Functions - Boundary condition tests (MP enqueue, MC dequeue)
 ****************************************************************************/

static void test_mpmc_enqueue_dequeue_boundary(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_boundary);

  /* Dequeue from empty ring (MC CAS path) */

  n = rt_mpmc_dequeue(&g_mpmc_boundary, &out, 1);
  assert_int_equal(n, 0);

  /* Enqueue count=0 (MP CAS path) */

  val = 42;
  n = rt_mpmc_enqueue(&g_mpmc_boundary, &val, 0);
  assert_int_equal(n, 0);
  assert_int_equal(rt_count(&g_mpmc_boundary), 0);

  /* Dequeue count=0 */

  n = rt_mpmc_dequeue(&g_mpmc_boundary, &out, 0);
  assert_int_equal(n, 0);

  /* Fill ring (MP CAS path), then try one more */

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      val = i;
      n = rt_mpmc_enqueue(&g_mpmc_boundary, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(&g_mpmc_boundary), RING_TEST_CAP);

  val = 999;
  n = rt_mpmc_enqueue(&g_mpmc_boundary, &val, 1);
  assert_int_equal(n, 0);
}

static void test_mpmc_partial_batch(FAR void **state)
{
  uint32_t buf[16];
  uint32_t out[16];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_partial);

  /* Batch enqueue larger than capacity (MP CAS path) */

  for (i = 0; i < 16; i++)
    {
      buf[i] = i + 1;
    }

  n = rt_mpmc_enqueue(&g_mpmc_partial, buf, 16);
  assert_int_equal(n, 8);

  /* Batch dequeue larger than available (MC CAS path) */

  n = rt_mpmc_dequeue(&g_mpmc_partial, out, 16);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 1);
    }

  /* Partial enqueue when partially full */

  for (i = 0; i < 6; i++)
    {
      buf[i] = i + 100;
    }

  n = rt_mpmc_enqueue(&g_mpmc_partial, buf, 6);
  assert_int_equal(n, 6);

  for (i = 0; i < 4; i++)
    {
      buf[i] = i + 200;
    }

  n = rt_mpmc_enqueue(&g_mpmc_partial, buf, 4);
  assert_int_equal(n, 2);
  assert_int_equal(rt_count(&g_mpmc_partial), 8);

  /* Partial dequeue (MC CAS path) */

  n = rt_mpmc_dequeue(&g_mpmc_partial, out, 5);
  assert_int_equal(n, 5);
  assert_int_equal(rt_count(&g_mpmc_partial), 3);

  n = rt_mpmc_dequeue(&g_mpmc_partial, out, 3);
  assert_int_equal(n, 3);
  assert_int_equal(rt_count(&g_mpmc_partial), 0);
}

static void test_mpmc_wraparound_batch(FAR void **state)
{
  uint32_t buf[8];
  uint32_t out[8];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_wrap);

  /* Advance indices to force wraparound */

  for (i = 0; i < 6; i++)
    {
      buf[i] = i;
    }

  n = rt_mpmc_enqueue(&g_mpmc_wrap, buf, 6);
  assert_int_equal(n, 6);
  n = rt_mpmc_dequeue(&g_mpmc_wrap, out, 6);
  assert_int_equal(n, 6);

  /* Enqueue 8 items wrapping around (MP CAS path) */

  for (i = 0; i < 8; i++)
    {
      buf[i] = i + 1000;
    }

  n = rt_mpmc_enqueue(&g_mpmc_wrap, buf, 8);
  assert_int_equal(n, 8);

  /* Dequeue wrapping around (MC CAS path) */

  memset(out, 0, sizeof(out));
  n = rt_mpmc_dequeue(&g_mpmc_wrap, out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 1000);
    }
}

static void test_mpmc_capacity_two(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  ring_test_init(&g_mpmc_cap2);

  assert_int_equal(rt_capacity(&g_mpmc_cap2), 2);

  val = 10;
  n = rt_mpmc_enqueue(&g_mpmc_cap2, &val, 1);
  assert_int_equal(n, 1);

  val = 20;
  n = rt_mpmc_enqueue(&g_mpmc_cap2, &val, 1);
  assert_int_equal(n, 1);

  /* Full (MP CAS path) */

  val = 30;
  n = rt_mpmc_enqueue(&g_mpmc_cap2, &val, 1);
  assert_int_equal(n, 0);

  /* MC dequeue */

  n = rt_mpmc_dequeue(&g_mpmc_cap2, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 10);

  n = rt_mpmc_dequeue(&g_mpmc_cap2, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 20);

  /* Empty (MC CAS path) */

  n = rt_mpmc_dequeue(&g_mpmc_cap2, &out, 1);
  assert_int_equal(n, 0);
}

static void test_mpmc_count_boundary(FAR void **state)
{
  uint32_t val;
  uint32_t out[8];
  uint32_t i;

  ring_test_init(&g_mpmc_count);

  assert_int_equal(rt_count(&g_mpmc_count), 0);

  for (i = 0; i < 3; i++)
    {
      val = i;
      rt_mpmc_enqueue(&g_mpmc_count, &val, 1);
    }

  assert_int_equal(rt_count(&g_mpmc_count), 3);

  for (i = 3; i < 8; i++)
    {
      val = i;
      rt_mpmc_enqueue(&g_mpmc_count, &val, 1);
    }

  assert_int_equal(rt_count(&g_mpmc_count), 8);

  rt_mpmc_dequeue(&g_mpmc_count, out, 1);
  assert_int_equal(rt_count(&g_mpmc_count), 7);

  rt_mpmc_dequeue(&g_mpmc_count, out, 7);
  assert_int_equal(rt_count(&g_mpmc_count), 0);
}

/****************************************************************************
 * Private Functions - Overwrite enqueue tests
 ****************************************************************************/

static void test_mpmc_overwrite_basic(FAR void **state)
{
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow);

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_mpmc_overwrite(&g_mpmc_ow, &val, 1);
    }

  assert_int_equal(rt_count(&g_mpmc_ow), 8);

  /* Overwrite: push one more, oldest (0) discarded */

  val = 100;
  n = rt_mpmc_overwrite(&g_mpmc_ow, &val, 1);
  assert_int_equal(n, 1);
  assert_int_equal(rt_count(&g_mpmc_ow), 8);

  n = rt_mpmc_dequeue(&g_mpmc_ow, out, 8);
  assert_int_equal(n, 8);
  for (i = 0; i < 7; i++)
    {
      assert_int_equal(out[i], i + 1);
    }

  assert_int_equal(out[7], 100);
}

static void test_mpmc_overwrite_wrap(FAR void **state)
{
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow_wrap);

  /* Advance indices to force wraparound */

  for (i = 0; i < 6; i++)
    {
      val = i;
      rt_mpmc_overwrite(&g_mpmc_ow_wrap, &val, 1);
    }

  rt_mpmc_dequeue(&g_mpmc_ow_wrap, out, 6);

  /* Fill ring (indices start at 6, wraps at 8) */

  for (i = 0; i < 8; i++)
    {
      val = i + 10;
      rt_mpmc_overwrite(&g_mpmc_ow_wrap, &val, 1);
    }

  /* Overwrite 3 more: discards 10,11,12 */

  for (i = 0; i < 3; i++)
    {
      val = i + 200;
      rt_mpmc_overwrite(&g_mpmc_ow_wrap, &val, 1);
    }

  assert_int_equal(rt_count(&g_mpmc_ow_wrap), 8);

  n = rt_mpmc_dequeue(&g_mpmc_ow_wrap, out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 5; i++)
    {
      assert_int_equal(out[i], i + 13);
    }

  for (i = 0; i < 3; i++)
    {
      assert_int_equal(out[i + 5], i + 200);
    }
}

static void test_mpmc_overwrite_batch(FAR void **state)
{
  uint32_t in[4];
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow_batch);

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_mpmc_overwrite(&g_mpmc_ow_batch, &val, 1);
    }

  /* Batch overwrite 4 items: discards 0,1,2,3 */

  for (i = 0; i < 4; i++)
    {
      in[i] = i + 100;
    }

  n = rt_mpmc_overwrite(&g_mpmc_ow_batch, in, 4);
  assert_int_equal(n, 4);
  assert_int_equal(rt_count(&g_mpmc_ow_batch), 8);

  n = rt_mpmc_dequeue(&g_mpmc_ow_batch, out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(out[i], i + 4);
    }

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(out[i + 4], i + 100);
    }
}

static void test_mpmc_overwrite_exceed_cap(FAR void **state)
{
  uint32_t in[16];
  uint32_t out[8];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow_cap);

  for (i = 0; i < 16; i++)
    {
      in[i] = i;
    }

  n = rt_mpmc_overwrite(&g_mpmc_ow_cap, in, 16);
  assert_int_equal(n, 8);
  assert_int_equal(rt_count(&g_mpmc_ow_cap), 8);

  n = rt_mpmc_dequeue(&g_mpmc_ow_cap, out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 8);
    }
}

static void test_mpmc_overwrite_not_full(FAR void **state)
{
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow);

  for (i = 0; i < 4; i++)
    {
      val = i;
      rt_mpmc_overwrite(&g_mpmc_ow, &val, 1);
    }

  val = 99;
  n = rt_mpmc_overwrite(&g_mpmc_ow, &val, 1);
  assert_int_equal(n, 0);
  assert_int_equal(rt_count(&g_mpmc_ow), 5);

  n = rt_mpmc_dequeue(&g_mpmc_ow, out, 5);
  assert_int_equal(n, 5);
  for (i = 0; i < 4; i++)
    {
      assert_int_equal(out[i], i);
    }

  assert_int_equal(out[4], 99);
}

/****************************************************************************
 * Private Functions - Dequeue skip tests
 ****************************************************************************/

static void test_mpmc_dequeue_skip_basic(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow);

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_mpmc_enqueue(&g_mpmc_ow, &val, 1);
    }

  n = rt_mpmc_skip(&g_mpmc_ow, 3);
  assert_int_equal(n, 3);
  assert_int_equal(rt_count(&g_mpmc_ow), 5);

  n = rt_mpmc_dequeue(&g_mpmc_ow, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 3);
}

static void test_mpmc_dequeue_skip_all(FAR void **state)
{
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow);

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_mpmc_enqueue(&g_mpmc_ow, &val, 1);
    }

  n = rt_mpmc_skip(&g_mpmc_ow, 8);
  assert_int_equal(n, 8);
  assert_int_equal(rt_count(&g_mpmc_ow), 0);
}

static void test_mpmc_dequeue_skip_more_than_avail(FAR void **state)
{
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_mpmc_ow);

  for (i = 0; i < 4; i++)
    {
      val = i;
      rt_mpmc_enqueue(&g_mpmc_ow, &val, 1);
    }

  n = rt_mpmc_skip(&g_mpmc_ow, 8);
  assert_int_equal(n, 4);
  assert_int_equal(rt_count(&g_mpmc_ow), 0);
}

static void test_mpmc_dequeue_skip_empty(FAR void **state)
{
  uint32_t n;

  ring_test_init(&g_mpmc_ow);

  n = rt_mpmc_skip(&g_mpmc_ow, 5);
  assert_int_equal(n, 0);
}

/****************************************************************************
 * Private Functions - Single-core multi-thread concurrent test
 *
 * MPMC: N producer threads + N consumer threads.
 * Uses pthread + sched_yield() for single-core safety.
 ****************************************************************************/

struct mpmc_prod_ctx_s
{
  FAR struct ring_test_s *ring;
  int items;
  atomic_t *start;
};

struct mpmc_cons_ctx_s
{
  FAR struct ring_test_s *ring;
  FAR atomic_t *consumed;
  int total;
  atomic_t *start;
};

static FAR void *mpmc_producer_thread(FAR void *arg)
{
  FAR struct mpmc_prod_ctx_s *ctx = (FAR struct mpmc_prod_ctx_s *)arg;
  uint32_t val;
  int sent = 0;

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  while (sent < ctx->items)
    {
      val = (uint32_t)sent;
      if (rt_mpmc_enqueue(ctx->ring, &val, 1) == 1)
        {
          sent++;
        }
      else
        {
          sched_yield();
        }
    }

  return NULL;
}

static FAR void *mpmc_consumer_thread(FAR void *arg)
{
  FAR struct mpmc_cons_ctx_s *ctx = (FAR struct mpmc_cons_ctx_s *)arg;
  uint32_t out = 0;

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  while (atomic_read(ctx->consumed) < ctx->total)
    {
      if (rt_mpmc_dequeue(ctx->ring, &out, 1) == 1)
        {
          atomic_add(ctx->consumed, 1);
        }
      else
        {
          sched_yield();
        }
    }

  return NULL;
}

static void test_mpmc_concurrent(FAR void **state)
{
  struct mpmc_prod_ctx_s pctx[RING_TEST_THREADS];
  struct mpmc_cons_ctx_s cctx[RING_TEST_THREADS];
  pthread_t prod_threads[RING_TEST_THREADS];
  pthread_t cons_threads[RING_TEST_THREADS];
  atomic_t consumed = 0;
  atomic_t start = 0;
  int total;
  int i;

  ring_test_init(&g_mpmc_conc);

  total = RING_TEST_ITEMS * RING_TEST_THREADS;

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pctx[i].ring = &g_mpmc_conc;
      pctx[i].items = RING_TEST_ITEMS;
      pctx[i].start = &start;
      pthread_create(&prod_threads[i], NULL,
                     mpmc_producer_thread, &pctx[i]);
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      cctx[i].ring = &g_mpmc_conc;
      cctx[i].consumed = &consumed;
      cctx[i].total = total;
      cctx[i].start = &start;
      pthread_create(&cons_threads[i], NULL,
                     mpmc_consumer_thread, &cctx[i]);
    }

  /* Signal all threads to start */

  atomic_set(&start, 1);

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(prod_threads[i], NULL);
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(cons_threads[i], NULL);
    }

  assert_int_equal(atomic_read(&consumed), total);
  assert_int_equal(rt_count(&g_mpmc_conc), 0);
}

/****************************************************************************
 * Private Functions - Overwrite concurrent test
 *
 * MPMC overwrite: N overwrite-producer threads + N consumer threads.
 * Producers continuously overwrite; consumers dequeue.
 * Invariant: consumed <= produced, ring empty at end.
 ****************************************************************************/

struct mpmc_ow_prod_ctx_s
{
  FAR struct ring_test_s    *ring;
  int                   items;
  FAR atomic_t         *start;
  FAR atomic_t         *produced;
};

struct mpmc_ow_cons_ctx_s
{
  FAR struct ring_test_s    *ring;
  FAR atomic_t         *start;
  FAR atomic_t         *consumed;
  FAR atomic_t         *prod_done;
};

static FAR void *mpmc_ow_producer_thread(FAR void *arg)
{
  FAR struct mpmc_ow_prod_ctx_s *ctx =
    (FAR struct mpmc_ow_prod_ctx_s *)arg;
  uint32_t val;
  int sent = 0;

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  while (sent < ctx->items)
    {
      val = (uint32_t)sent;
      rt_mpmc_overwrite(ctx->ring, &val, 1);
      atomic_add(ctx->produced, 1);
      sent++;
    }

  return NULL;
}

static FAR void *mpmc_ow_consumer_thread(FAR void *arg)
{
  FAR struct mpmc_ow_cons_ctx_s *ctx =
    (FAR struct mpmc_ow_cons_ctx_s *)arg;
  uint32_t out = 0;

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  for (; ; )
    {
      if (rt_mpmc_dequeue(ctx->ring, &out, 1) == 1)
        {
          atomic_add(ctx->consumed, 1);
        }
      else if (atomic_read(ctx->prod_done) ==
               RING_TEST_THREADS)
        {
          if (rt_mpmc_dequeue(ctx->ring, &out, 1) == 0)
            {
              break;
            }

          atomic_add(ctx->consumed, 1);
        }
      else
        {
          sched_yield();
        }
    }

  return NULL;
}

static void test_mpmc_overwrite_concurrent(FAR void **state)
{
  struct mpmc_ow_prod_ctx_s pctx[RING_TEST_THREADS];
  struct mpmc_ow_cons_ctx_s cctx[RING_TEST_THREADS];
  pthread_t prod_threads[RING_TEST_THREADS];
  pthread_t cons_threads[RING_TEST_THREADS];
  atomic_t produced = 0;
  atomic_t consumed = 0;
  atomic_t prod_done = 0;
  atomic_t start = 0;
  int i;

  ring_test_init(&g_mpmc_ow_conc);

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pctx[i].ring = &g_mpmc_ow_conc;
      pctx[i].items = RING_TEST_ITEMS;
      pctx[i].start = &start;
      pctx[i].produced = &produced;
      pthread_create(&prod_threads[i], NULL,
                     mpmc_ow_producer_thread, &pctx[i]);
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      cctx[i].ring = &g_mpmc_ow_conc;
      cctx[i].start = &start;
      cctx[i].consumed = &consumed;
      cctx[i].prod_done = &prod_done;
      pthread_create(&cons_threads[i], NULL,
                     mpmc_ow_consumer_thread, &cctx[i]);
    }

  atomic_set(&start, 1);

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(prod_threads[i], NULL);
      atomic_add(&prod_done, 1);
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(cons_threads[i], NULL);
    }

  assert_true(atomic_read(&consumed) <= atomic_read(&produced));
  assert_int_equal(rt_count(&g_mpmc_ow_conc), 0);
}

/****************************************************************************
 * Private Functions - Skip concurrent test
 *
 * MPMC skip: N producer threads + N skip-consumer threads.
 * Producers enqueue normally; consumers use skip.
 * Invariant: total_skipped == total_produced, ring empty at end.
 ****************************************************************************/

struct mpmc_skip_cons_ctx_s
{
  FAR struct ring_test_s    *ring;
  FAR atomic_t         *start;
  FAR atomic_t         *skipped;
  FAR atomic_t         *prod_done;
};

static FAR void *mpmc_skip_consumer_thread(FAR void *arg)
{
  FAR struct mpmc_skip_cons_ctx_s *ctx =
    (FAR struct mpmc_skip_cons_ctx_s *)arg;
  uint32_t n;

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  for (; ; )
    {
      n = rt_mpmc_skip(ctx->ring, 1);
      if (n > 0)
        {
          atomic_add(ctx->skipped, n);
        }
      else if (atomic_read(ctx->prod_done) ==
               RING_TEST_THREADS)
        {
          n = rt_mpmc_skip(ctx->ring, 1);
          if (n == 0)
            {
              break;
            }

          atomic_add(ctx->skipped, n);
        }
      else
        {
          sched_yield();
        }
    }

  return NULL;
}

static void test_mpmc_skip_concurrent(FAR void **state)
{
  struct mpmc_prod_ctx_s pctx[RING_TEST_THREADS];
  struct mpmc_skip_cons_ctx_s cctx[RING_TEST_THREADS];
  pthread_t prod_threads[RING_TEST_THREADS];
  pthread_t cons_threads[RING_TEST_THREADS];
  atomic_t skipped = 0;
  atomic_t prod_done = 0;
  atomic_t start = 0;
  int total;
  int i;

  ring_test_init(&g_mpmc_skip_conc);

  total = RING_TEST_ITEMS * RING_TEST_THREADS;

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pctx[i].ring = &g_mpmc_skip_conc;
      pctx[i].items = RING_TEST_ITEMS;
      pctx[i].start = &start;
      pthread_create(&prod_threads[i], NULL,
                     mpmc_producer_thread, &pctx[i]);
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      cctx[i].ring = &g_mpmc_skip_conc;
      cctx[i].start = &start;
      cctx[i].skipped = &skipped;
      cctx[i].prod_done = &prod_done;
      pthread_create(&cons_threads[i], NULL,
                     mpmc_skip_consumer_thread, &cctx[i]);
    }

  atomic_set(&start, 1);

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(prod_threads[i], NULL);
      atomic_add(&prod_done, 1);
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(cons_threads[i], NULL);
    }

  assert_int_equal(atomic_read(&skipped), total);
  assert_int_equal(rt_count(&g_mpmc_skip_conc), 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static const struct CMUnitTest g_mpmc_local_tests[] =
{
  cmocka_unit_test(test_mpmc_basic),
  cmocka_unit_test(test_mpmc_batch),
  cmocka_unit_test(test_mpmc_full),
  cmocka_unit_test(test_mpmc_typed_api),
  cmocka_unit_test(test_mpmc_alloc_free),
  cmocka_unit_test(test_mpmc_enqueue_dequeue_boundary),
  cmocka_unit_test(test_mpmc_partial_batch),
  cmocka_unit_test(test_mpmc_wraparound_batch),
  cmocka_unit_test(test_mpmc_capacity_two),
  cmocka_unit_test(test_mpmc_count_boundary),
  cmocka_unit_test(test_mpmc_concurrent),
  cmocka_unit_test(test_mpmc_overwrite_basic),
  cmocka_unit_test(test_mpmc_overwrite_wrap),
  cmocka_unit_test(test_mpmc_overwrite_batch),
  cmocka_unit_test(test_mpmc_overwrite_exceed_cap),
  cmocka_unit_test(test_mpmc_overwrite_not_full),
  cmocka_unit_test(test_mpmc_dequeue_skip_basic),
  cmocka_unit_test(test_mpmc_dequeue_skip_all),
  cmocka_unit_test(test_mpmc_dequeue_skip_more_than_avail),
  cmocka_unit_test(test_mpmc_dequeue_skip_empty),
  cmocka_unit_test(test_mpmc_overwrite_concurrent),
  cmocka_unit_test(test_mpmc_skip_concurrent),
};

int ring_test_mpmc_local(void)
{
  return cmocka_run_group_tests(g_mpmc_local_tests, NULL, NULL);
}
