/****************************************************************************
 * apps/testing/nest/collections/hmap.c
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

#include <search.h>

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
 * Name: collections_hmap__create
 *
 * Description:
 *   Tests that an hmap can be created.
 ****************************************************************************/

static void collections_hmap__create(void)
{
  TEST_ASSERT_EQUAL(1, hcreate(16));
  hdestroy();
}

/****************************************************************************
 * Name: collections_hmap__create_r
 *
 * Description:
 *   Tests that an hmap can be created with the re-entrant API.
 ****************************************************************************/

static void collections_hmap__create_r(void)
{
  struct hsearch_data htab;
  memset(&htab, 0, sizeof(htab));
  TEST_ASSERT_EQUAL(1, hcreate_r(16, &htab));
  hdestroy_r(&htab);
}

/****************************************************************************
 * Name: collections_hmap__create_zero
 *
 * Description:
 *   Tests that an hmap with zero elements can be created.
 ****************************************************************************/

static void collections_hmap__create_zero(void)
{
  TEST_ASSERT_EQUAL(1, hcreate(0));
  hdestroy();
}

/****************************************************************************
 * Name: collections_hmap__create_r_zero
 *
 * Description:
 *   Tests that an hmap with zero elements can be created with the re-entrant
 *   API.
 ****************************************************************************/

static void collections_hmap__create_r_zero(void)
{
  struct hsearch_data htab;
  memset(&htab, 0, sizeof(htab));
  TEST_ASSERT_EQUAL(1, hcreate_r(0, &htab));
  hdestroy_r(&htab);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nest_collections_hmap
 *
 * Description:
 *   Runs the test cases for the hash map collection.
 ****************************************************************************/

void nest_collections_hmap(void)
{
  RUN_TEST(collections_hmap__create);
  RUN_TEST(collections_hmap__create_r);
  RUN_TEST(collections_hmap__create_zero);
  RUN_TEST(collections_hmap__create_r_zero);
}
