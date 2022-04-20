/****************************************************************************
 * apps/testing/mm/mm_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <queue.h>
#include <assert.h>

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

static dq_queue_t g_realloc_queue;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
      if (!mem[j])
        {
          printf("(%d)Allocating %d bytes\n", i,  size[j]);

          mem[j] = malloc(size[j]);
          printf("(%d)Memory allocated at %p\n", i, mem[j]);

          if (mem[j] == NULL)
            {
              int allocsize = MM_ALIGN_UP(size[j] + SIZEOF_MM_ALLOCNODE);

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
          int allocsize = MM_ALIGN_UP(newsize[j] + SIZEOF_MM_ALLOCNODE);

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
      printf("(%d)Allocating %d bytes aligned to 0x%08x\n",
             i,  size[j], align[i]);

      mem[j] = memalign(align[i], size[j]);
      printf("(%d)Memory allocated at %p\n", i, mem[j]);

      if (mem[j] == NULL)
        {
          int allocsize = MM_ALIGN_UP(size[j] + SIZEOF_MM_ALLOCNODE) +
                                      2 * align[i];

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

static void realloc_boundary_free(void)
{
  dq_entry_t *tail;

  /* Free all the memory in the relloc queue */

  printf("Free all the memory in the relloc queue\n");

  while (!dq_empty(&g_realloc_queue))
    {
      tail = dq_remlast(&g_realloc_queue);
      if (tail != NULL)
        {
          free(tail);
        }
      else
        {
          DEBUGASSERT(false);
        }
    }
}

static void *realloc_boundary_malloc(int *nodesize)
{
  int size;
  int index;
  void *ptr = NULL;

  DEBUGASSERT(nodesize);

  *nodesize = 0;
  g_alloc_info = mallinfo();
  if (g_alloc_info.mxordblk < MM_MIN_CHUNK)
    {
      size = MM_MIN_CHUNK;
    }
  else
    {
      /* Get a a suitable size to make sure function
       * realloc_boundary_malloc() can run twice.
       */

      index  = fls(g_alloc_info.mxordblk);
      size = 1 << (index - 1);
      size = (size < MM_MIN_CHUNK) ? MM_MIN_CHUNK : size;
    }

  /* Continuously mallocing util success or heap ran out */

  while (ptr == NULL && size >= MM_MIN_CHUNK)
    {
      ptr = malloc(size - SIZEOF_MM_ALLOCNODE);
      if (ptr)
        {
          *nodesize = size;
        }
      else
        {
          size = size >> 1;
        }
    }

  if (ptr)
    {
      printf("malloc success, ptr=%p, mem node size=%d\n", ptr, size);
    }
  else
    {
      printf("malloc failed, size=%zu\n",
             (size_t)((size << 1) - SIZEOF_MM_ALLOCNODE));
    }

  return ptr;
}

static void realloc_boundary(void)
{
  dq_entry_t *prev_ptr2 = NULL;
  dq_entry_t *prev_ptr1 = NULL;
  dq_entry_t *prev_ptr0 = NULL;
  int prev_size2 = 0;
  int prev_size1 = 0;
  int prev_size0 = 0;
  int reallocsize;

  /* The (MM_MIN_CHUNK - SIZEOF_MM_ALLOCNODE) must >= sizeof(dq_entry_t),
   * so all the malloced memory can hold dq_entry_t.
   */

  DEBUGASSERT(sizeof(dq_entry_t) <= (MM_MIN_CHUNK - SIZEOF_MM_ALLOCNODE));

  printf("memory realloc_boundary test start.\n");
  printf("MM_MIN_CHUNK=%d, SIZEOF_MM_ALLOCNODE=%zu\n",
         MM_MIN_CHUNK, SIZEOF_MM_ALLOCNODE);

  /* Malloc memory until the memeory ran out */

  while (1)
    {
      prev_ptr0 = (dq_entry_t *)realloc_boundary_malloc(&prev_size0);
      if (prev_ptr0 == NULL)
        {
          break;
        }

      /* Add all the malloced memory into the queue, so we can free
       * them conveniently after test finished.
       */

      dq_addlast(prev_ptr0 , &g_realloc_queue);

      /* Make sure prev_ptr1 and prev_ptr2 are at the bottom of heap */

      if (prev_ptr0 > prev_ptr1)
        {
          prev_size2 = prev_size1;
          prev_ptr2  = prev_ptr1;
          prev_size1 = prev_size0;
          prev_ptr1  = prev_ptr0;
        }
    }

  /* Free the previous 1 memory node. There will be only one freed memory
   * node in the heap.
   */

  printf("free the previous 1 memory node, addr: 0x%p\n", prev_ptr1);

  if (prev_ptr1 != NULL)
    {
      dq_rem(prev_ptr1, &g_realloc_queue);
      free(prev_ptr1);
    }
  else
    {
      /* Free all malloced memory */

      realloc_boundary_free();
      exit(1);
    }

  reallocsize = prev_size1 + prev_size2 - SIZEOF_MM_ALLOCNODE;

  printf("realloc the previous 2 memory node: \n");
  printf("reallocsize = %d, reallocptr = %p\n", reallocsize, prev_ptr2);

  /* Realloc reallocsize, the actual memory occupation in heap is
   * prev_size1 + prev_size2, rest memory size in the heap
   * is:
   * if MM_MIN_CHUNK >= 2 * SIZEOF_MM_ALLOCNODE
   *   REST = MM_MIN_CHUNK - 2 * SIZEOF_MM_ALLOCNODE
   * if MM_MIN_CHUNK < 2 * SIZEOF_MM_ALLOCNODE
   *   REST = 2 * MM_MIN_CHUNK - 2 * SIZEOF_MM_ALLOCNODE
   * If REST < SIZEOF_MM_FREENODE, software will assert fail in
   * mm_heap/mm_realloc.c line: 319.
   */

  prev_ptr0 = realloc(prev_ptr2, reallocsize);

  if (prev_ptr0 != NULL)
    {
      printf("realloc success\n");
    }
  else
    {
      printf("realloc fail\n");
    }

  /* Free all malloced memory */

  realloc_boundary_free();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  mm_showmallinfo();

  /* Memory boundary realloc test */

  realloc_boundary();

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
  return 0;
}
