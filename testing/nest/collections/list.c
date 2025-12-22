/****************************************************************************
 * apps/testing/nest/collections/list.c
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

#include <testing/unity.h>

#include <nuttx/list.h>

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
 * Name: collections_list__empty_after_init
 *
 * Description:
 *   Tests that a newly initialized list is considered empty.
 ****************************************************************************/

static void collections_list__empty_after_init(void)
{
  struct list_node list;
  list_initialize(&list);
  TEST_ASSERT_TRUE(list_is_empty(&list));
}

/****************************************************************************
 * Name: collections_list__zerolen_after_init
 *
 * Description:
 *   Tests that a newly initialized list has zero length.
 ****************************************************************************/

static void collections_list__zerolen_after_init(void)
{
  struct list_node list;
  list_initialize(&list);
  TEST_ASSERT_EQUAL(0, list_length(&list));
}

/****************************************************************************
 * Name: collections_list__notail_after_init
 *
 * Description:
 *   Tests that a newly initialized list has no tail.
 ****************************************************************************/

static void collections_list__notail_after_init(void)
{
  struct list_node list;
  list_initialize(&list);
  TEST_ASSERT_NULL(list_remove_tail(&list));
}

/****************************************************************************
 * Name: collections_list__nohead_after_init
 *
 * Description:
 *   Tests that a newly initialized list has no head.
 ****************************************************************************/

static void collections_list__nohead_after_init(void)
{
  struct list_node list;
  list_initialize(&list);
  TEST_ASSERT_NULL(list_remove_head(&list));
}

/****************************************************************************
 * Name: collections_list__static_null_init
 *
 * Description:
 *   Tests that a list statically initialized with
 *   `LIST_INITIAL_CLEARED_VALUE` has NULL next and previous nodes.
 ****************************************************************************/

static void collections_list__static_null_init(void)
{
  struct list_node list = LIST_INITIAL_CLEARED_VALUE;
  TEST_ASSERT_NULL(list.next);
  TEST_ASSERT_NULL(list.prev);
  TEST_ASSERT_NULL(list_peek_head(&list));
  TEST_ASSERT_NULL(list_peek_tail(&list));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_collections_list
 *
 * Description:
 *   Runs the test cases for the list collection.
 ****************************************************************************/

void nest_collections_list(void)
{
  RUN_TEST(collections_list__empty_after_init);
  RUN_TEST(collections_list__zerolen_after_init);
  RUN_TEST(collections_list__notail_after_init);
  RUN_TEST(collections_list__nohead_after_init);
  RUN_TEST(collections_list__static_null_init);
}
