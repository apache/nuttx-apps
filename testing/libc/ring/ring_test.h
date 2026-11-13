/****************************************************************************
 * apps/testing/libc/ring/ring_test.h
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

#ifndef __APPS_TESTING_LIBC_RING_RING_TEST_H
#define __APPS_TESTING_LIBC_RING_RING_TEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/ring.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <sys/param.h>
#include <unistd.h>

#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RING_TEST_CAP       64
#define RING_TEST_ITEMS     1024
#define RING_TEST_BATCH     8
#define RING_TEST_THREADS   2

/* The ring counts bytes while the cases count items, so each test ring
 * records its item size and the rt_ wrappers convert both ways.
 */

#define RING_TEST_DEFINE(name, type, cap) \
  static char name##_blob[sizeof(struct ring_blob_s) + \
                          (cap) * sizeof(type)] \
              aligned_data(RING_BLOB_ALIGN); \
  static struct ring_test_s name = \
  { \
    .r    = &name.handle, \
    .blob = name##_blob, \
    .size = sizeof(name##_blob), \
    .elem = sizeof(type), \
  }

#define RT_BYTES(t, n)  ((uint32_t)(n) * (t)->elem)
#define RT_ITEMS(t, b)  ((uint32_t)(b) / (t)->elem)

#define RT_IO(name, op) \
  static inline uint32_t name(FAR struct ring_test_s *t, \
                              FAR void *items, uint32_t n) \
  { \
    return RT_ITEMS(t, op(t->r, items, RT_BYTES(t, n))); \
  }

#define RT_SKIP(name, op) \
  static inline uint32_t name(FAR struct ring_test_s *t, uint32_t n) \
  { \
    return RT_ITEMS(t, op(t->r, RT_BYTES(t, n))); \
  }

#define rt_count(t)               RT_ITEMS(t, ring_count((t)->r))
#define rt_capacity(t)            RT_ITEMS(t, ring_capacity((t)->r))
#define rt_reset(t)               ((void)ring_reset((t)->r))

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ring_test_s
{
  FAR struct ring_s *r;          /* Handle in use: &handle or ring_alloc */
  struct ring_s      handle;
  FAR char          *blob;
  uint32_t           size;       /* Whole blob, header included */
  uint32_t           elem;       /* Item size in bytes */
};

/****************************************************************************
 * Inline Functions - item-count wrappers
 ****************************************************************************/

RT_IO(rt_spsc_enqueue, ring_spsc_enqueue)
RT_IO(rt_spsc_dequeue, ring_spsc_dequeue)
RT_IO(rt_mpsc_enqueue, ring_mpsc_enqueue)
RT_IO(rt_mpsc_dequeue, ring_mpsc_dequeue)
RT_IO(rt_spmc_enqueue, ring_spmc_enqueue)
RT_IO(rt_spmc_dequeue, ring_spmc_dequeue)
RT_IO(rt_mpmc_enqueue, ring_mpmc_enqueue)
RT_IO(rt_mpmc_dequeue, ring_mpmc_dequeue)

/* Overwrite returns the items that did not stay in the ring */

RT_IO(rt_spmc_overwrite, ring_spmc_overwrite)
RT_IO(rt_mpmc_overwrite, ring_mpmc_overwrite)

RT_SKIP(rt_spsc_skip, ring_spsc_skip)
RT_SKIP(rt_mpmc_skip, ring_mpmc_skip)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int ring_test_spsc_local(void);
int ring_test_mpsc_local(void);
int ring_test_spmc_local(void);
int ring_test_mpmc_local(void);

/****************************************************************************
 * Inline Functions - test ring helpers
 ****************************************************************************/

/****************************************************************************
 * Name: ring_test_init
 *
 * Description:
 *   Cold-start a test ring.  Both magics are cleared first: a READY blob
 *   would be recovered as a warm restart, and a READY handle makes init
 *   return without touching the blob.
 *
 ****************************************************************************/

static inline void ring_test_init(FAR struct ring_test_s *t)
{
  memset(t->r, 0, sizeof(*t->r));
  memset(t->blob, 0, sizeof(struct ring_blob_s));
  ring_spsc_init(t->r, t->blob, t->size, 0, false);
}

/****************************************************************************
 * Name: ring_test_alloc
 *
 * Description:
 *   Wrap a ring_alloc() ring holding cap items of elem bytes.  Free it
 *   with ring_free(t->r).
 *
 ****************************************************************************/

static inline FAR struct ring_s *ring_test_alloc(FAR struct ring_test_s *t,
                                                 uint32_t cap,
                                                 uint32_t elem)
{
  t->elem = elem;
  t->size = sizeof(struct ring_blob_s) + cap * elem;
  t->blob = NULL;
  t->r = ring_alloc(t->size);
  return t->r;
}

#endif /* __APPS_TESTING_LIBC_RING_RING_TEST_H */
