/****************************************************************************
 * apps/testing/kasantest/kasantest.c
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

#include <assert.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>

#include <sys/param.h>
#include <sys/wait.h>

#include <nuttx/fs/procfs.h>
#include <nuttx/mm/mm.h>
#include <nuttx/mm/kasan.h>

/****************************************************************************
 * Private Types Prototypes
 ****************************************************************************/

typedef struct testcase_s
{
  bool (*func)(FAR struct mm_heap_s *heap, size_t size);
  FAR const char *name;
} testcase_t;

typedef struct run_s
{
  char argv[32];
  FAR const testcase_t *testcase;
  FAR struct mm_heap_s *heap;
  size_t size;
} run_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static bool test_heap_underflow(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_overflow(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_use_after_free(FAR struct mm_heap_s *heap,
                                     size_t size);
static bool test_heap_invalid_free(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_double_free(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_poison(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_unpoison(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_memset(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_memcpy(FAR struct mm_heap_s *heap, size_t size);
static bool test_heap_memmove(FAR struct mm_heap_s *heap, size_t size);

/****************************************************************************
 * Private Data
 ****************************************************************************/

const static testcase_t g_kasan_test[] =
{
  {test_heap_underflow, "heap underflow"},
  {test_heap_overflow, "heap overflow"},
  {test_heap_use_after_free, "heap use after free"},
  {test_heap_invalid_free, "heap inval free"},
  {test_heap_double_free, "test heap double free"},
  {test_heap_poison, "heap poison"},
  {test_heap_unpoison, "heap unpoison"},
  {test_heap_memset, "heap memset"},
  {test_heap_memcpy, "heap memcpy"},
  {test_heap_memmove, "heap memmove"}
};

static char g_kasan_heap[65536] aligned_data(8);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool test_heap_underflow(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *mem = mm_malloc(heap, size);
  *(mem - 1) = 0x12;
  return false;
}

static bool test_heap_overflow(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *mem = mm_malloc(heap, size);
  size = mm_malloc_size(heap, mem);

  mem[size + 1] = 0x11;
  return false;
}

static bool test_heap_use_after_free(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *mem = mm_malloc(heap, size);

  mm_free(heap, mem);
  mem[0] = 0x10;
  return 0;
}

static bool test_heap_invalid_free(FAR struct mm_heap_s *heap, size_t size)
{
  int x;
  mm_free(heap, &x);
  return false;
}

static bool test_heap_double_free(FAR struct mm_heap_s *heap, size_t size)
{
  uint8_t *mem = mm_malloc(heap, size);

  mm_free(heap, mem);
  mm_free(heap, mem);
  return false;
}

static bool test_heap_poison(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *mem = mm_malloc(heap, size);
  size = mm_malloc_size(heap, mem);

  kasan_poison(mem, size);
  mem[0] = 0x10;
  return false;
}

static bool test_heap_unpoison(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *mem = mm_malloc(heap, size);
  size_t memsize = mm_malloc_size(heap, mem);

  kasan_poison(mem, memsize);
  kasan_unpoison(mem, memsize);
  mem[0] = 0x10;
  return true;
}

static bool test_heap_memset(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *mem = mm_malloc(heap, size);
  size = mm_malloc_size(heap, mem);

  memset(mem, 0x11, size + 1);
  return false;
}

static bool test_heap_memcpy(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *src;
  FAR uint8_t *dst;

  size = size / 2;
  src =  mm_malloc(heap, size);
  size = mm_malloc_size(heap, src);
  dst = mm_malloc(heap, size);

  memcpy(dst, src, size);
  memcpy(dst, src, size + 4);
  return false;
}

static bool test_heap_memmove(FAR struct mm_heap_s *heap, size_t size)
{
  FAR uint8_t *src;
  FAR uint8_t *dst;

  size = size / 2;
  src =  mm_malloc(heap, size);
  size = mm_malloc_size(heap, src);
  dst = mm_malloc(heap, size);

  memmove(dst, src, size);
  memmove(dst, src, size + 4);
  return false;
}

static int run_testcase(int argc, FAR char *argv[])
{
  FAR run_t *run = (FAR run_t *)(uintptr_t)strtoul(argv[1], NULL, 0);
  return run->testcase->func(run->heap, run->size);
}

static int run_test(FAR const testcase_t *test)
{
  size_t heap_size = 65536;
  FAR char *argv[3];
  FAR run_t *run;
  int status;
  pid_t pid;

  /* There is a memory leak here because we cannot guarantee that
   * it can be released correctly.
   */

  run = (run_t *)g_kasan_heap;
  if (!run)
    {
      return ERROR;
    }

  snprintf(run->argv, sizeof(run->argv), "%p", run);
  run->testcase = test;
  run->size = rand() % (heap_size / 2) + 1;
  run->heap = mm_initialize("kasan", (struct mm_heap_s *)&run[1], heap_size);
  if (!run->heap)
    {
      free(run);
      return ERROR;
    }

  argv[0] = "kasantest";
  argv[1] = run->argv;
  argv[2] = NULL;

  posix_spawn(&pid, "kasantest", NULL, NULL, argv, NULL);
  waitpid(pid, &status, 0);
  if (status == 0)
    {
      printf("KASan test: %s, size: %ld FAIL\n", test->name, run->size);
    }
  else
    {
      printf("KASan test: %s, size: %ld PASS\n", test->name, run->size);
    }

  mm_uninitialize(run->heap);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      size_t j;
      for (j = 0; j < nitems(g_kasan_test); j++)
        {
          if (run_test(&g_kasan_test[j]) < 0)
            {
              return EXIT_FAILURE;
            }
        }
    }
  else
    {
      return run_testcase(argc, argv);
    }

  return EXIT_SUCCESS;
}
