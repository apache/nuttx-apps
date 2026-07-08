/****************************************************************************
 * apps/testing/libc/arch_libc/arch_libc_test_main.c
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
#include <nuttx/arch.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <nuttx/debug.h>
#include <assert.h>
#include <sys/param.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_BUF_SIZE  512
#define TEST_REPEAT    100
#define MAX_ALIGN      16

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_buf1[TEST_BUF_SIZE + MAX_ALIGN];
static char g_buf2[TEST_BUF_SIZE + MAX_ALIGN];
static volatile uintptr_t g_sink;

/* Boundary sizes that exercise 8-byte and 16-byte chunk edges and sub-word
 * tails, so that vectorized (NEON/MVE) and word-at-a-time paths are stressed
 * at their alignment and size boundaries.  Unused if every test that sweeps
 * these sizes is disabled.
 */

static const int unused_data g_boundary_sizes[] =
{
  0, 1, 7, 8, 9, 15, 16, 17, 31, 32, 33,
  63, 64, 65, 127, 128, 129, 255, 256, 257
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void fill_pattern(FAR char *buf, int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      buf[i] = 'A' + (i % 26);
    }
}

/****************************************************************************
 * Name: test_memcpy
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_MEMCPY
static int test_memcpy(void)
{
  int align;
  int size;
  int fail = 0;

  printf("Testing memcpy...\n");
  for (align = 0; align < 8; align++)
    {
      for (size = 1; size <= 128; size++)
        {
          fill_pattern(g_buf1 + align, size);
          memset(g_buf2, 0, sizeof(g_buf2));
          memcpy(g_buf2 + align, g_buf1 + align, size);
          if (memcmp(g_buf2 + align, g_buf1 + align, size) != 0)
            {
              printf("  FAIL: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("memcpy: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_memcpy(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)memcpy(g_buf2, g_buf1, 128);
    }

  end = perf_gettime();
  printf("memcpy(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_memmove
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_MEMMOVE
static int test_memmove(void)
{
  int align;
  int si;
  int size;
  int fail = 0;

  printf("Testing memmove...\n");

  /* Sweep alignment 0..7 and boundary sizes, across four overlap layouts:
   *  forward  : dst = src + 8        (dst > src, backward copy internally)
   *  backward : dst = src - 8        (dst < src, forward copy internally)
   *  contained: dst = src + size/2   (full overlap region)
   *  adjacent : dst = src + size     (no overlap, tail-to-tail)
   */

  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          /* Forward overlap: dst = src + 8 */

          fill_pattern(g_buf1 + align + 8, size);
          memcpy(g_buf2 + align + 8, g_buf1 + align + 8, size);
          memmove(g_buf1 + align + 16, g_buf1 + align + 8, size);
          if (memcmp(g_buf1 + align + 16, g_buf2 + align + 8, size) != 0)
            {
              printf("  FAIL forward: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Backward overlap: dst = src - 8 */

          fill_pattern(g_buf1 + align + 16, size);
          memcpy(g_buf2 + align + 16, g_buf1 + align + 16, size);
          memmove(g_buf1 + align + 8, g_buf1 + align + 16, size);
          if (memcmp(g_buf1 + align + 8, g_buf2 + align + 16, size) != 0)
            {
              printf("  FAIL backward: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Contained overlap: dst = src + size/2 */

          fill_pattern(g_buf1 + align + 32, size);
          memcpy(g_buf2 + align + 32, g_buf1 + align + 32, size);
          memmove(g_buf1 + align + 32 + size / 2,
                  g_buf1 + align + 32, size);
          if (memcmp(g_buf1 + align + 32 + size / 2,
                     g_buf2 + align + 32, size) != 0)
            {
              printf("  FAIL contained: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Adjacent (no overlap): dst = src + size.
           * The destination tail reaches align + 2*size, so the base
           * offset must satisfy align + 2*size <= sizeof(g_buf1); a
           * fixed +64 base overflows g_buf1 for the larger boundary
           * sizes (e.g. size=255, align=0 writes 45 bytes past the
           * end), which AddressSanitizer flags as a global-buffer-
           * overflow.  Start from g_buf1 + align instead.
           */

          fill_pattern(g_buf1 + align, size);
          memcpy(g_buf2 + align, g_buf1 + align, size);
          memmove(g_buf1 + align + size,
                  g_buf1 + align, size);
          if (memcmp(g_buf1 + align + size,
                     g_buf2 + align, size) != 0)
            {
              printf("  FAIL adjacent: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("memmove: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_memmove(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)memmove(g_buf2, g_buf1, 128);
    }

  end = perf_gettime();
  printf("memmove(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_memset
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_MEMSET
static int test_memset(void)
{
  int align;
  int size;
  int i;
  int fail = 0;

  printf("Testing memset...\n");
  for (align = 0; align < 8; align++)
    {
      for (size = 1; size <= 128; size++)
        {
          memset(g_buf1, 0, sizeof(g_buf1));
          memset(g_buf1 + align, 0xaa, size);
          for (i = 0; i < size; i++)
            {
              if ((unsigned char)g_buf1[align + i] != 0xaa)
                {
                  printf("  FAIL: align=%d size=%d idx=%d\n",
                         align, size, i);
                  fail++;
                  break;
                }
            }
        }
    }

  printf("memset: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_memset(void)
{
  clock_t start;
  clock_t end;
  int i;

  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)memset(g_buf1, 0x55, 128);
    }

  end = perf_gettime();
  printf("memset(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_memcmp
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_MEMCMP
static int test_memcmp(void)
{
  int align;
  int si;
  int size;
  int fail = 0;

  printf("Testing memcmp...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);
          fill_pattern(g_buf2 + align, size);

          /* Equal */

          if (memcmp(g_buf1 + align, g_buf2 + align, size) != 0)
            {
              printf("  FAIL equal: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Last byte less */

          g_buf2[align + size - 1] = '~';
          if (memcmp(g_buf1 + align, g_buf2 + align, size) >= 0)
            {
              printf("  FAIL less: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Last byte greater */

          g_buf2[align + size - 1] = 0;
          if (memcmp(g_buf1 + align, g_buf2 + align, size) <= 0)
            {
              printf("  FAIL greater: align=%d size=%d\n", align, size);
              fail++;
            }

          /* First byte differs */

          g_buf2[align] = g_buf1[align] + 1;
          if (memcmp(g_buf1 + align, g_buf2 + align, size) >= 0)
            {
              printf("  FAIL first: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("memcmp: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_memcmp(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  fill_pattern(g_buf2, 128);
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)memcmp(g_buf1, g_buf2, 128);
    }

  end = perf_gettime();
  printf("memcmp(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_memchr
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_MEMCHR
static int test_memchr(void)
{
  int align;
  int si;
  int size;
  int fail = 0;
  FAR void *p;

  printf("Testing memchr...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);

          /* Find first char (pattern starts with 'A' at align) */

          p = memchr(g_buf1 + align, g_buf1[align], size);
          if (p != g_buf1 + align)
            {
              printf("  FAIL first: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Find a unique marker placed at the last position */

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size - 1] = 0x7e;
          p = memchr(g_buf1 + align, 0x7e, size);
          if (p != g_buf1 + align + size - 1)
            {
              printf("  FAIL last: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Find a unique marker placed in the middle */

          if (size > 2)
            {
              fill_pattern(g_buf1 + align, size);
              g_buf1[align + size / 2] = 0x7e;
              p = memchr(g_buf1 + align, 0x7e, size);
              if (p != g_buf1 + align + size / 2)
                {
                  printf("  FAIL mid: align=%d size=%d\n", align, size);
                  fail++;
                }
            }

          /* Not found (full scan) */

          fill_pattern(g_buf1 + align, size);
          p = memchr(g_buf1 + align, 0xff, size);
          if (p != NULL)
            {
              printf("  FAIL notfound: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("memchr: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_memchr(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)memchr(g_buf1, g_buf1[127], 128);
    }

  end = perf_gettime();
  printf("memchr(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strlen
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRLEN
static int test_strlen(void)
{
  int align;
  int si;
  int size;
  int fail = 0;

  printf("Testing strlen...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          if ((int)strlen(g_buf1 + align) != size)
            {
              printf("  FAIL: align=%d size=%d got=%d\n", align, size,
                     (int)strlen(g_buf1 + align));
              fail++;
            }
        }
    }

  printf("strlen: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strlen(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = strlen(g_buf1);
    }

  end = perf_gettime();
  printf("strlen(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strcmp
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCMP
static int test_strcmp(void)
{
  int align;
  int si;
  int size;
  int fail = 0;

  printf("Testing strcmp...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          fill_pattern(g_buf2 + align, size);
          g_buf2[align + size] = '\0';

          /* Equal */

          if (strcmp(g_buf1 + align, g_buf2 + align) != 0)
            {
              printf("  FAIL equal: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Last byte less */

          g_buf2[align + size - 1] = '~';
          if (strcmp(g_buf1 + align, g_buf2 + align) >= 0)
            {
              printf("  FAIL less: align=%d size=%d\n", align, size);
              fail++;
            }

          /* First byte differs */

          fill_pattern(g_buf2 + align, size);
          g_buf2[align + size] = '\0';
          g_buf2[align] = g_buf1[align] + 1;
          if (strcmp(g_buf1 + align, g_buf2 + align) >= 0)
            {
              printf("  FAIL first: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("strcmp: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strcmp(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  fill_pattern(g_buf2, 128);
  g_buf2[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = strcmp(g_buf1, g_buf2);
    }

  end = perf_gettime();
  printf("strcmp(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strcpy
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCPY
static int test_strcpy(void)
{
  int size;
  int fail = 0;
  int ai;

  printf("Testing strcpy...\n");

  /* ai encodes da*8+sa: sa==da keeps src/dst in the same 4-byte congruence
   * (aligned word-copy path), sa!=da forces different congruence and
   * exercises the shift-merge path; any nonzero da also hits the dst byte
   * prologue.
   */

  for (ai = 0; ai < 64; ai++)
    {
      int da = ai / 8;
      int sa = ai % 8;

      for (size = 1; size <= 64; size++)
        {
          fill_pattern(g_buf1 + sa, size);
          g_buf1[sa + size] = '\0';
          memset(g_buf2, 0, sizeof(g_buf2));
          strcpy(g_buf2 + da, g_buf1 + sa);
          if (strcmp(g_buf2 + da, g_buf1 + sa) != 0)
            {
              printf("  FAIL: sa=%d da=%d size=%d\n", sa, da, size);
              fail++;
            }
        }
    }

  printf("strcpy: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strcpy(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)strcpy(g_buf2, g_buf1);
    }

  end = perf_gettime();
  printf("strcpy(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strchr
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCHR
static int test_strchr(void)
{
  int align;
  int si;
  int size;
  int fail = 0;
  FAR char *p;

  printf("Testing strchr...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';

          /* Find first char (pattern starts with 'A' at align) */

          p = strchr(g_buf1 + align, g_buf1[align]);
          if (p != g_buf1 + align)
            {
              printf("  FAIL first: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Find a unique marker placed at the last position */

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          g_buf1[align + size - 1] = 0x7e;
          p = strchr(g_buf1 + align, 0x7e);
          if (p != g_buf1 + align + size - 1)
            {
              printf("  FAIL last: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Find NUL terminator */

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          p = strchr(g_buf1 + align, '\0');
          if (p != g_buf1 + align + size)
            {
              printf("  FAIL nul: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Not found (full scan) */

          p = strchr(g_buf1 + align, 0x01);
          if (p != NULL)
            {
              printf("  FAIL notfound: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("strchr: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strchr(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)strchr(g_buf1, g_buf1[127]);
    }

  end = perf_gettime();
  printf("strchr(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strncmp
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRNCMP
static int test_strncmp(void)
{
  int align;
  int si;
  int size;
  int fail = 0;

  printf("Testing strncmp...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          fill_pattern(g_buf2 + align, size);
          g_buf2[align + size] = '\0';

          /* Equal within n */

          if (strncmp(g_buf1 + align, g_buf2 + align, size) != 0)
            {
              printf("  FAIL equal: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Differ at last position */

          g_buf2[align + size - 1] = '~';
          if (strncmp(g_buf1 + align, g_buf2 + align, size) >= 0)
            {
              printf("  FAIL less: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Equal when n is smaller than the difference */

          fill_pattern(g_buf2 + align, size);
          g_buf2[align + size] = '\0';
          g_buf2[align + size - 1] = '~';
          if (size > 1 &&
              strncmp(g_buf1 + align, g_buf2 + align, size - 1) != 0)
            {
              printf("  FAIL partial: align=%d size=%d\n", align, size);
              fail++;
            }

          /* n boundary: 0 and 1 (reset buf2 to match buf1 first) */

          fill_pattern(g_buf2 + align, size);
          g_buf2[align + size] = '\0';

          if (strncmp(g_buf1 + align, g_buf2 + align, 0) != 0)
            {
              printf("  FAIL zero: align=%d size=%d\n", align, size);
              fail++;
            }

          if (strncmp(g_buf1 + align, g_buf2 + align, 1) != 0)
            {
              printf("  FAIL one: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("strncmp: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strncmp(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  fill_pattern(g_buf2, 128);
  g_buf2[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = strncmp(g_buf1, g_buf2, 128);
    }

  end = perf_gettime();
  printf("strncmp(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strnlen
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRNLEN
static int test_strnlen(void)
{
  int align;
  int si;
  int size;
  int fail = 0;

  printf("Testing strnlen...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';

          /* maxlen >= actual length */

          if ((int)strnlen(g_buf1 + align, size + 10) != size)
            {
              printf("  FAIL full: align=%d size=%d\n", align, size);
              fail++;
            }

          /* maxlen == actual length */

          if ((int)strnlen(g_buf1 + align, size) != size)
            {
              printf("  FAIL exact: align=%d size=%d\n", align, size);
              fail++;
            }

          /* maxlen < actual length */

          if (size > 0 &&
              (int)strnlen(g_buf1 + align, size - 1) != size - 1)
            {
              printf("  FAIL trunc: align=%d size=%d\n", align, size);
              fail++;
            }

          /* maxlen == 0 */

          if (strnlen(g_buf1 + align, 0) != 0)
            {
              printf("  FAIL zero: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("strnlen: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strnlen(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = strnlen(g_buf1, 256);
    }

  end = perf_gettime();
  printf("strnlen(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strncpy
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRNCPY
static int test_strncpy(void)
{
  int size;
  int i;
  int fail = 0;
  int ai;

  printf("Testing strncpy...\n");

  /* ai encodes da*8+sa so src/dst offsets vary independently: sa!=da forces
   * different congruence and exercises the shift-merge path.
   */

  for (ai = 0; ai < 64; ai++)
    {
      int da = ai / 8;
      int sa = ai % 8;

      for (size = 1; size <= 64; size++)
        {
          fill_pattern(g_buf1 + sa, size);
          g_buf1[sa + size] = '\0';

          /* n > strlen: should copy and zero-fill */

          memset(g_buf2, 0xaa, sizeof(g_buf2));
          strncpy(g_buf2 + da, g_buf1 + sa, size + 4);
          if (strcmp(g_buf2 + da, g_buf1 + sa) != 0)
            {
              printf("  FAIL copy: sa=%d da=%d size=%d\n", sa, da, size);
              fail++;
            }

          /* Check zero-fill */

          for (i = 0; i < 4; i++)
            {
              if (g_buf2[da + size + i] != '\0')
                {
                  printf("  FAIL zfill: sa=%d da=%d size=%d\n",
                         sa, da, size);
                  fail++;
                  break;
                }
            }

          /* n < strlen: should truncate without NUL */

          if (size > 1)
            {
              memset(g_buf2, 0xaa, sizeof(g_buf2));
              strncpy(g_buf2 + da, g_buf1 + sa, size - 1);
              if (memcmp(g_buf2 + da, g_buf1 + sa, size - 1) != 0)
                {
                  printf("  FAIL trunc: sa=%d da=%d size=%d\n",
                         sa, da, size);
                  fail++;
                }
            }
        }
    }

  printf("strncpy: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strncpy(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)strncpy(g_buf2, g_buf1, 128);
    }

  end = perf_gettime();
  printf("strncpy(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_stpcpy
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STPCPY
static int test_stpcpy(void)
{
  int size;
  int fail = 0;
  int ai;
  FAR char *p;

  printf("Testing stpcpy...\n");

  /* ai encodes da*8+sa so src/dst offsets vary independently: sa!=da forces
   * different congruence and exercises the shift-merge path.
   */

  for (ai = 0; ai < 64; ai++)
    {
      int da = ai / 8;
      int sa = ai % 8;

      for (size = 1; size <= 64; size++)
        {
          fill_pattern(g_buf1 + sa, size);
          g_buf1[sa + size] = '\0';
          memset(g_buf2, 0, sizeof(g_buf2));
          p = stpcpy(g_buf2 + da, g_buf1 + sa);

          /* Check content */

          if (strcmp(g_buf2 + da, g_buf1 + sa) != 0)
            {
              printf("  FAIL copy: sa=%d da=%d size=%d\n", sa, da, size);
              fail++;
            }

          /* Check return value points to NUL */

          if (p != g_buf2 + da + size)
            {
              printf("  FAIL retval: sa=%d da=%d size=%d\n", sa, da, size);
              fail++;
            }

          if (*p != '\0')
            {
              printf("  FAIL nul: sa=%d da=%d size=%d\n", sa, da, size);
              fail++;
            }
        }
    }

  printf("stpcpy: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_stpcpy(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)stpcpy(g_buf2, g_buf1);
    }

  end = perf_gettime();
  printf("stpcpy(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strcat
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCAT
static int test_strcat(void)
{
  int size;
  int fail = 0;

  printf("Testing strcat...\n");
  for (size = 1; size <= 64; size++)
    {
      /* Build expected: "ABCD..." + "ABCD..." */

      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';

      memset(g_buf2, 0, sizeof(g_buf2));
      fill_pattern(g_buf2, size);
      g_buf2[size] = '\0';

      strcat(g_buf2, g_buf1);

      /* Check total length */

      if ((int)strlen(g_buf2) != size * 2)
        {
          printf("  FAIL len: size=%d got=%d\n", size,
                 (int)strlen(g_buf2));
          fail++;
        }

      /* Check second half matches */

      if (memcmp(g_buf2 + size, g_buf1, size) != 0)
        {
          printf("  FAIL content: size=%d\n", size);
          fail++;
        }

      /* Test cat to empty string */

      g_buf2[0] = '\0';
      strcat(g_buf2, g_buf1);
      if (strcmp(g_buf2, g_buf1) != 0)
        {
          printf("  FAIL empty: size=%d\n", size);
          fail++;
        }
    }

  printf("strcat: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strcat(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 64);
  g_buf1[64] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_buf2[0] = '\0';
      g_sink = (uintptr_t)strcat(g_buf2, g_buf1);
    }

  end = perf_gettime();
  printf("strcat(64) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strrchr
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRRCHR
static int test_strrchr(void)
{
  int align;
  int si;
  int size;
  int fail = 0;
  FAR char *p;

  printf("Testing strrchr...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';

          /* Find last occurrence of first char (repeats every 26) */

          p = strrchr(g_buf1 + align, 'A');
          if (p == NULL)
            {
              printf("  FAIL found: align=%d size=%d\n", align, size);
              fail++;
            }
          else
            {
              /* 'A' appears at 0, 26, 52, ... — last one <= size-1 */

              int expected = ((size - 1) / 26) * 26;
              if (p != g_buf1 + align + expected)
                {
                  printf("  FAIL pos: align=%d size=%d expected=%d"
                         " got=%d\n", align, size, expected,
                         (int)(p - (g_buf1 + align)));
                  fail++;
                }
            }

          /* Find NUL */

          p = strrchr(g_buf1 + align, '\0');
          if (p != g_buf1 + align + size)
            {
              printf("  FAIL nul: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Not found (full scan) */

          p = strrchr(g_buf1 + align, 0x01);
          if (p != NULL)
            {
              printf("  FAIL notfound: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("strrchr: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strrchr(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)strrchr(g_buf1, g_buf1[127]);
    }

  end = perf_gettime();
  printf("strrchr(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Name: test_strchrnul
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCHRNUL
static int test_strchrnul(void)
{
  int align;
  int si;
  int size;
  int fail = 0;
  FAR char *p;

  printf("Testing strchrnul...\n");
  for (align = 0; align < 8; align++)
    {
      for (si = 0; si < nitems(g_boundary_sizes); si++)
        {
          size = g_boundary_sizes[si];
          if (size < 1)
            {
              continue;
            }

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';

          /* Find first char (pattern starts with 'A' at align) */

          p = strchrnul(g_buf1 + align, g_buf1[align]);
          if (p != g_buf1 + align)
            {
              printf("  FAIL first: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Find a unique marker placed at the last position */

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          g_buf1[align + size - 1] = 0x7e;
          p = strchrnul(g_buf1 + align, 0x7e);
          if (p != g_buf1 + align + size - 1)
            {
              printf("  FAIL last: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Find NUL terminator (always succeeds) */

          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          p = strchrnul(g_buf1 + align, '\0');
          if (p != g_buf1 + align + size)
            {
              printf("  FAIL nul: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Not found: must return pointer to the NUL terminator */

          p = strchrnul(g_buf1 + align, 0x01);
          if (p != g_buf1 + align + size)
            {
              printf("  FAIL notfound: align=%d size=%d\n", align, size);
              fail++;
            }
        }
    }

  printf("strchrnul: %s\n", fail ? "FAILED" : "PASSED");
  return fail;
}

static void speed_strchrnul(void)
{
  clock_t start;
  clock_t end;
  int i;

  fill_pattern(g_buf1, 128);
  g_buf1[128] = '\0';
  start = perf_gettime();
  for (i = 0; i < TEST_REPEAT; i++)
    {
      g_sink = (uintptr_t)strchrnul(g_buf1, g_buf1[127]);
    }

  end = perf_gettime();
  printf("strchrnul(128) avg cycles: %ju\n",
         (uintmax_t)(end - start) / TEST_REPEAT);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fail = 0;

#ifdef CONFIG_TESTING_ARCH_LIBC_MEMCPY
  fail += test_memcpy();
  speed_memcpy();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_MEMMOVE
  fail += test_memmove();
  speed_memmove();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_MEMSET
  fail += test_memset();
  speed_memset();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_MEMCMP
  fail += test_memcmp();
  speed_memcmp();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_MEMCHR
  fail += test_memchr();
  speed_memchr();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRLEN
  fail += test_strlen();
  speed_strlen();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRCMP
  fail += test_strcmp();
  speed_strcmp();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRCPY
  fail += test_strcpy();
  speed_strcpy();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRCHR
  fail += test_strchr();
  speed_strchr();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRNCMP
  fail += test_strncmp();
  speed_strncmp();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRNLEN
  fail += test_strnlen();
  speed_strnlen();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRNCPY
  fail += test_strncpy();
  speed_strncpy();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STPCPY
  fail += test_stpcpy();
  speed_stpcpy();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRCAT
  fail += test_strcat();
  speed_strcat();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRRCHR
  fail += test_strrchr();
  speed_strrchr();
#endif
#ifdef CONFIG_TESTING_ARCH_LIBC_STRCHRNUL
  fail += test_strchrnul();
  speed_strchrnul();
#endif

  printf("arch_libc_test %s\n", fail ? "Failed" : "Passed");
  return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
