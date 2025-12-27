/****************************************************************************
 * apps/testing/nuts/dstructs/list.c
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

#include <nuttx/list.h>

#include "tests.h"

#ifdef CONFIG_TESTING_NUTS_DSTRUCTS_LIST

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
 * Name: empty_after_init
 *
 * Description:
 *   Tests that a newly initialized list is considered empty.
 ****************************************************************************/

static void empty_after_init(void **state)
{
  UNUSED(state);
  struct list_node list;
  list_initialize(&list);
  assert_true(list_is_empty(&list));
}

/****************************************************************************
 * Name: zerolen_after_init
 *
 * Description:
 *   Tests that a newly initialized list has zero length.
 ****************************************************************************/

static void zerolen_after_init(void **state)
{
  UNUSED(state);
  struct list_node list;
  list_initialize(&list);
  assert_uint_equal(0, list_length(&list));
}

/****************************************************************************
 * Name: notail_after_init
 *
 * Description:
 *   Tests that a newly initialized list has no tail.
 ****************************************************************************/

static void notail_after_init(void **state)
{
  UNUSED(state);
  struct list_node list;
  list_initialize(&list);
  assert_null(list_remove_tail(&list));
}

/****************************************************************************
 * Name: nohead_after_init
 *
 * Description:
 *   Tests that a newly initialized list has no head.
 ****************************************************************************/

static void nohead_after_init(void **state)
{
  UNUSED(state);
  struct list_node list;
  list_initialize(&list);
  assert_null(list_remove_head(&list));
}

/****************************************************************************
 * Name: static_null_init
 *
 * Description:
 *   Tests that a list statically initialized with
 *   `LIST_INITIAL_CLEARED_VALUE` has NULL next and previous nodes.
 ****************************************************************************/

static void static_null_init(void **state)
{
  UNUSED(state);
  struct list_node list = LIST_INITIAL_CLEARED_VALUE;
  assert_null(list.next);
  assert_null(list.prev);
  assert_null(list_peek_head(&list));
  assert_null(list_peek_tail(&list));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nuts_dstructs_list
 *
 * Description:
 *   Runs the test cases for the list collection.
 ****************************************************************************/

int nuts_dstructs_list(void)
{
  static const struct CMUnitTest tests[] = {
      cmocka_unit_test(empty_after_init),
      cmocka_unit_test(zerolen_after_init),
      cmocka_unit_test(notail_after_init),
      cmocka_unit_test(nohead_after_init),
      cmocka_unit_test(static_null_init),
  };

  return cmocka_run_group_tests_name("list_node", tests, NULL, NULL);
}

#endif /* CONFIG_TESTING_NUTS_DSTRUCTS_LIST */
