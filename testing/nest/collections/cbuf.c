/****************************************************************************
 * apps/testing/nest/collections/cbuf.c
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

#include <testing/unity.h>

#include <nuttx/circbuf.h>

#include "tests.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: collections_cbuf__init_local
 *
 * Description:
 *   Tests that a circular buffer can be initialized with a local buffer.
 ****************************************************************************/

static void collections_cbuf__init_local(void)
{
  struct circbuf_s cbuf;
  uint8_t bbuf[16];
  TEST_ASSERT_EQUAL(0, circbuf_init(&cbuf, bbuf, sizeof(bbuf)));

  TEST_ASSERT_TRUE(circbuf_is_init(&cbuf));

  TEST_ASSERT_EQUAL(16, circbuf_size(&cbuf));
  TEST_ASSERT_EQUAL(0, circbuf_used(&cbuf));
  TEST_ASSERT_EQUAL(16, circbuf_space(&cbuf));

  circbuf_uninit(&cbuf);
  TEST_ASSERT_FALSE(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Name: collections_cbuf__init_malloc
 *
 * Description:
 *   Tests that a circular buffer can be initialized using an internally
 *   malloc'd buffer.
 ****************************************************************************/

static void collections_cbuf__init_malloc(void)
{
  struct circbuf_s cbuf;
  TEST_ASSERT_EQUAL(0, circbuf_init(&cbuf, NULL, 16));

  TEST_ASSERT_TRUE(circbuf_is_init(&cbuf));

  TEST_ASSERT_EQUAL(16, circbuf_size(&cbuf));
  TEST_ASSERT_EQUAL(0, circbuf_used(&cbuf));
  TEST_ASSERT_EQUAL(16, circbuf_space(&cbuf));

  circbuf_uninit(&cbuf);
  TEST_ASSERT_FALSE(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Name: collections_cbuf__empty_local
 *
 * Description:
 *   Tests that a circular buffer initialized with a local buffer is empty
 *   upon creation.
 ****************************************************************************/

static void collections_cbuf__empty_local(void)
{
  struct circbuf_s cbuf;
  uint8_t bbuf[16];
  TEST_ASSERT_EQUAL(0, circbuf_init(&cbuf, bbuf, sizeof(bbuf)));

  TEST_ASSERT_TRUE(circbuf_is_empty(&cbuf));
  TEST_ASSERT_FALSE(circbuf_is_full(&cbuf));
  TEST_ASSERT_EQUAL(0, circbuf_used(&cbuf));

  circbuf_uninit(&cbuf);
  TEST_ASSERT_FALSE(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Name: collections_cbuf__empty_malloc
 *
 * Description:
 *   Tests that a circular buffer initialized with a malloc'd buffer is empty
 *   upon creation.
 ****************************************************************************/

static void collections_cbuf__empty_malloc(void)
{
  struct circbuf_s cbuf;
  TEST_ASSERT_EQUAL(0, circbuf_init(&cbuf, NULL, 16));

  TEST_ASSERT_TRUE(circbuf_is_empty(&cbuf));
  TEST_ASSERT_FALSE(circbuf_is_full(&cbuf));
  TEST_ASSERT_EQUAL(0, circbuf_used(&cbuf));

  circbuf_uninit(&cbuf);
  TEST_ASSERT_FALSE(circbuf_is_init(&cbuf));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_collections_cbuf
 *
 * Description:
 *   Runs the test cases for the circular buffer collection.
 ****************************************************************************/

void nest_collections_cbuf(void)
{
  RUN_TEST(collections_cbuf__init_local);
  RUN_TEST(collections_cbuf__init_malloc);
  RUN_TEST(collections_cbuf__empty_local);
  RUN_TEST(collections_cbuf__empty_malloc);
}
