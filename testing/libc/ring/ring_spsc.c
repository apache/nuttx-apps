/****************************************************************************
 * apps/testing/libc/ring/ring_spsc.c
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
 * Private Types
 ****************************************************************************/

struct large_elem
{
  uint32_t a;
  uint32_t b;
  uint64_t c;
  char     name[16];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Per-test rings (moved from stack to avoid overflow on embedded) */

RING_TEST_DEFINE(g_spsc_basic, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_batch, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_full, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_typed, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_boundary, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_partial, uint32_t, 8);
RING_TEST_DEFINE(g_spsc_wrap, uint32_t, 8);
RING_TEST_DEFINE(g_spsc_cap2, uint32_t, 2);
RING_TEST_DEFINE(g_spsc_large, struct large_elem, 8);
RING_TEST_DEFINE(g_spsc_count, uint32_t, 8);
RING_TEST_DEFINE(g_spsc_concurrent, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_init, uint32_t, RING_TEST_CAP);
RING_TEST_DEFINE(g_spsc_ow, uint32_t, 8);
RING_TEST_DEFINE(g_spsc_ow_wrap, uint32_t, 8);
RING_TEST_DEFINE(g_spsc_ow_batch, uint32_t, 8);
RING_TEST_DEFINE(g_spsc_ow_cap, uint32_t, 8);

/* Per-test large element arrays */

static struct large_elem g_large_batch_in[8];
static struct large_elem g_large_batch_out[8];

/****************************************************************************
 * Private Functions - Single-core functional tests
 ****************************************************************************/

static void test_spsc_basic(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_basic);

  assert_int_equal(rt_count(&g_spsc_basic), 0);
  assert_int_equal(rt_capacity(&g_spsc_basic), RING_TEST_CAP);
  n = rt_spsc_dequeue(&g_spsc_basic, &out, 1);
  assert_int_equal(n, 0);

  for (i = 0; i < 32; i++)
    {
      val = i + 100;
      n = rt_spsc_enqueue(&g_spsc_basic, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(&g_spsc_basic), 32);

  for (i = 0; i < 32; i++)
    {
      n = rt_spsc_dequeue(&g_spsc_basic, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 100);
    }

  assert_int_equal(rt_count(&g_spsc_basic), 0);
}

static void test_spsc_batch(FAR void **state)
{
  uint32_t buf[RING_TEST_BATCH];
  uint32_t out[RING_TEST_BATCH];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_batch);

  for (i = 0; i < RING_TEST_BATCH; i++)
    {
      buf[i] = i + 200;
    }

  n = rt_spsc_enqueue(&g_spsc_batch, buf, RING_TEST_BATCH);
  assert_int_equal(n, RING_TEST_BATCH);
  n = rt_spsc_dequeue(&g_spsc_batch, out, RING_TEST_BATCH);
  assert_int_equal(n, RING_TEST_BATCH);

  for (i = 0; i < RING_TEST_BATCH; i++)
    {
      assert_int_equal(out[i], i + 200);
    }
}

static void test_spsc_full(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_full);

  for (i = 0; i < RING_TEST_CAP; i++)
    {
      val = i;
      n = rt_spsc_enqueue(&g_spsc_full, &val, 1);
      assert_int_equal(n, 1);
    }

  val = 999;
  n = rt_spsc_enqueue(&g_spsc_full, &val, 1);
  assert_int_equal(n, 0);

  for (i = 0; i < RING_TEST_CAP / 2; i++)
    {
      n = rt_spsc_dequeue(&g_spsc_full, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i);
    }

  for (i = 0; i < RING_TEST_CAP / 2; i++)
    {
      val = i + 1000;
      n = rt_spsc_enqueue(&g_spsc_full, &val, 1);
      assert_int_equal(n, 1);
    }

  for (i = 0; i < RING_TEST_CAP / 2; i++)
    {
      n = rt_spsc_dequeue(&g_spsc_full, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + RING_TEST_CAP / 2);
    }

  for (i = 0; i < RING_TEST_CAP / 2; i++)
    {
      n = rt_spsc_dequeue(&g_spsc_full, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 1000);
    }

  assert_int_equal(rt_count(&g_spsc_full), 0);
}

static void test_spsc_typed_api(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t batch[4];
  uint32_t obatch[4];
  uint32_t n;
  uint32_t i;

  ring_test_init(&g_spsc_typed);

  val = 42;
  n = rt_spsc_enqueue(&g_spsc_typed, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(&g_spsc_typed, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 42);

  for (i = 0; i < 4; i++)
    {
      batch[i] = i + 500;
    }

  n = rt_spsc_enqueue(&g_spsc_typed, batch, 4);
  assert_int_equal(n, 4);
  n = rt_spsc_dequeue(&g_spsc_typed, obatch, 4);
  assert_int_equal(n, 4);

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(obatch[i], i + 500);
    }
}

