/****************************************************************************
 * apps/testing/nuts/dstructs/cbuf.c
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

#include <stdint.h>
#include <string.h>

#include <nuttx/circbuf.h>

#include "tests.h"

#ifdef CONFIG_TESTING_NUTS_DSTRUCTS_CBUF

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CBUF_SIZE (16)
#define CBUF_HALFSIZE (CBUF_SIZE / 2)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct circbuf_s g_cbuf;
static uint8_t g_buf[CBUF_SIZE];
static uint8_t g_popbuf[CBUF_SIZE] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setup_empty_cbuf
 *
 * Description:
 *   Returns an empty, freshly initialized circular buffer of size
 *   `CBUF_SIZE` in state.
 ****************************************************************************/

static int setup_empty_cbuf(void **state)
{
  *state = &g_cbuf;
  return circbuf_init(&g_cbuf, g_buf, sizeof(g_buf));
}

/****************************************************************************
 * Name: setup_partial_cbuf
 *
 * Description:
 *   Returns a circular buffer of size `CBUF_SIZE` which is not empty but is
 *   not full.
 ****************************************************************************/

static int setup_partial_cbuf(void **state)
{
  int err;
  *state = &g_cbuf;
  err = circbuf_init(&g_cbuf, g_buf, sizeof(g_buf));

  if (err)
    {
      return err;
    }

  if (circbuf_write(&g_cbuf, g_popbuf, CBUF_HALFSIZE) != CBUF_HALFSIZE)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: setup_full_cbuf
 *
 * Description:
 *   Returns a circular buffer which is full.
 ****************************************************************************/

static int setup_full_cbuf(void **state)
{
  int err;
  *state = &g_cbuf;
  err = circbuf_init(&g_cbuf, g_buf, sizeof(g_buf));

  if (err)
    {
      return err;
    }

  if (circbuf_write(&g_cbuf, g_popbuf, sizeof(g_popbuf)) != sizeof(g_popbuf))
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: teardown_cbuf
 *
 * Description:
 *   Uninitializes a circular buffer.
 ****************************************************************************/

static int teardown_cbuf(void **state)
{
  circbuf_uninit(*state);

  if (circbuf_is_init(*state))
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: init_static
 *
 * Description:
 *   Tests that a statically initialized circular buffer functions properly.
 ****************************************************************************/

static void init_static(void **state)
{
  UNUSED(state);
  uint8_t bbuf[CBUF_SIZE];
  struct circbuf_s cbuf = CIRCBUF_INITIALIZER(bbuf, sizeof(bbuf));

  assert_true(circbuf_is_init(&cbuf));

  assert_uint_equal(sizeof(bbuf), circbuf_size(&cbuf));
  assert_uint_equal(0, circbuf_used(&cbuf));
  assert_uint_equal(sizeof(bbuf), circbuf_space(&cbuf));

  circbuf_uninit(&cbuf);
  assert_false(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Name: init_local
 *
 * Description:
 *   Tests that a circular buffer can be initialized with a local buffer, and
 *   subsequently uninitialized.
 ****************************************************************************/

static void init_local(void **state)
{
  UNUSED(state);
  struct circbuf_s cbuf;
  uint8_t bbuf[CBUF_SIZE];
  assert_int_equal(0, circbuf_init(&cbuf, bbuf, sizeof(bbuf)));

  assert_true(circbuf_is_init(&cbuf));

  assert_uint_equal(sizeof(bbuf), circbuf_size(&cbuf));
  assert_uint_equal(0, circbuf_used(&cbuf));
  assert_uint_equal(sizeof(bbuf), circbuf_space(&cbuf));

  circbuf_uninit(&cbuf);
  assert_false(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Name: init_malloc
 *
 * Description:
 *   Tests that a circular buffer can be initialized using an internally
 *   malloc'd buffer and subsequently uninitialized.
 ****************************************************************************/

static void init_malloc(void **state)
{
  UNUSED(state);
  struct circbuf_s cbuf;
  assert_int_equal(0, circbuf_init(&cbuf, NULL, 16));

  assert_true(circbuf_is_init(&cbuf));

  assert_uint_equal(16, circbuf_size(&cbuf));
  assert_uint_equal(0, circbuf_used(&cbuf));
  assert_uint_equal(16, circbuf_space(&cbuf));

  circbuf_uninit(&cbuf);
  assert_false(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Name: empty_postinit
 *
 * Description:
 *   Tests that a circular buffer is empty right after being initialized.
 ****************************************************************************/

static void empty_postinit(void **state)
{
  struct circbuf_s *cbuf = *state;

  assert_true(circbuf_is_empty(cbuf));
  assert_false(circbuf_is_full(cbuf));
  assert_uint_equal(0, circbuf_used(cbuf));
  assert_uint_equal(CBUF_SIZE, circbuf_space(cbuf));
}

/****************************************************************************
 * Name: fail_peek_empty
 *
 * Description:
 *   Tests that circbuf_peek peeks zero bytes from an empty circbuf.
 ****************************************************************************/

static void fail_peek_empty(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf;
  assert_int_equal(0, circbuf_peek(cbuf, &buf, sizeof(buf)));
}

/****************************************************************************
 * Name: peekat_empty
 *
 * Description:
 *   Tests that circbuf_peekat peeks zero bytes from an empty circbuf.
 ****************************************************************************/

static void fail_peekat_empty(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf;
  assert_int_equal(0, circbuf_peekat(cbuf, 0, &buf, sizeof(buf)));
}

/****************************************************************************
 * Name: fail_read_empty
 *
 * Description:
 *   Tests that circbuf_read peeks zero bytes from an empty circbuf.
 ****************************************************************************/

static void fail_read_empty(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf;
  assert_int_equal(0, circbuf_read(cbuf, &buf, sizeof(buf)));
}

/****************************************************************************
 * Name: fail_skip_empty
 *
 * Description:
 *   Tests that circbuf_skip can only skip zero bytes from an empty circbuf.
 ****************************************************************************/

static void fail_skip_empty(void **state)
{
  struct circbuf_s *cbuf = *state;
  assert_int_equal(0, circbuf_skip(cbuf, 1));
  assert_int_equal(0, circbuf_skip(cbuf, 10));
  assert_int_equal(0, circbuf_skip(cbuf, CBUF_SIZE + 1));
}

/****************************************************************************
 * Name: empty_equal_pointers
 *
 * Description:
 *   Tests that an empty circular buffer has the same value for the read and
 *   write pointer.
 ****************************************************************************/

static void empty_equal_pointers(void **state)
{
  struct circbuf_s *cbuf = *state;
  size_t sz;
  void *rptr;
  void *wptr;

  rptr = circbuf_get_readptr(cbuf, &sz);
  assert_non_null(rptr);
  assert_uint_equal(0, sz);

  wptr = circbuf_get_writeptr(cbuf, &sz);
  assert_non_null(wptr);
  assert_uint_equal(CBUF_SIZE, sz);
  assert_ptr_equal(rptr, wptr);
}

/****************************************************************************
 * Name: partial_init
 *
 * Description:
 *   Tests that a circbuf partially initialized with some data has the
 *   correct attributes.
 ****************************************************************************/

static void partial_init(void **state)
{
  struct circbuf_s *cbuf = *state;
  assert_true(circbuf_is_init(cbuf));
  assert_false(circbuf_is_empty(cbuf));
  assert_false(circbuf_is_full(cbuf));
  assert_uint_equal(CBUF_SIZE, circbuf_size(cbuf));
  assert_uint_equal(CBUF_HALFSIZE, circbuf_used(cbuf));
  assert_uint_not_equal(0, circbuf_space(cbuf));
  assert_uint_not_equal(0, circbuf_used(cbuf));
}

/****************************************************************************
 * Name: partial_not_equal_pointers
 *
 * Description:
 *   Tests that a partially full circular buffer has different read and write
 *   pointers.
 ****************************************************************************/

static void partial_not_equal_pointers(void **state)
{
  struct circbuf_s *cbuf = *state;
  size_t sz;
  void *rptr;
  void *wptr;

  rptr = circbuf_get_readptr(cbuf, &sz);
  assert_non_null(rptr);
  assert_uint_not_equal(0, sz);

  wptr = circbuf_get_writeptr(cbuf, &sz);
  assert_non_null(wptr);
  assert_uint_not_equal(0, sz);
  assert_ptr_not_equal(rptr, wptr);
}

/****************************************************************************
 * Name: read_correct_contents
 *
 * Description:
 *   Tests that the contents read from the cbuf match the written contents.
 ****************************************************************************/

static void read_correct_contents(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_HALFSIZE];
  assert_uint_equal(sizeof(buf), circbuf_read(cbuf, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf, buf, sizeof(buf));
  assert_true(circbuf_is_empty(cbuf));
}

/****************************************************************************
 * Name: peek_correct_contents
 *
 * Description:
 *   Tests that the contents peeked from the cbuf match the written contents.
 ****************************************************************************/

static void peek_correct_contents(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_HALFSIZE];
  assert_uint_equal(sizeof(buf), circbuf_peek(cbuf, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf, buf, sizeof(buf));
  assert_false(circbuf_is_empty(cbuf));
}

/****************************************************************************
 * Name: peekat_correct_contents
 *
 * Description:
 *   Tests that the contents peeked from a position in the cbuf match the
 *   written contents.
 ****************************************************************************/

static void peekat_correct_contents(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_HALFSIZE];
  assert_uint_equal(sizeof(buf), circbuf_peekat(cbuf, 0, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf, buf, sizeof(buf));

  memset(buf, 0, sizeof(buf));
  assert_uint_equal(1, circbuf_peekat(cbuf, 0, buf, 1));
  assert_uint_equal(0, buf[0]);

  assert_uint_equal(1, circbuf_peekat(cbuf, 5, buf, 1));
  assert_uint_equal(5, buf[0]);
  assert_false(circbuf_is_empty(cbuf));
}

/****************************************************************************
 * Name: skip_correct_contents
 *
 * Description:
 *   Tests that the contents read from a cbuf after skipping some data is
 *   correct.
 ****************************************************************************/

static void skip_correct_contents(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_HALFSIZE];

  assert_uint_equal(3, circbuf_skip(cbuf, 3));
  assert_uint_equal(CBUF_HALFSIZE - 3, circbuf_read(cbuf, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf + 3, buf, CBUF_HALFSIZE - 3);
  assert_true(circbuf_is_empty(cbuf));
}

/****************************************************************************
 * Name: write_within_bounds
 *
 * Description:
 *   Tests that the circular buffer behaves properly when more data is
 *   written but the size of the backing buffer is not exceeded.
 ****************************************************************************/

static void write_within_bounds(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_HALFSIZE + 2];
  buf[0] = 8;
  buf[1] = 9;

  /* Write in the new contents */

  assert_uint_equal(CBUF_HALFSIZE, circbuf_used(cbuf));
  assert_uint_equal(2, circbuf_write(cbuf, buf, 2));
  assert_uint_equal(CBUF_HALFSIZE + 2, circbuf_used(cbuf));

  /* Check correct contents */

  assert_uint_equal(CBUF_HALFSIZE + 2, circbuf_read(cbuf, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf, buf, CBUF_HALFSIZE);
  assert_uint_equal(buf[8], 8);
  assert_uint_equal(buf[9], 9);
}

/****************************************************************************
 * Name: write_writeptr
 *
 * Description:
 *   Tests that the circular buffer behaves properly when more data is
 *   written to it using the write pointer and writecommit operation.
 ****************************************************************************/

static void write_writeptr(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_HALFSIZE + 2];
  buf[0] = 8;
  buf[1] = 9;
  uint8_t *wptr;
  size_t sz;

  wptr = circbuf_get_writeptr(cbuf, &sz);
  assert_non_null(wptr);
  assert_true(sz >= 2);

  /* Write in the new contents */

  memcpy(wptr, buf, 2);

  /* Check that the changes are not in effect */

  assert_uint_equal(CBUF_HALFSIZE, circbuf_used(cbuf));
  assert_uint_equal(CBUF_HALFSIZE, circbuf_space(cbuf));

  /* Check that the changes are in effect after commit */

  circbuf_writecommit(cbuf, 2);
  assert_uint_equal(CBUF_HALFSIZE + 2, circbuf_used(cbuf));
  assert_uint_equal(CBUF_HALFSIZE - 2, circbuf_space(cbuf));

  /* Check correct contents */

  assert_uint_equal(CBUF_HALFSIZE + 2, circbuf_read(cbuf, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf, buf, CBUF_HALFSIZE);
  assert_uint_equal(buf[8], 8);
  assert_uint_equal(buf[9], 9);
}

/****************************************************************************
 * Name: write_out_of_bounds
 *
 * Description:
 *   Tests that the circular buffer behaves properly when more data is
 *   written but the written data exceeds the backing buffer size.
 ****************************************************************************/

static void write_out_of_bounds(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_SIZE];

  /* Write in the new contents */

  assert_uint_equal(CBUF_HALFSIZE, circbuf_used(cbuf));
  assert_uint_equal(CBUF_HALFSIZE,
                    circbuf_write(cbuf, g_popbuf, sizeof(g_popbuf)));
  assert_uint_equal(CBUF_SIZE, circbuf_used(cbuf));

  /* Check correct contents */

  assert_uint_equal(CBUF_SIZE, circbuf_read(cbuf, buf, sizeof(buf)));
  assert_memory_equal(g_popbuf, buf, CBUF_HALFSIZE);
  assert_memory_equal(g_popbuf, buf + CBUF_HALFSIZE, CBUF_HALFSIZE);
}

/****************************************************************************
 * Name: overwrite_out_of_bounds
 *
 * Description:
 *   Tests that the circular buffer behaves properly when more data is
 *   written but the written data exceeds the backing buffer size and must
 *   overwrite.
 ****************************************************************************/

static void overwrite_out_of_bounds(void **state)
{
  struct circbuf_s *cbuf = *state;
  uint8_t buf[CBUF_SIZE];

  /* Write in the new contents */

  assert_uint_equal(CBUF_HALFSIZE, circbuf_used(cbuf));
  assert_uint_equal(CBUF_SIZE,
                    circbuf_overwrite(cbuf, g_popbuf, sizeof(g_popbuf)));
  assert_uint_equal(CBUF_SIZE, circbuf_used(cbuf));

  /* Check correct contents */

  assert_uint_equal(CBUF_SIZE, circbuf_read(cbuf, buf, sizeof(buf)));
  assert_memory_equal(buf, g_popbuf + CBUF_HALFSIZE, CBUF_HALFSIZE);
  assert_memory_equal(buf + CBUF_HALFSIZE, g_popbuf, CBUF_HALFSIZE);
}

/****************************************************************************
 * Name: reset_partial
 *
 * Description:
 *   Tests that when the partially filled circular buffer is reset, it
 *   behaves like a freshly initialized, empty circular buffer.
 ****************************************************************************/

static void reset_partial(void **state)
{
  struct circbuf_s *cbuf = *state;

  assert_false(circbuf_is_empty(cbuf));
  assert_false(circbuf_is_full(cbuf));
  assert_uint_not_equal(0, circbuf_used(cbuf));
  assert_uint_not_equal(0, circbuf_space(cbuf));

  circbuf_reset(cbuf);

  assert_true(circbuf_is_empty(cbuf));
  assert_false(circbuf_is_full(cbuf));
  assert_uint_equal(0, circbuf_used(cbuf));
  assert_uint_equal(CBUF_SIZE, circbuf_space(cbuf));
}

/****************************************************************************
 * Name: full_init
 *
 * Description:
 *   Tests that the full circular buffer has the correct attributes after
 *   initialization.
 ****************************************************************************/

static void full_init(void **state)
{
  struct circbuf_s *cbuf = *state;

  assert_false(circbuf_is_empty(cbuf));
  assert_true(circbuf_is_full(cbuf));
  assert_uint_equal(circbuf_size(cbuf), circbuf_used(cbuf));
  assert_uint_equal(0, circbuf_space(cbuf));
}

/****************************************************************************
 * Name: full_readptr
 *
 * Description:
 *   Tests that reading from the full circular buffer with a read pointer
 *   works as expected, and that the read commit operation behaves correctly.
 ****************************************************************************/

static void full_readptr(void **state)
{
  struct circbuf_s *cbuf = *state;
  size_t sz;
  uint8_t *rptr;

  rptr = circbuf_get_readptr(cbuf, &sz);
  assert_non_null(rptr);
  assert_uint_equal(CBUF_SIZE, sz);

  /* Verify contents */

  assert_memory_equal(g_popbuf, rptr, sizeof(g_popbuf));

  /* No changes to state before commit */

  assert_true(circbuf_is_full(cbuf));
  assert_false(circbuf_is_empty(cbuf));
  assert_uint_equal(0, circbuf_space(cbuf));
  assert_uint_equal(CBUF_SIZE, circbuf_used(cbuf));

  circbuf_readcommit(cbuf, sz);

  /* Proper attributes after read commit */

  assert_false(circbuf_is_full(cbuf));
  assert_true(circbuf_is_empty(cbuf));
  assert_uint_equal(0, circbuf_used(cbuf));
  assert_uint_equal(CBUF_SIZE, circbuf_space(cbuf));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nuts_dstructs_cbuf
 *
 * Description:
 *   Runs the test cases for the circular buffer collection.
 ****************************************************************************/

int nuts_dstructs_cbuf(void)
{
  static const struct CMUnitTest tests[] = {
      cmocka_unit_test(init_static),
      cmocka_unit_test(init_local),
      cmocka_unit_test(init_malloc),
      cmocka_unit_test_setup_teardown(empty_postinit, setup_empty_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(fail_peek_empty, setup_empty_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(fail_peekat_empty, setup_empty_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(fail_read_empty, setup_empty_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(fail_skip_empty, setup_empty_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(empty_equal_pointers, setup_empty_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(partial_init, setup_partial_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(partial_not_equal_pointers,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(read_correct_contents,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(peek_correct_contents,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(peekat_correct_contents,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(skip_correct_contents,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(write_within_bounds,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(write_writeptr, setup_partial_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(write_out_of_bounds,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(overwrite_out_of_bounds,
                                      setup_partial_cbuf, teardown_cbuf),
      cmocka_unit_test_setup_teardown(reset_partial, setup_partial_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(full_init, setup_full_cbuf,
                                      teardown_cbuf),
      cmocka_unit_test_setup_teardown(full_readptr, setup_full_cbuf,
                                      teardown_cbuf),
  };

  return cmocka_run_group_tests_name("circbuf_s", tests, NULL, NULL);
}

#endif /* CONFIG_TESTING_NUTS_DSTRUCTS_CBUF */
