/****************************************************************************
 * apps/testing/libc/ring/ring_spmc.c
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

RING_TEST_DEFINE(g_spmc_basic, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spmc_batch, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spmc_full, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spmc_typed, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spmc_boundary, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spmc_partial, uint32_t, 8);
RING_TEST_DEFINE(g_spmc_wrap, uint32_t, 8);
RING_TEST_DEFINE(g_spmc_cap2, uint32_t, 2);
RING_TEST_DEFINE(g_spmc_count, uint32_t, 8);
RING_TEST_DEFINE(g_spmc_concurrent, uint32_t, RING_TEST_CAP);

/****************************************************************************
 * Private Functions - Single-core functional test
 ****************************************************************************/

static void test_spmc_basic(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spmc_basic);

  for (i = 0; i < 16; i++)
    {
      val = i + 400;
      n = rt_spmc_enqueue(&g_spmc_basic, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(&g_spmc_basic), 16);

  for (i = 0; i < 16; i++)
    {
      n = rt_spmc_dequeue(&g_spmc_basic, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 400);
    }
}

static void test_spmc_batch(FAR void **state)
{
  uint32_t buf[RING_TEST_BATCH];
  uint32_t out[RING_TEST_BATCH];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spmc_batch);

  for (i = 0; i < RING_TEST_BATCH; i++)
    {
      buf[i] = i + 200;
    }

  n = rt_spmc_enqueue(&g_spmc_batch, buf, RING_TEST_BATCH);
  assert_int_equal(n, RING_TEST_BATCH);
  n = rt_spmc_dequeue(&g_spmc_batch, out, RING_TEST_BATCH);
  assert_int_equal(n, RING_TEST_BATCH);

  for (i = 0; i < RING_TEST_BATCH; i++)
    {
      assert_int_equal(out[i], i + 200);
    }
}

static void test_spmc_full(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spmc_full);

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      val = i;
      n = rt_spmc_enqueue(&g_spmc_full, &val, 1);
      assert_int_equal(n, 1);
    }

  val = 999;
  n = rt_spmc_enqueue(&g_spmc_full, &val, 1);
  assert_int_equal(n, 0);

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      n = rt_spmc_dequeue(&g_spmc_full, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i);
    }

  assert_int_equal(rt_count(&g_spmc_full), 0);
}

static void test_spmc_typed_api(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t batch[4];
  uint32_t obatch[4];
  uint32_t n;
  uint32_t i;

  ring_test_init(&g_spmc_typed);

  val = 42;
  n = rt_spmc_enqueue(&g_spmc_typed, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spmc_dequeue(&g_spmc_typed, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 42);

  for (i = 0; i < 4; i++)
    {
      batch[i] = i + 500;
    }

  n = rt_spmc_enqueue(&g_spmc_typed, batch, 4);
  assert_int_equal(n, 4);
  n = rt_spmc_dequeue(&g_spmc_typed, obatch, 4);
  assert_int_equal(n, 4);

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(obatch[i], i + 500);
    }
}

static void test_spmc_alloc_free(FAR void **state)
{
  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  assert_non_null(ring_test_alloc(r, RING_TEST_CAP, sizeof(uint32_t)));

  val = 77;
  n = rt_spmc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spmc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 77);

  ring_free(r->r);
}

/****************************************************************************
 * Private Functions - Boundary condition tests (SP enqueue, MC dequeue)
 ****************************************************************************/

static void test_spmc_enqueue_dequeue_boundary(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spmc_boundary);

  /* Dequeue from empty ring (MC CAS path) */

  n = rt_spmc_dequeue(&g_spmc_boundary, &out, 1);
  assert_int_equal(n, 0);

  /* Enqueue count=0 */

  val = 42;
  n = rt_spmc_enqueue(&g_spmc_boundary, &val, 0);
  assert_int_equal(n, 0);
  assert_int_equal(rt_count(&g_spmc_boundary), 0);

  /* Dequeue count=0 */

  n = rt_spmc_dequeue(&g_spmc_boundary, &out, 0);
  assert_int_equal(n, 0);

  /* Fill ring, then try one more */

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      val = i;
      n = rt_spmc_enqueue(&g_spmc_boundary, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(&g_spmc_boundary), RING_TEST_CAP);

  val = 999;
  n = rt_spmc_enqueue(&g_spmc_boundary, &val, 1);
  assert_int_equal(n, 0);
}

static void test_spmc_partial_batch(FAR void **state)
{
  uint32_t buf[16];
  uint32_t out[16];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spmc_partial);

  /* Batch enqueue larger than capacity */

  for (i = 0; i < 16; i++)
    {
      buf[i] = i + 1;
    }

  n = rt_spmc_enqueue(&g_spmc_partial, buf, 16);
  assert_int_equal(n, 8);

  /* Batch dequeue larger than available (MC CAS path) */

  n = rt_spmc_dequeue(&g_spmc_partial, out, 16);
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

  n = rt_spmc_enqueue(&g_spmc_partial, buf, 6);
  assert_int_equal(n, 6);

  for (i = 0; i < 4; i++)
    {
      buf[i] = i + 200;
    }

  n = rt_spmc_enqueue(&g_spmc_partial, buf, 4);
  assert_int_equal(n, 2);
  assert_int_equal(rt_count(&g_spmc_partial), 8);

  /* Partial dequeue (MC CAS path) */

  n = rt_spmc_dequeue(&g_spmc_partial, out, 5);
  assert_int_equal(n, 5);
  assert_int_equal(rt_count(&g_spmc_partial), 3);

  n = rt_spmc_dequeue(&g_spmc_partial, out, 3);
  assert_int_equal(n, 3);
  assert_int_equal(rt_count(&g_spmc_partial), 0);
}

