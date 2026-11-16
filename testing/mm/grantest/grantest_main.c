/****************************************************************************
 * apps/testing/mm/grantest/grantest_main.c
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <nuttx/compiler.h>
#include <nuttx/mm/gran.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOG2GRAN   4
#define GRANSIZE   (1 << LOG2GRAN)
#define NGRANULES  16
#define HEAPSIZE   (NGRANULES * GRANSIZE)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The heap is aligned to its size, so that granule alignment is also
 * address alignment.
 */

static uint8_t g_heap[HEAPSIZE] aligned_data(HEAPSIZE);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Return the granule number of an allocation, or -1 for NULL */

static int granule(FAR void *mem)
{
  if (mem == NULL)
    {
      return -1;
    }

  return ((FAR uint8_t *)mem - g_heap) >> LOG2GRAN;
}

static GRAN_HANDLE heap_init(void)
{
  GRAN_HANDLE handle = gran_initialize(g_heap, HEAPSIZE, LOG2GRAN, LOG2GRAN);

  if (handle == NULL)
    {
      printf("grantest: gran_initialize() failed\n");
      exit(EXIT_FAILURE);
    }

  return handle;
}

static bool report(FAR const char *name, bool pass, int expect, int got)
{
  printf("grantest: %-34s %s (expected %d, got %d)\n",
         name, pass ? "PASS" : "FAIL", expect, got);
  return pass;
}

/* A reservation of one granule uses one granule */

static bool test_reserve_one(void)
{
  GRAN_HANDLE handle = heap_init();
  int got;

  gran_reserve(handle, (uintptr_t)g_heap, GRANSIZE);
  got = granule(gran_alloc(handle, GRANSIZE));
  gran_release(handle);

  return report("reserve one granule:", got == 1, 1, got);
}

/* The last granule of the heap can be reserved */

static bool test_reserve_last(void)
{
  GRAN_HANDLE handle = heap_init();
  int got;

  got = granule(gran_reserve(handle,
                             (uintptr_t)&g_heap[HEAPSIZE - GRANSIZE],
                             GRANSIZE));
  gran_release(handle);

  return report("reserve the last granule:", got == NGRANULES - 1,
                NGRANULES - 1, got);
}

/* gran_free() of a reserved range makes the whole heap free again */

static bool test_reserve_free(void)
{
  GRAN_HANDLE handle = heap_init();
  FAR void *mem;
  int got;

  mem = gran_reserve(handle, (uintptr_t)g_heap, 2 * GRANSIZE);
  if (mem != NULL)
    {
      gran_free(handle, mem, 2 * GRANSIZE);
    }

  got = granule(gran_alloc(handle, HEAPSIZE));
  gran_release(handle);

  return report("reserve, free, allocate the heap:", got == 0, 0, got);
}

/* An alignment of zero does not return memory that is in use */

static bool test_align_zero(void)
{
  GRAN_HANDLE handle = heap_init();
  int got;

  gran_alloc(handle, 2 * GRANSIZE);
  got = granule(gran_alloc_align(handle, 2 * GRANSIZE, 0));
  gran_release(handle);

  return report("align 0, two granules:", got == 2, 2, got);
}

/* An alignment of zero allocates one granule */

static bool test_align_zero_one(void)
{
  GRAN_HANDLE handle = heap_init();
  int got;

  got = granule(gran_alloc_align(handle, GRANSIZE, 0));
  gran_release(handle);

  return report("align 0, one granule:", got == 0, 0, got);
}

/* An alignment that is not a power of two is rejected */

static bool test_align_npot(void)
{
  GRAN_HANDLE handle = heap_init();
  int got;

  gran_alloc(handle, GRANSIZE);
  got = granule(gran_alloc_align(handle, GRANSIZE, 3 * GRANSIZE));
  gran_release(handle);

  return report("align 3 granules:", got == -1, -1, got);
}

/* An alignment that is a power of two is kept */

static bool test_align_pot(void)
{
  GRAN_HANDLE handle = heap_init();
  int got;

  gran_alloc(handle, GRANSIZE);
  got = granule(gran_alloc_align(handle, GRANSIZE, 4 * GRANSIZE));
  gran_release(handle);

  return report("align 4 granules:", got == 4, 4, got);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int failed = 0;

  failed += !test_reserve_one();
  failed += !test_reserve_last();
  failed += !test_reserve_free();
  failed += !test_align_zero();
  failed += !test_align_zero_one();
  failed += !test_align_npot();
  failed += !test_align_pot();

  printf("grantest: %d failed\n", failed);
  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
