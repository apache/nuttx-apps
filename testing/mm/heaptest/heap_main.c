/****************************************************************************
 * apps/testing/mm/heaptest/heap_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#ifdef CONFIG_TESTING_MM_POWEROFF
#include <sys/boardctl.h>
#endif

#include <nuttx/queue.h>

/* Include nuttx/mm/mm_heap/mm.h */

#include <mm.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NTEST_ALLOCS 32

/* #define STOP_ON_ERRORS do {} while (0) */

#define STOP_ON_ERRORS exit(1)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Test allocations */

static const int g_alloc_sizes[NTEST_ALLOCS] =
{
    1024,    12,    962,   5692, 10254,   111,   9932,    601,
    222,   2746,      3, 124321,    68,   776,   6750,    852,
    4732,    28,    901,    480,  5011,  1536,   2011,  81647,
    646,   1646,  69179,    194,  2590,     7,    969,     70
};

static const int g_alloc_small_sizes[NTEST_ALLOCS] =
{
       1,     2,      3,      4,     5,     6,      7,      8,
       9,    10,     11,     12,    13,    14,     15,     16,
      17,    18,     19,     20,    21,    22,     23,     24,
      25,    26,     27,     28,    29,    30,     31,     32,
};

static const int g_realloc_sizes[NTEST_ALLOCS] =
{
    18,     3088,    963,    123,   511, 11666,   3723,     42,
    9374,   1990,   1412,      6,   592,  4088,     11,   5040,
    8663,  91255,     28,   4346,  9172,   168,    229,   4734,
    59139,   221,   7830,  30421,  1666,     4,    812,    416
};

static const int g_random1[NTEST_ALLOCS] =
{
     20,     11,      3,     31,     9,    29,      7,     17,
     21,      2,     26,     18,    14,    25,      0,     10,
     27,     19,     22,     28,     8,    30,     12,     15,
      4,      1,     24,      6,    16,    13,      5,     23
};

static const int g_random2[NTEST_ALLOCS] =
{
      2,     19,     12,     23,    30,    11,     27,      4,
     20,      7,      0,     16,    28,    15,      5,     24,
     10,     17,     25,     31,     8,    29,      3,     26,
      9,     18,     22,     13,     1,    21,     14,      6
};

static const int g_random3[NTEST_ALLOCS] =
{
      8,     17,      3,     18,     26,   23,     30,     11,
     12,     22,      4,     20,     25,   10,     27,      1,
     29,     14,     19,     21,      0,   31,      7,     24,
      9,     15,      2,     28,     16,    6,     13,      5
};

static const int g_alignment[NTEST_ALLOCS / 2] =
{
    128,  2048, 131072,   8192,    32,  32768, 16384 , 262144,
    512,  4096,  65536,      8,     64,  1024,    16,       4
};

static const int g_alignment2[NTEST_ALLOCS / 2] =
{
      1,     2,      4,      8,    16,     32,     64,    128,
      1,     2,      4,      8,    16,     32,     64,    128,
};

static FAR void       *g_allocs[NTEST_ALLOCS];
static struct mallinfo g_alloc_info;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool is_oversize(int size)
{
  if (size < 0)
    {
      return false;
    }

  unsigned long threshold = g_alloc_info.mxordblk * 3 / 4;
  return size > threshold;
}

static void mm_showmallinfo(void)
{
  g_alloc_info = mallinfo();
  printf("     mallinfo:\n");
  printf("       Total space allocated from system = %lu\n",
         (unsigned long)g_alloc_info.arena);
  printf("       Number of non-inuse chunks        = %lu\n",
         (unsigned long)g_alloc_info.ordblks);
  printf("       Largest non-inuse chunk           = %lu\n",
         (unsigned long)g_alloc_info.mxordblk);
  printf("       Total allocated space             = %lu\n",
         (unsigned long)g_alloc_info.uordblks);
  printf("       Total non-inuse space             = %lu\n",
         (unsigned long)g_alloc_info.fordblks);
}

