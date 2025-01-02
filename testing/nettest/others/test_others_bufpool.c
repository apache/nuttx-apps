/****************************************************************************
 * apps/testing/nettest/others/test_others_bufpool.c
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

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "../../nuttx/net/utils/utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_PREALLOC  10
#define TEST_DYNALLOC  1
#define TEST_MAXALLOC  20

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_net_bufpool_alloc_to
 ****************************************************************************/

static void test_net_bufpool_alloc_to(FAR struct net_bufpool_s *pool,
                                      FAR sq_queue_t *allocated)
{
  FAR sq_entry_t *node;
  assert_true(NET_BUFPOOL_TEST(*pool) == OK);
  node = NET_BUFPOOL_ALLOC(*pool);
  assert_non_null(node);
  sq_addlast(node, allocated);
}

/****************************************************************************
 * Name: test_net_bufpool_free_from
 ****************************************************************************/

static void test_net_bufpool_free_from(FAR struct net_bufpool_s *pool,
                                       FAR sq_queue_t *allocated)
{
  while (!sq_empty(allocated))
    {
      FAR sq_entry_t *node = sq_remfirst(allocated);
      NET_BUFPOOL_FREE(*pool, node);
      assert_true(NET_BUFPOOL_TEST(*pool) == OK);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_others_bufpool
 ****************************************************************************/

void test_others_bufpool(FAR void **state)
{
  NET_BUFPOOL_DECLARE(fixed, sizeof(sq_entry_t), TEST_PREALLOC, 0, 0);
  NET_BUFPOOL_DECLARE(limited, sizeof(sq_entry_t), TEST_PREALLOC,
                               TEST_DYNALLOC, TEST_MAXALLOC);
  NET_BUFPOOL_DECLARE(unlimited, sizeof(sq_entry_t), TEST_PREALLOC,
                                 TEST_DYNALLOC, 0);
  sq_queue_t allocated_fixed;
  sq_queue_t allocated_limited;
  sq_queue_t allocated_unlimited;
  FAR sq_entry_t *node;
  int i;

#ifdef NET_BUFPOOL_INIT
  NET_BUFPOOL_INIT(fixed);
  NET_BUFPOOL_INIT(limited);
  NET_BUFPOOL_INIT(unlimited);
#endif
  sq_init(&allocated_fixed);
  sq_init(&allocated_limited);
  sq_init(&allocated_unlimited);

  for (i = 0; i < TEST_PREALLOC; i++)
    {
      test_net_bufpool_alloc_to(&fixed, &allocated_fixed);
      test_net_bufpool_alloc_to(&limited, &allocated_limited);
      test_net_bufpool_alloc_to(&unlimited, &allocated_unlimited);
    }

  for (i = TEST_PREALLOC; i < TEST_MAXALLOC; i++)
    {
      test_net_bufpool_alloc_to(&limited, &allocated_limited);
      test_net_bufpool_alloc_to(&unlimited, &allocated_unlimited);
    }

  node = NET_BUFPOOL_TRYALLOC(fixed);
  assert_null(node);
  assert_true(NET_BUFPOOL_TEST(fixed) == -ENOSPC);

  node = NET_BUFPOOL_TRYALLOC(limited);
  assert_null(node);
  assert_true(NET_BUFPOOL_TEST(limited) == -ENOSPC);

  node = NET_BUFPOOL_TRYALLOC(unlimited);
  assert_non_null(node);
  assert_true(NET_BUFPOOL_TEST(unlimited) == OK);
  NET_BUFPOOL_FREE(unlimited, node);

  test_net_bufpool_free_from(&fixed, &allocated_fixed);
  test_net_bufpool_free_from(&limited, &allocated_limited);
  test_net_bufpool_free_from(&unlimited, &allocated_unlimited);
}