static void test_spsc_alloc_free(FAR void **state)
{
  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  assert_non_null(ring_test_alloc(r, RING_TEST_CAP, sizeof(uint32_t)));

  val = 77;
  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 77);

  ring_free(r->r);
}

/****************************************************************************
 * Private Functions - ring_alloc tests
 ****************************************************************************/

static void test_ring_alloc_valid(FAR void **state)
{
  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  assert_non_null(ring_test_alloc(r, 16, sizeof(uint32_t)));
  assert_int_equal(rt_capacity(r), 16);
  assert_int_equal(rt_count(r), 0);

  val = 123;
  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 123);

  ring_free(r->r);
}

static void test_ring_alloc_invalid(FAR void **state)
{
  assert_null(ring_alloc(0));
}

static void test_ring_alloc_non_pow2(FAR void **state)
{
  static const uint32_t caps[] =
  {
    3, 5, 6, 7, 9, 10
  };

  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  uint32_t val;
  uint32_t out;
  uint32_t i;
  uint32_t j;

  for (i = 0; i < nitems(caps); i++)
    {
      assert_non_null(ring_test_alloc(r, caps[i], sizeof(uint32_t)));
      assert_int_equal(rt_capacity(r), caps[i]);

      /* Two full passes, so the indices wrap at least once */

      for (j = 0; j < 2 * caps[i]; j++)
        {
          val = j;
          assert_int_equal(rt_spsc_enqueue(r, &val, 1), 1);
          assert_int_equal(rt_spsc_dequeue(r, &out, 1), 1);
          assert_int_equal(out, j);
        }

      ring_free(r->r);
    }
}

/****************************************************************************
 * Private Functions - Enqueue/dequeue boundary tests
 ****************************************************************************/

static void test_enqueue_dequeue_boundary(FAR void **state)
{
  uint32_t out = 0;
  uint32_t n;

  ring_test_init(&g_spsc_boundary);

  /* Dequeue from empty ring */

  n = rt_spsc_dequeue(&g_spsc_boundary, &out, 1);
  assert_int_equal(n, 0);

  /* Enqueue count=0 */

  uint32_t val = 42;
  n = rt_spsc_enqueue(&g_spsc_boundary, &val, 0);
  assert_int_equal(n, 0);
  assert_int_equal(rt_count(&g_spsc_boundary), 0);

  /* Dequeue count=0 */

  n = rt_spsc_dequeue(&g_spsc_boundary, &out, 0);
  assert_int_equal(n, 0);

  /* Enqueue to full ring, then try one more */

  uint32_t i;
  for (i = 0; i < RING_TEST_CAP; i++)
    {
      val = i;
      n = rt_spsc_enqueue(&g_spsc_boundary, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(&g_spsc_boundary), RING_TEST_CAP);

  val = 999;
  n = rt_spsc_enqueue(&g_spsc_boundary, &val, 1);
  assert_int_equal(n, 0);
}

static void test_partial_batch(FAR void **state)
{
  uint32_t buf[16];
  uint32_t out[16];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_partial);

  /* Batch enqueue larger than capacity: should partial enqueue */

  for (i = 0; i < 16; i++)
    {
      buf[i] = i + 1;
    }

  n = rt_spsc_enqueue(&g_spsc_partial, buf, 16);
  assert_int_equal(n, 8);
  assert_int_equal(rt_count(&g_spsc_partial), 8);

  /* Batch dequeue larger than available: should partial dequeue */

  n = rt_spsc_dequeue(&g_spsc_partial, out, 16);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 1);
    }

  assert_int_equal(rt_count(&g_spsc_partial), 0);

  /* Partial enqueue when partially full */

  for (i = 0; i < 6; i++)
    {
      buf[i] = i + 100;
    }

  n = rt_spsc_enqueue(&g_spsc_partial, buf, 6);
  assert_int_equal(n, 6);

  /* Try to enqueue 4 more, only 2 should fit */

  for (i = 0; i < 4; i++)
    {
      buf[i] = i + 200;
    }

  n = rt_spsc_enqueue(&g_spsc_partial, buf, 4);
  assert_int_equal(n, 2);
  assert_int_equal(rt_count(&g_spsc_partial), 8);

  /* Partial dequeue: request 5, get 5 */

  n = rt_spsc_dequeue(&g_spsc_partial, out, 5);
  assert_int_equal(n, 5);
  assert_int_equal(rt_count(&g_spsc_partial), 3);
}