static void do_mallocs(FAR void **mem, FAR const int *size,
                       FAR const int *seq, int n)
{
  int i;
  int j;

  for (i = 0; i < n; i++)
    {
      j = seq[i];
      int allocsize = MM_ALIGN_UP(size[j] + MM_SIZEOF_ALLOCNODE);
      if (!mem[j])
        {
          printf("(%d)Allocating %d bytes\n", i, allocsize);
          if (is_oversize(allocsize))
            {
              printf("(%d)The allocated memory exceeds the threshold, "
                       "skipping\n", i);
              continue;
            }

          mem[j] = malloc(size[j]);
          printf("(%d)Memory allocated at %p\n", i, mem[j]);

          if (mem[j] == NULL)
            {
              fprintf(stderr, "(%d)malloc failed for allocsize=%d\n",
                      i, allocsize);

              if (allocsize > g_alloc_info.mxordblk)
                {
                  fprintf(stderr,
                          "   Normal, largest free block is only %lu\n",
                          (unsigned long)g_alloc_info.mxordblk);
                }
              else
                {
                  fprintf(stderr, "   ERROR largest free block is %lu\n",
                          (unsigned long)g_alloc_info.mxordblk);
                  exit(1);
                }
            }
          else
            {
              memset(mem[j], 0xaa, size[j]);
            }

          mm_showmallinfo();
        }
    }
}

static void do_reallocs(FAR void **mem, FAR const int *oldsize,
                        FAR const int *newsize, FAR const int *seq, int n)
{
  int i;
  int j;
  void *ptr;

  for (i = 0; i < n; i++)
    {
      j = seq[i];
      int allocsize = MM_ALIGN_UP(newsize[j] + MM_SIZEOF_ALLOCNODE) -
                      MM_ALIGN_UP(oldsize[j] + MM_SIZEOF_ALLOCNODE);
      if (is_oversize(allocsize))
        {
          printf("(%d)The reallocs memory exceeds the threshold, "
                    "skipping\n", i);
          continue;
        }

      printf("(%d)Re-allocating at %p from %d to %d bytes\n",
             i, mem[j], oldsize[j], newsize[j]);

      /* Return null if realloc failed, so using a local variable to store
       * the return value to avoid the missing of old memory pointer.
       */

      ptr = realloc(mem[j], newsize[j]);
      if (ptr != NULL)
        {
          mem[j] = ptr;
        }

      printf("(%d)Memory re-allocated at %p\n", i, ptr);

      if (ptr == NULL)
        {
          fprintf(stderr,
                  "(%d)realloc failed for allocsize=%d\n", i, allocsize);
          if (allocsize > g_alloc_info.mxordblk)
            {
              fprintf(stderr, "   Normal, largest free block is only %lu\n",
                      (unsigned long)g_alloc_info.mxordblk);
            }
          else
            {
              fprintf(stderr, "   ERROR largest free block is %lu\n",
                      (unsigned long)g_alloc_info.mxordblk);
              exit(1);
            }
        }
      else
        {
          memset(mem[j], 0x55, newsize[j]);
        }

      mm_showmallinfo();
    }
}

static void do_memaligns(FAR void **mem,
                         FAR const int *size,
                         FAR const int *align,
                         FAR const int *seq, int n)
{
  int i;
  int j;

  for (i = 0; i < n; i++)
    {
      j = seq[i];
      int allocsize = MM_ALIGN_UP(size[j] + MM_SIZEOF_ALLOCNODE) +
                                      2 * align[i];
      printf("(%d)Allocating %d bytes aligned to 0x%08x\n",
             i,  size[j], align[i]);
      if (is_oversize(allocsize))
        {
          printf("(%d)The reallocs memory exceeds the threshold, "
                    "skipping\n", i);
          continue;
        }

      mem[j] = memalign(align[i], size[j]);
      printf("(%d)Memory allocated at %p\n", i, mem[j]);

      if (mem[j] == NULL)
        {
          fprintf(stderr,
                  "(%d)memalign failed for allocsize=%d\n", i, allocsize);
          if (allocsize > g_alloc_info.mxordblk)
            {
              fprintf(stderr, "   Normal, largest free block is only %lu\n",
                      (unsigned long)g_alloc_info.mxordblk);
            }
          else
            {
              fprintf(stderr, "   ERROR largest free block is %lu\n",
                      (unsigned long)g_alloc_info.mxordblk);
              exit(1);
            }
        }
      else
        {
          if (((uintptr_t)mem[j] % align[i]) != 0)
            {
              fprintf(stderr,
                      "   ERROR wrong alignment: ptr %p, alignment %d\n",
                      mem[j], align[i]);
              exit(1);
            }

          memset(mem[j], 0x33, size[j]);
        }

      mm_showmallinfo();
    }
}