static void test_spmc_wraparound_batch(FAR void **state)
{
  uint32_t buf[8];
  uint32_t out[8];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spmc_wrap);

  /* Advance indices to force wraparound */

  for (i = 0; i < 6; i++)
    {
      buf[i] = i;
    }

  n = rt_spmc_enqueue(&g_spmc_wrap, buf, 6);
  assert_int_equal(n, 6);
  n = rt_spmc_dequeue(&g_spmc_wrap, out, 6);
  assert_int_equal(n, 6);

  /* Enqueue 8 items wrapping around */

  for (i = 0; i < 8; i++)
    {
      buf[i] = i + 1000;
    }

  n = rt_spmc_enqueue(&g_spmc_wrap, buf, 8);
  assert_int_equal(n, 8);

  /* Dequeue wrapping around (MC CAS path) */

  memset(out, 0, sizeof(out));
  n = rt_spmc_dequeue(&g_spmc_wrap, out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 1000);
    }
}

static void test_spmc_capacity_two(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  ring_test_init(&g_spmc_cap2);

  assert_int_equal(rt_capacity(&g_spmc_cap2), 2);

  val = 10;
  n = rt_spmc_enqueue(&g_spmc_cap2, &val, 1);
  assert_int_equal(n, 1);

  val = 20;
  n = rt_spmc_enqueue(&g_spmc_cap2, &val, 1);
  assert_int_equal(n, 1);

  /* Full */

  val = 30;
  n = rt_spmc_enqueue(&g_spmc_cap2, &val, 1);
  assert_int_equal(n, 0);

  /* MC dequeue */

  n = rt_spmc_dequeue(&g_spmc_cap2, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 10);

  n = rt_spmc_dequeue(&g_spmc_cap2, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 20);

  /* Empty */

  n = rt_spmc_dequeue(&g_spmc_cap2, &out, 1);
  assert_int_equal(n, 0);
}

static void test_spmc_count_boundary(FAR void **state)
{
  uint32_t val;
  uint32_t out[8];
  uint32_t i;

  ring_test_init(&g_spmc_count);

  assert_int_equal(rt_count(&g_spmc_count), 0);

  for (i = 0; i < 3; i++)
    {
      val = i;
      rt_spmc_enqueue(&g_spmc_count, &val, 1);
    }

  assert_int_equal(rt_count(&g_spmc_count), 3);

  for (i = 3; i < 8; i++)
    {
      val = i;
      rt_spmc_enqueue(&g_spmc_count, &val, 1);
    }

  assert_int_equal(rt_count(&g_spmc_count), 8);

  rt_spmc_dequeue(&g_spmc_count, out, 1);
  assert_int_equal(rt_count(&g_spmc_count), 7);

  rt_spmc_dequeue(&g_spmc_count, out, 7);
  assert_int_equal(rt_count(&g_spmc_count), 0);
}

/****************************************************************************
 * Private Functions - Single-core multi-thread concurrent test
 *
 * SPMC: main thread as single producer + multiple consumer threads.
 * Uses pthread + sched_yield() for single-core safety.
 ****************************************************************************/

struct spmc_thread_ctx_s
{
  FAR struct ring_test_s *ring;
  FAR atomic_t *consumed;
  int total;
  atomic_t *start;
};

static FAR void *spmc_consumer_thread(FAR void *arg)
{
  FAR struct spmc_thread_ctx_s *ctx = (FAR struct spmc_thread_ctx_s *)arg;
  uint32_t out = 0;

  /* Wait for start signal */

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  while (atomic_read(ctx->consumed) < ctx->total)
    {
      if (rt_spmc_dequeue(ctx->ring, &out, 1) == 1)
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

static void test_spmc_concurrent(FAR void **state)
{
  struct spmc_thread_ctx_s ctx[RING_TEST_THREADS];
  pthread_t threads[RING_TEST_THREADS];
  atomic_t consumed = 0;
  atomic_t start = 0;
  uint32_t val;
  int total;
  int sent = 0;
  int i;

  ring_test_init(&g_spmc_concurrent);

  total = RING_TEST_ITEMS;

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      ctx[i].ring = &g_spmc_concurrent;
      ctx[i].consumed = &consumed;
      ctx[i].total = total;
      ctx[i].start = &start;
      pthread_create(&threads[i], NULL, spmc_consumer_thread, &ctx[i]);
    }

  /* Signal all consumers to start */

  atomic_set(&start, 1);

  /* Main thread: single producer */

  while (sent < total)
    {
      val = (uint32_t)sent;
      if (rt_spmc_enqueue(&g_spmc_concurrent, &val, 1) == 1)
        {
          sent++;
        }
      else
        {
          sched_yield();
        }
    }

  for (i = 0; i < RING_TEST_THREADS; i++)
    {
      pthread_join(threads[i], NULL);
    }

  assert_int_equal(atomic_read(&consumed), total);
  assert_int_equal(rt_count(&g_spmc_concurrent), 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static const struct CMUnitTest g_spmc_local_tests[] =
{
  cmocka_unit_test(test_spmc_basic),
  cmocka_unit_test(test_spmc_batch),
  cmocka_unit_test(test_spmc_full),
  cmocka_unit_test(test_spmc_typed_api),
  cmocka_unit_test(test_spmc_alloc_free),
  cmocka_unit_test(test_spmc_enqueue_dequeue_boundary),
  cmocka_unit_test(test_spmc_partial_batch),
  cmocka_unit_test(test_spmc_wraparound_batch),
  cmocka_unit_test(test_spmc_capacity_two),
  cmocka_unit_test(test_spmc_count_boundary),
  cmocka_unit_test(test_spmc_concurrent),
};

int ring_test_spmc_local(void)
{
  return cmocka_run_group_tests(g_spmc_local_tests, NULL, NULL);
}