static void test_wraparound_batch(FAR void **state)
{
  uint32_t buf[8];
  uint32_t out[8];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_wrap);

  /* Fill ring to advance head/tail indices */

  for (i = 0; i < 6; i++)
    {
      buf[i] = i;
    }

  n = rt_spsc_enqueue(&g_spsc_wrap, buf, 6);
  assert_int_equal(n, 6);
  n = rt_spsc_dequeue(&g_spsc_wrap, out, 6);
  assert_int_equal(n, 6);

  /* Now head/tail are at index 6. Enqueue 8 items wraps around. */

  for (i = 0; i < 8; i++)
    {
      buf[i] = i + 1000;
    }

  n = rt_spsc_enqueue(&g_spsc_wrap, buf, 8);
  assert_int_equal(n, 8);

  memset(out, 0, sizeof(out));
  n = rt_spsc_dequeue(&g_spsc_wrap, out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 1000);
    }
}

static void test_capacity_two(FAR void **state)
{
  /* Smallest ring exercised: 2 items */

  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  ring_test_init(&g_spsc_cap2);

  assert_int_equal(rt_capacity(&g_spsc_cap2), 2);
  assert_int_equal(rt_count(&g_spsc_cap2), 0);

  val = 10;
  n = rt_spsc_enqueue(&g_spsc_cap2, &val, 1);
  assert_int_equal(n, 1);

  val = 20;
  n = rt_spsc_enqueue(&g_spsc_cap2, &val, 1);
  assert_int_equal(n, 1);

  /* Full */

  val = 30;
  n = rt_spsc_enqueue(&g_spsc_cap2, &val, 1);
  assert_int_equal(n, 0);

  n = rt_spsc_dequeue(&g_spsc_cap2, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 10);

  n = rt_spsc_dequeue(&g_spsc_cap2, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 20);

  /* Empty */

  n = rt_spsc_dequeue(&g_spsc_cap2, &out, 1);
  assert_int_equal(n, 0);
}

static void test_large_elem(FAR void **state)
{
  struct large_elem in;
  struct large_elem out;
  uint32_t n;
  uint32_t i;

  ring_test_init(&g_spsc_large);

  /* Single element */

  memset(&in, 0, sizeof(in));
  in.a = 1;
  in.b = 2;
  in.c = 0xdeadbeefcafe0000ull;
  strlcpy(in.name, "hello", sizeof(in.name));

  n = rt_spsc_enqueue(&g_spsc_large, &in, 1);
  assert_int_equal(n, 1);

  memset(&out, 0, sizeof(out));
  n = rt_spsc_dequeue(&g_spsc_large, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out.a, 1);
  assert_int_equal(out.b, 2);
  assert_true(out.c == 0xdeadbeefcafe0000ull);
  assert_string_equal(out.name, "hello");

  /* Batch with wraparound */

  /* Advance indices to force wraparound */

  for (i = 0; i < 5; i++)
    {
      memset(&in, 0, sizeof(in));
      in.a = i;
      rt_spsc_enqueue(&g_spsc_large, &in, 1);
    }

  rt_spsc_dequeue(&g_spsc_large, g_large_batch_out, 5);

  /* Now enqueue 8 items (wraps around) */

  for (i = 0; i < 8; i++)
    {
      memset(&g_large_batch_in[i], 0, sizeof(g_large_batch_in[i]));
      g_large_batch_in[i].a = i + 100;
      g_large_batch_in[i].b = i + 200;
      g_large_batch_in[i].c = (uint64_t)i * 1000;
    }

  n = rt_spsc_enqueue(&g_spsc_large, g_large_batch_in, 8);
  assert_int_equal(n, 8);

  memset(g_large_batch_out, 0, sizeof(g_large_batch_out));
  n = rt_spsc_dequeue(&g_spsc_large, g_large_batch_out, 8);
  assert_int_equal(n, 8);

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(g_large_batch_out[i].a, i + 100);
      assert_int_equal(g_large_batch_out[i].b, i + 200);
      assert_true(g_large_batch_out[i].c == (uint64_t)i * 1000);
    }
}

static void test_ring_count_boundary(FAR void **state)
{
  uint32_t val;
  uint32_t out[8];
  uint32_t i;

  ring_test_init(&g_spsc_count);

  /* Empty */

  assert_int_equal(rt_count(&g_spsc_count), 0);

  /* Partially filled */

  for (i = 0; i < 3; i++)
    {
      val = i;
      rt_spsc_enqueue(&g_spsc_count, &val, 1);
    }

  assert_int_equal(rt_count(&g_spsc_count), 3);

  /* Full */

  for (i = 3; i < 8; i++)
    {
      val = i;
      rt_spsc_enqueue(&g_spsc_count, &val, 1);
    }

  assert_int_equal(rt_count(&g_spsc_count), 8);

  /* Dequeue one: count should be 7 */

  rt_spsc_dequeue(&g_spsc_count, out, 1);
  assert_int_equal(rt_count(&g_spsc_count), 7);

  /* Drain all */

  rt_spsc_dequeue(&g_spsc_count, out, 7);
  assert_int_equal(rt_count(&g_spsc_count), 0);
}