static void do_frees(FAR void **mem, FAR const int *size,
                     FAR const int *seq, int n)
{
  int i;
  int j;

  for (i = 0; i < n; i++)
    {
      j = seq[i];

      printf("(%d)Releasing memory at %p (size=%d bytes)\n",
             i, mem[j], size[j]);

      free(mem[j]);
      mem[j] = NULL;

      mm_showmallinfo();
    }
}

static int mm_stress_test(int delay, int prio, int maxsize)
{
  FAR unsigned char *tmp;
  int size;
  int i;
  if (prio != 0)
    {
      struct sched_param param;

      sched_getparam(0, &param);
      param.sched_priority = prio;
      sched_setparam(0, &param);
    }

  while (1)
    {
      size = random() % maxsize + 1;
      tmp = malloc(size);
      assert(tmp);

      memset(tmp, 0xfe, size);
      usleep(delay);

      for (i = 0; i < size; i++)
        {
          assert(tmp[i] == 0xfe);
        }

      free(tmp);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int stress_test_mode = 0;
  int delay = 1;
  int prio = 0;
  int i;
  int maxsize = 1024;

  while ((i = getopt(argc, argv, "mfd:p:s:")) != ERROR)
    {
      switch (i)
        {
          case 'm':
            {
              stress_test_mode = 1;
              break;
            }

          case 'd':
            {
              delay = atoi(optarg);
              break;
            }

          case 'p':
            {
              prio = atoi(optarg);
              break;
            }

          case 's':
            {
              maxsize = atoi(optarg);
              break;
            }

          default:
            {
              printf("Unrecognized option: '%c'\n", i);
              return -EINVAL;
            }
        }
    }

  if (stress_test_mode)
    {
      return mm_stress_test(delay, prio, maxsize);
    }

  mm_showmallinfo();

  /* Allocate some memory */

  do_mallocs(g_allocs, g_alloc_sizes, g_random1, NTEST_ALLOCS);

  /* Re-allocate the memory */

  do_reallocs(g_allocs, g_alloc_sizes,
              g_realloc_sizes, g_random2, NTEST_ALLOCS);

  /* Release the memory */

  do_frees(g_allocs, g_realloc_sizes, g_random3, NTEST_ALLOCS);

  /* Allocate aligned memory */

  do_memaligns(g_allocs, g_alloc_sizes,
               g_alignment, g_random2, NTEST_ALLOCS / 2);
  do_memaligns(g_allocs, g_alloc_sizes,
               g_alignment, &g_random2[NTEST_ALLOCS / 2],
               NTEST_ALLOCS / 2);

  /* Release aligned memory */

  do_frees(g_allocs, g_alloc_sizes, g_random1, NTEST_ALLOCS);

  /* Allocate aligned memory */

  do_memaligns(g_allocs, g_alloc_small_sizes,
               g_alignment2, g_random2, NTEST_ALLOCS / 2);
  do_memaligns(g_allocs, g_alloc_small_sizes,
               g_alignment2, &g_random2[NTEST_ALLOCS / 2],
               NTEST_ALLOCS / 2);

  /* Release aligned memory */

  do_frees(g_allocs, g_alloc_small_sizes, g_random1, NTEST_ALLOCS);

  printf("TEST COMPLETE\n");

#ifdef CONFIG_TESTING_MM_POWEROFF
  /* Power down. This is useful when used with the simulator and gcov,
   * as the graceful shutdown allows for the generation of the .gcda files.
   */

  boardctl(BOARDIOC_POWEROFF, 0);
#endif

  return 0;
}