static void test_ring_alloc_all_modes(FAR void **state)
{
  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  /* Test ring_alloc with MPSC mode */

  assert_non_null(ring_test_alloc(r, 8, sizeof(uint32_t)));

  val = 1;
  n = rt_mpsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 1);
  ring_free(r->r);

  /* Test ring_alloc with SPMC mode */

  assert_non_null(ring_test_alloc(r, 8, sizeof(uint32_t)));

  val = 2;
  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_mpmc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 2);
  ring_free(r->r);

  /* Test ring_alloc with MPMC mode */

  assert_non_null(ring_test_alloc(r, 8, sizeof(uint32_t)));

  val = 3;
  n = rt_mpsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_mpmc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 3);
  ring_free(r->r);
}

/****************************************************************************
 * Private Functions - init magic / warm-restart tests
 ****************************************************************************/

/* A reboot leaves the blob as it was and clears the handle, which lives
 * in ordinary RAM: forget the handle, then init again on the same blob.
 */

static void ring_test_reboot(FAR struct ring_test_s *t)
{
  memset(t->r, 0, sizeof(*t->r));
}

static void test_init_fresh(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  FAR struct ring_blob_s *blob = (FAR struct ring_blob_s *)r->blob;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;

  ring_test_init(r);

  assert_true(ring_is_ready(r->r));
  assert_int_equal(blob->magic, RING_BLOB_MAGIC_READY);
  assert_int_equal(rt_capacity(r), RING_TEST_CAP);
  assert_int_equal(rt_count(r), 0);

  val = 42;
  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 42);
}

static void test_init_warm_restart_recovery(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;
  uint32_t i;

  ring_test_init(r);
  for (i = 0; i < 16; i++)
    {
      val = i + 500;
      rt_spsc_enqueue(r, &val, 1);
    }

  for (i = 0; i < 8; i++)
    {
      rt_spsc_dequeue(r, &out, 1);
    }

  assert_int_equal(rt_count(r), 8);

  ring_test_reboot(r);
  ring_mpmc_init(r->r, r->blob, r->size, 0);

  assert_true(ring_is_ready(r->r));
  assert_int_equal(rt_count(r), 8);

  for (i = 0; i < 8; i++)
    {
      n = rt_spsc_dequeue(r, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 508);
    }

  assert_int_equal(rt_count(r), 0);
}

static void test_init_warm_restart_rewind(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;

  ring_test_init(r);
  for (i = 0; i < 16; i++)
    {
      val = i + 500;
      rt_spsc_enqueue(r, &val, 1);
    }

  for (i = 0; i < 8; i++)
    {
      rt_spsc_dequeue(r, &out, 1);
    }

  /* Replay the last 3 items consumed before the reboot */

  ring_test_reboot(r);
  ring_mpmc_init(r->r, r->blob, r->size, RT_BYTES(r, 3));

  assert_int_equal(rt_count(r), 11);

  for (i = 0; i < 11; i++)
    {
      assert_int_equal(rt_spsc_dequeue(r, &out, 1), 1);
      assert_int_equal(out, i + 505);
    }
}

static void test_init_warm_restart_corrupt(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  FAR struct ring_blob_s *blob = (FAR struct ring_blob_s *)r->blob;
  uint32_t val = 99;
  uint32_t out = 0;
  uint32_t n;

  /* READY blob whose indices claim more data than the ring can hold */

  ring_test_init(r);
  ring_test_reboot(r);
  atomic_set(&blob->prod_tail,
             RTS_PACK(0, RT_BYTES(r, RING_TEST_CAP + 10)));
  atomic_set(&blob->cons_tail, RTS_PACK(0, 0));

  ring_mpmc_init(r->r, r->blob, r->size, 0);

  assert_true(ring_is_ready(r->r));
  assert_int_equal(rt_count(r), 0);

  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 99);
}

static void test_init_warm_restart_cnt_reset(FAR void **state)
{
  /* A reboot in the middle of an MPMC enqueue leaves head.cnt !=
   * tail.cnt; unless init zeroes the counters, tail can never catch head
   * again and the next MP commit spins forever.
   */

  FAR struct ring_test_s *r = &g_spsc_init;
  FAR struct ring_blob_s *blob = (FAR struct ring_blob_s *)r->blob;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;
  uint32_t i;

  ring_test_init(r);
  ring_test_reboot(r);
  atomic_set(&blob->prod_head, RTS_PACK(3, RT_BYTES(r, 5)));
  atomic_set(&blob->prod_tail, RTS_PACK(1, RT_BYTES(r, 5)));
  atomic_set(&blob->cons_head, RTS_PACK(2, 0));
  atomic_set(&blob->cons_tail, RTS_PACK(0, 0));

  ring_mpmc_init(r->r, r->blob, r->size, 0);

  assert_int_equal(RTS_CNT(atomic_read(&blob->prod_head)), 0);
  assert_int_equal(RTS_CNT(atomic_read(&blob->prod_tail)), 0);
  assert_int_equal(RTS_CNT(atomic_read(&blob->cons_head)), 0);
  assert_int_equal(RTS_CNT(atomic_read(&blob->cons_tail)), 0);

  assert_int_equal(RTS_IDX(atomic_read(&blob->prod_tail)),
                   RT_BYTES(r, 5));
  assert_int_equal(RTS_IDX(atomic_read(&blob->cons_tail)), 0);
  assert_int_equal(rt_count(r), 5);

  for (i = 0; i < 4; i++)
    {
      val = i + 900;
      n = rt_mpmc_enqueue(r, &val, 1);
      assert_int_equal(n, 1);
    }

  assert_int_equal(rt_count(r), 9);

  rt_mpmc_skip(r, 5);

  for (i = 0; i < 4; i++)
    {
      n = rt_mpmc_dequeue(r, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 900);
    }

  assert_int_equal(rt_count(r), 0);
}

static void test_init_garbage_magic(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  FAR struct ring_blob_s *blob = (FAR struct ring_blob_s *)r->blob;

  ring_test_reboot(r);
  atomic_set(&blob->magic, (int32_t)0xdeadbeef);
  atomic_set(&blob->prod_tail, RTS_PACK(3, 100));
  atomic_set(&blob->cons_tail, RTS_PACK(1, 50));

  ring_mpmc_init(r->r, r->blob, r->size, 0);

  assert_int_equal(atomic_read(&blob->magic), RING_BLOB_MAGIC_READY);
  assert_int_equal(rt_count(r), 0);
}

static void test_init_geometry_mismatch(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  uint32_t val = 7;

  /* A READY blob left by a build with a different size is not reused */

  ring_test_init(r);
  rt_spsc_enqueue(r, &val, 1);

  ring_test_reboot(r);
  ring_mpmc_init(r->r, r->blob, r->size - RT_BYTES(r, 1), 0);

  assert_int_equal(rt_capacity(r), RING_TEST_CAP - 1);
  assert_int_equal(rt_count(r), 0);
}

static void test_init_double_call(FAR void **state)
{
  /* A second init in the same boot must not reset a ring in use */

  FAR struct ring_test_s *r = &g_spsc_init;
  uint32_t val = 77;
  uint32_t out = 0;
  uint32_t n;

  ring_test_init(r);
  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);

  assert_false(ring_mpmc_init(r->r, r->blob, r->size, 0));
  assert_int_equal(ring_spsc_init(r->r, r->blob, r->size, 0, false), OK);
  assert_int_equal(rt_count(r), 1);

  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 77);
}

static void test_init_spsc_slave(FAR void **state)
{
  FAR struct ring_test_s *r = &g_spsc_init;
  struct ring_s slave;
  uint32_t val = 55;
  uint32_t out = 0;

  memset(&slave, 0, sizeof(slave));
  memset(r->blob, 0, sizeof(struct ring_blob_s));
  memset(r->r, 0, sizeof(*r->r));

  /* The slave never initializes the blob, it waits for the master */

  assert_int_equal(ring_spsc_init(&slave, r->blob, r->size, 0, true),
                   -EAGAIN);
  assert_false(ring_is_ready(&slave));

  assert_int_equal(ring_spsc_init(r->r, r->blob, r->size, 0, false), OK);
  assert_int_equal(ring_spsc_init(&slave, r->blob, r->size - 1, 0, true),
                   -EINVAL);
  assert_int_equal(ring_spsc_init(&slave, r->blob, r->size, 0, true), OK);

  assert_int_equal(ring_spsc_enqueue(r->r, &val, sizeof(val)),
                   sizeof(val));
  assert_int_equal(ring_spsc_dequeue(&slave, &out, sizeof(out)),
                   sizeof(out));
  assert_int_equal(out, 55);
}

static void test_init_alloc_warm_restart(FAR void **state)
{
  struct ring_test_s t;
  FAR struct ring_test_s *r = &t;
  FAR struct ring_s *alloc;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;
  uint32_t i;

  alloc = ring_test_alloc(r, 16, sizeof(uint32_t));
  assert_non_null(alloc);

  for (i = 0; i < 10; i++)
    {
      val = i + 300;
      rt_spsc_enqueue(r, &val, 1);
    }

  for (i = 0; i < 4; i++)
    {
      rt_spsc_dequeue(r, &out, 1);
    }

  assert_int_equal(rt_count(r), 6);

  /* Rebind a fresh handle to the allocated blob, as after a reboot */

  memset(&r->handle, 0, sizeof(r->handle));
  r->r = &r->handle;
  ring_mpmc_init(r->r, alloc->blob, r->size, 0);

  assert_int_equal(rt_count(r), 6);

  for (i = 0; i < 6; i++)
    {
      n = rt_spsc_dequeue(r, &out, 1);
      assert_int_equal(n, 1);
      assert_int_equal(out, i + 304);
    }

  ring_free(alloc);
}

struct init_thread_ctx_s
{
  FAR struct ring_test_s *ring;
  FAR atomic_t *go;
  FAR atomic_t *first_count;
  FAR atomic_t *done_count;
};

static FAR void *init_thread_func(FAR void *arg)
{
  FAR struct init_thread_ctx_s *ctx =
    (FAR struct init_thread_ctx_s *)arg;
  struct ring_s handle;

  memset(&handle, 0, sizeof(handle));

  while (atomic_read(ctx->go) == 0)
    {
      sched_yield();
    }

  if (ring_mpmc_init(&handle, ctx->ring->blob, ctx->ring->size, 0))
    {
      atomic_add(ctx->first_count, 1);
    }

  if (ring_is_ready(&handle))
    {
      atomic_add(ctx->done_count, 1);
    }

  return NULL;
}

static void test_init_concurrent(FAR void **state)
{
  /* Each thread binds its own handle to the same cold blob: exactly one
   * of them may run the init body.
   */

  FAR struct ring_test_s *r = &g_spsc_init;
  struct init_thread_ctx_s ctx;
  pthread_t threads[4];
  atomic_t go = 0;
  atomic_t first_count = 0;
  atomic_t done_count = 0;
  uint32_t val;
  uint32_t out = 0;
  uint32_t n;
  int i;

  memset(r->blob, 0, sizeof(struct ring_blob_s));

  ctx.ring = r;
  ctx.go = &go;
  ctx.first_count = &first_count;
  ctx.done_count = &done_count;

  for (i = 0; i < 4; i++)
    {
      pthread_create(&threads[i], NULL, init_thread_func, &ctx);
    }

  atomic_set(&go, 1);

  for (i = 0; i < 4; i++)
    {
      pthread_join(threads[i], NULL);
    }

  assert_int_equal(atomic_read(&first_count), 1);
  assert_int_equal(atomic_read(&done_count), 4);

  ring_test_reboot(r);
  ring_mpmc_init(r->r, r->blob, r->size, 0);
  assert_int_equal(rt_count(r), 0);

  val = 123;
  n = rt_spsc_enqueue(r, &val, 1);
  assert_int_equal(n, 1);
  n = rt_spsc_dequeue(r, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 123);
}

/****************************************************************************
 * Private Functions - Overwrite enqueue tests
 ****************************************************************************/

static void test_spsc_overwrite_basic(FAR void **state)
{
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow);

  /* Fill ring to capacity */

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_spmc_overwrite(&g_spsc_ow, &val, 1);
    }

  assert_int_equal(rt_count(&g_spsc_ow), 8);

  /* Overwrite: push one more, oldest (0) should be discarded */

  val = 100;
  n = rt_spmc_overwrite(&g_spsc_ow, &val, 1);
  assert_int_equal(n, 1);
  assert_int_equal(rt_count(&g_spsc_ow), 8);

  /* Dequeue all: expect 1,2,3,4,5,6,7,100 */

  n = rt_spmc_dequeue(&g_spsc_ow, out, 8);
  assert_int_equal(n, 8);
  for (i = 0; i < 7; i++)
    {
      assert_int_equal(out[i], i + 1);
    }

  assert_int_equal(out[7], 100);
}

static void test_spsc_overwrite_wrap(FAR void **state)
{
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow_wrap);

  /* Advance indices to force wraparound */

  for (i = 0; i < 6; i++)
    {
      val = i;
      rt_spmc_overwrite(&g_spsc_ow_wrap, &val, 1);
    }

  rt_spmc_dequeue(&g_spsc_ow_wrap, out, 6);

  /* Now fill ring (indices start at 6, wraps at 8) */

  for (i = 0; i < 8; i++)
    {
      val = i + 10;
      rt_spmc_overwrite(&g_spsc_ow_wrap, &val, 1);
    }

  /* Overwrite 3 more: discards 10,11,12 */

  for (i = 0; i < 3; i++)
    {
      val = i + 200;
      rt_spmc_overwrite(&g_spsc_ow_wrap, &val, 1);
    }

  assert_int_equal(rt_count(&g_spsc_ow_wrap), 8);

  n = rt_spmc_dequeue(&g_spsc_ow_wrap, out, 8);
  assert_int_equal(n, 8);

  /* Expect: 13,14,15,16,17,200,201,202 */

  for (i = 0; i < 5; i++)
    {
      assert_int_equal(out[i], i + 13);
    }

  for (i = 0; i < 3; i++)
    {
      assert_int_equal(out[i + 5], i + 200);
    }
}

static void test_spsc_overwrite_batch(FAR void **state)
{
  uint32_t in[8];
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow_batch);

  /* Fill with 0..7 */

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_spmc_overwrite(&g_spsc_ow_batch, &val, 1);
    }

  /* Batch overwrite 4 items: discards 0,1,2,3 */

  for (i = 0; i < 4; i++)
    {
      in[i] = i + 100;
    }

  n = rt_spmc_overwrite(&g_spsc_ow_batch, in, 4);
  assert_int_equal(n, 4);
  assert_int_equal(rt_count(&g_spsc_ow_batch), 8);

  n = rt_spmc_dequeue(&g_spsc_ow_batch, out, 8);
  assert_int_equal(n, 8);

  /* Expect: 4,5,6,7,100,101,102,103 */

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(out[i], i + 4);
    }

  for (i = 0; i < 4; i++)
    {
      assert_int_equal(out[i + 4], i + 100);
    }
}

static void test_spsc_overwrite_exceed_cap(FAR void **state)
{
  uint32_t in[16];
  uint32_t out[8];
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow_cap);

  /* Enqueue 16 items into cap=8 ring: only last 8 should remain */

  for (i = 0; i < 16; i++)
    {
      in[i] = i;
    }

  n = rt_spmc_overwrite(&g_spsc_ow_cap, in, 16);
  assert_int_equal(n, 8);
  assert_int_equal(rt_count(&g_spsc_ow_cap), 8);

  n = rt_spmc_dequeue(&g_spsc_ow_cap, out, 8);
  assert_int_equal(n, 8);

  /* Expect: 8,9,10,11,12,13,14,15 */

  for (i = 0; i < 8; i++)
    {
      assert_int_equal(out[i], i + 8);
    }
}

static void test_spsc_overwrite_not_full(FAR void **state)
{
  uint32_t out[8];
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow);

  /* Partially fill (4 of 8) */

  for (i = 0; i < 4; i++)
    {
      val = i;
      rt_spmc_overwrite(&g_spsc_ow, &val, 1);
    }

  /* Overwrite enqueue when not full: should behave like normal */

  val = 99;
  n = rt_spmc_overwrite(&g_spsc_ow, &val, 1);
  assert_int_equal(n, 0);
  assert_int_equal(rt_count(&g_spsc_ow), 5);

  n = rt_spmc_dequeue(&g_spsc_ow, out, 5);
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

static void test_spsc_dequeue_skip_basic(FAR void **state)
{
  uint32_t val;
  uint32_t out = 0;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow);

  /* Fill with 0..7 */

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_spsc_enqueue(&g_spsc_ow, &val, 1);
    }

  /* Skip first 3 */

  n = rt_spsc_skip(&g_spsc_ow, 3);
  assert_int_equal(n, 3);
  assert_int_equal(rt_count(&g_spsc_ow), 5);

  /* Next dequeue should return 3 */

  n = rt_spsc_dequeue(&g_spsc_ow, &out, 1);
  assert_int_equal(n, 1);
  assert_int_equal(out, 3);
}

static void test_spsc_dequeue_skip_all(FAR void **state)
{
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow);

  for (i = 0; i < 8; i++)
    {
      val = i;
      rt_spsc_enqueue(&g_spsc_ow, &val, 1);
    }

  /* Skip all */

  n = rt_spsc_skip(&g_spsc_ow, 8);
  assert_int_equal(n, 8);
  assert_int_equal(rt_count(&g_spsc_ow), 0);
}

static void test_spsc_dequeue_skip_more_than_avail(FAR void **state)
{
  uint32_t val;
  uint32_t i;
  uint32_t n;

  ring_test_init(&g_spsc_ow);

  for (i = 0; i < 4; i++)
    {
      val = i;
      rt_spsc_enqueue(&g_spsc_ow, &val, 1);
    }

  /* Try to skip 8, only 4 available */

  n = rt_spsc_skip(&g_spsc_ow, 8);
  assert_int_equal(n, 4);
  assert_int_equal(rt_count(&g_spsc_ow), 0);
}

static void test_spsc_dequeue_skip_empty(FAR void **state)
{
  uint32_t n;

  ring_test_init(&g_spsc_ow);

  n = rt_spsc_skip(&g_spsc_ow, 5);
  assert_int_equal(n, 0);
}

/****************************************************************************
 * Private Functions - Single-core multi-thread concurrent test
 *
 * SPSC: 1 producer thread + main thread as consumer.
 * Uses pthread + sched_yield() for single-core safety.
 ****************************************************************************/

struct spsc_thread_ctx_s
{
  FAR struct ring_test_s *ring;
  int items;
  atomic_t *start;
};

static FAR void *spsc_producer_thread(FAR void *arg)
{
  FAR struct spsc_thread_ctx_s *ctx =
    (FAR struct spsc_thread_ctx_s *)arg;
  uint32_t val;
  int sent = 0;

  while (atomic_read(ctx->start) == 0)
    {
      sched_yield();
    }

  while (sent < ctx->items)
    {
      val = (uint32_t)sent;
      if (rt_spsc_enqueue(ctx->ring, &val, 1) == 1)
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

static void test_spsc_concurrent(FAR void **state)
{
  struct spsc_thread_ctx_s ctx;
  pthread_t thread;
  atomic_t start = 0;
  uint32_t out = 0;
  int received = 0;

  ring_test_init(&g_spsc_concurrent);

  ctx.ring = &g_spsc_concurrent;
  ctx.items = RING_TEST_ITEMS;
  ctx.start = &start;
  pthread_create(&thread, NULL, spsc_producer_thread, &ctx);

  /* Signal producer to start */

  atomic_set(&start, 1);

  /* Main thread: single consumer */

  while (received < RING_TEST_ITEMS)
    {
      if (rt_spsc_dequeue(&g_spsc_concurrent, &out, 1) == 1)
        {
          assert_int_equal(out, (uint32_t)received);
          received++;
        }
      else
        {
          sched_yield();
        }
    }

  pthread_join(thread, NULL);
  assert_int_equal(received, RING_TEST_ITEMS);
  assert_int_equal(rt_count(&g_spsc_concurrent), 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static const struct CMUnitTest g_spsc_local_tests[] =
{
  cmocka_unit_test(test_spsc_basic),
  cmocka_unit_test(test_spsc_batch),
  cmocka_unit_test(test_spsc_full),
  cmocka_unit_test(test_spsc_typed_api),
  cmocka_unit_test(test_spsc_alloc_free),
  cmocka_unit_test(test_ring_alloc_valid),
  cmocka_unit_test(test_ring_alloc_invalid),
  cmocka_unit_test(test_ring_alloc_all_modes),
  cmocka_unit_test(test_ring_alloc_non_pow2),
  cmocka_unit_test(test_enqueue_dequeue_boundary),
  cmocka_unit_test(test_partial_batch),
  cmocka_unit_test(test_wraparound_batch),
  cmocka_unit_test(test_capacity_two),
  cmocka_unit_test(test_large_elem),
  cmocka_unit_test(test_ring_count_boundary),
  cmocka_unit_test(test_init_fresh),
  cmocka_unit_test(test_init_warm_restart_recovery),
  cmocka_unit_test(test_init_warm_restart_rewind),
  cmocka_unit_test(test_init_warm_restart_corrupt),
  cmocka_unit_test(test_init_warm_restart_cnt_reset),
  cmocka_unit_test(test_init_garbage_magic),
  cmocka_unit_test(test_init_geometry_mismatch),
  cmocka_unit_test(test_init_double_call),
  cmocka_unit_test(test_init_spsc_slave),
  cmocka_unit_test(test_init_alloc_warm_restart),
  cmocka_unit_test(test_init_concurrent),
  cmocka_unit_test(test_spsc_concurrent),
  cmocka_unit_test(test_spsc_overwrite_basic),
  cmocka_unit_test(test_spsc_overwrite_wrap),
  cmocka_unit_test(test_spsc_overwrite_batch),
  cmocka_unit_test(test_spsc_overwrite_exceed_cap),
  cmocka_unit_test(test_spsc_overwrite_not_full),
  cmocka_unit_test(test_spsc_dequeue_skip_basic),
  cmocka_unit_test(test_spsc_dequeue_skip_all),
  cmocka_unit_test(test_spsc_dequeue_skip_more_than_avail),
  cmocka_unit_test(test_spsc_dequeue_skip_empty),
};

int ring_test_spsc_local(void)
{
  return cmocka_run_group_tests(g_spsc_local_tests, NULL, NULL);
}
