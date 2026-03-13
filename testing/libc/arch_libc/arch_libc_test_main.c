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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_BUF_SIZE  256
#define TEST_REPEAT    100
#define MAX_ALIGN      16

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_buf1[TEST_BUF_SIZE + MAX_ALIGN];
static char g_buf2[TEST_BUF_SIZE + MAX_ALIGN];
static volatile uintptr_t g_sink;

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
  int size;
  int fail = 0;

  printf("Testing memmove...\n");

  /* Test overlapping forward */

  for (size = 1; size <= 64; size++)
    {
      fill_pattern(g_buf1, size + 16);
      memcpy(g_buf2, g_buf1, size + 16);
      memmove(g_buf1 + 8, g_buf1, size);
      if (memcmp(g_buf1 + 8, g_buf2, size) != 0)
        {
          printf("  FAIL forward: size=%d\n", size);
          fail++;
        }
    }

  /* Test overlapping backward */

  for (size = 1; size <= 64; size++)
    {
      fill_pattern(g_buf1 + 8, size);
      memcpy(g_buf2, g_buf1 + 8, size);
      memmove(g_buf1, g_buf1 + 8, size);
      if (memcmp(g_buf1, g_buf2, size) != 0)
        {
          printf("  FAIL backward: size=%d\n", size);
          fail++;
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
  int size;
  int fail = 0;

  printf("Testing memcmp...\n");
  for (size = 1; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      fill_pattern(g_buf2, size);
      if (memcmp(g_buf1, g_buf2, size) != 0)
        {
          printf("  FAIL equal: size=%d\n", size);
          fail++;
        }

      g_buf2[size - 1] = '~';
      if (memcmp(g_buf1, g_buf2, size) >= 0)
        {
          printf("  FAIL less: size=%d\n", size);
          fail++;
        }

      g_buf2[size - 1] = 0;
      if (memcmp(g_buf1, g_buf2, size) <= 0)
        {
          printf("  FAIL greater: size=%d\n", size);
          fail++;
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
  int size;
  int fail = 0;
  FAR void *p;

  printf("Testing memchr...\n");
  for (size = 1; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);

      /* Find last char */

      p = memchr(g_buf1, g_buf1[size - 1], size);
      if (p == NULL)
        {
          printf("  FAIL found: size=%d\n", size);
          fail++;
        }

      /* Not found */

      p = memchr(g_buf1, 0xff, size);
      if (p != NULL)
        {
          printf("  FAIL notfound: size=%d\n", size);
          fail++;
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
  int size;
  int fail = 0;

  printf("Testing strlen...\n");
  for (size = 0; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';
      if ((int)strlen(g_buf1) != size)
        {
          printf("  FAIL: size=%d got=%d\n", size, (int)strlen(g_buf1));
          fail++;
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
  int size;
  int fail = 0;

  printf("Testing strcmp...\n");
  for (size = 1; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';
      fill_pattern(g_buf2, size);
      g_buf2[size] = '\0';
      if (strcmp(g_buf1, g_buf2) != 0)
        {
          printf("  FAIL equal: size=%d\n", size);
          fail++;
        }

      g_buf2[size - 1] = '~';
      g_buf2[size] = '\0';
      if (strcmp(g_buf1, g_buf2) >= 0)
        {
          printf("  FAIL less: size=%d\n", size);
          fail++;
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
  int align;
  int size;
  int fail = 0;

  printf("Testing strcpy...\n");
  for (align = 0; align < 8; align++)
    {
      for (size = 1; size <= 64; size++)
        {
          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          memset(g_buf2, 0, sizeof(g_buf2));
          strcpy(g_buf2 + align, g_buf1 + align);
          if (strcmp(g_buf2 + align, g_buf1 + align) != 0)
            {
              printf("  FAIL: align=%d size=%d\n", align, size);
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
  int size;
  int fail = 0;
  FAR char *p;

  printf("Testing strchr...\n");
  for (size = 1; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';

      /* Find first char */

      p = strchr(g_buf1, g_buf1[0]);
      if (p != g_buf1)
        {
          printf("  FAIL first: size=%d\n", size);
          fail++;
        }

      /* Find NUL */

      p = strchr(g_buf1, '\0');
      if (p != g_buf1 + size)
        {
          printf("  FAIL nul: size=%d\n", size);
          fail++;
        }

      /* Not found */

      p = strchr(g_buf1, 0x01);
      if (p != NULL)
        {
          printf("  FAIL notfound: size=%d\n", size);
          fail++;
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
  int size;
  int fail = 0;

  printf("Testing strncmp...\n");
  for (size = 1; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';
      fill_pattern(g_buf2, size);
      g_buf2[size] = '\0';

      /* Equal within n */

      if (strncmp(g_buf1, g_buf2, size) != 0)
        {
          printf("  FAIL equal: size=%d\n", size);
          fail++;
        }

      /* Differ at last position */

      g_buf2[size - 1] = '~';
      if (strncmp(g_buf1, g_buf2, size) >= 0)
        {
          printf("  FAIL less: size=%d\n", size);
          fail++;
        }

      /* Equal when n is smaller */

      fill_pattern(g_buf2, size);
      g_buf2[size] = '\0';
      g_buf2[size - 1] = '~';
      if (size > 1 && strncmp(g_buf1, g_buf2, size - 1) != 0)
        {
          printf("  FAIL partial: size=%d\n", size);
          fail++;
        }

      /* Zero count */

      if (strncmp(g_buf1, g_buf2, 0) != 0)
        {
          printf("  FAIL zero: size=%d\n", size);
          fail++;
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
  int size;
  int fail = 0;

  printf("Testing strnlen...\n");
  for (size = 0; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';

      /* maxlen >= actual length */

      if ((int)strnlen(g_buf1, size + 10) != size)
        {
          printf("  FAIL full: size=%d\n", size);
          fail++;
        }

      /* maxlen < actual length */

      if (size > 0 && (int)strnlen(g_buf1, size - 1) != size - 1)
        {
          printf("  FAIL trunc: size=%d\n", size);
          fail++;
        }

      /* maxlen == 0 */

      if (strnlen(g_buf1, 0) != 0)
        {
          printf("  FAIL zero: size=%d\n", size);
          fail++;
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
  int align;
  int size;
  int i;
  int fail = 0;

  printf("Testing strncpy...\n");
  for (align = 0; align < 8; align++)
    {
      for (size = 1; size <= 64; size++)
        {
          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';

          /* n > strlen: should copy and zero-fill */

          memset(g_buf2, 0xaa, sizeof(g_buf2));
          strncpy(g_buf2 + align, g_buf1 + align, size + 4);
          if (strcmp(g_buf2 + align, g_buf1 + align) != 0)
            {
              printf("  FAIL copy: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Check zero-fill */

          for (i = 0; i < 4; i++)
            {
              if (g_buf2[align + size + i] != '\0')
                {
                  printf("  FAIL zfill: align=%d size=%d\n", align, size);
                  fail++;
                  break;
                }
            }

          /* n < strlen: should truncate without NUL */

          if (size > 1)
            {
              memset(g_buf2, 0xaa, sizeof(g_buf2));
              strncpy(g_buf2 + align, g_buf1 + align, size - 1);
              if (memcmp(g_buf2 + align, g_buf1 + align, size - 1) != 0)
                {
                  printf("  FAIL trunc: align=%d size=%d\n", align, size);
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
  int align;
  int size;
  int fail = 0;
  FAR char *p;

  printf("Testing stpcpy...\n");
  for (align = 0; align < 8; align++)
    {
      for (size = 1; size <= 64; size++)
        {
          fill_pattern(g_buf1 + align, size);
          g_buf1[align + size] = '\0';
          memset(g_buf2, 0, sizeof(g_buf2));
          p = stpcpy(g_buf2 + align, g_buf1 + align);

          /* Check content */

          if (strcmp(g_buf2 + align, g_buf1 + align) != 0)
            {
              printf("  FAIL copy: align=%d size=%d\n", align, size);
              fail++;
            }

          /* Check return value points to NUL */

          if (p != g_buf2 + align + size)
            {
              printf("  FAIL retval: align=%d size=%d\n", align, size);
              fail++;
            }

          if (*p != '\0')
            {
              printf("  FAIL nul: align=%d size=%d\n", align, size);
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
  int size;
  int fail = 0;
  FAR char *p;

  printf("Testing strrchr...\n");
  for (size = 1; size <= 128; size++)
    {
      fill_pattern(g_buf1, size);
      g_buf1[size] = '\0';

      /* Find last occurrence of first char (repeats every 26) */

      p = strrchr(g_buf1, 'A');
      if (p == NULL)
        {
          printf("  FAIL found: size=%d\n", size);
          fail++;
        }
      else
        {
          /* 'A' appears at 0, 26, 52, ... — last one <= size-1 */

          int expected = ((size - 1) / 26) * 26;
          if (p != g_buf1 + expected)
            {
              printf("  FAIL pos: size=%d expected=%d got=%d\n",
                     size, expected, (int)(p - g_buf1));
              fail++;
            }
        }

      /* Find NUL */

      p = strrchr(g_buf1, '\0');
      if (p != g_buf1 + size)
        {
          printf("  FAIL nul: size=%d\n", size);
          fail++;
        }

      /* Not found */

      p = strrchr(g_buf1, 0x01);
      if (p != NULL)
        {
          printf("  FAIL notfound: size=%d\n", size);
          fail++;
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

  printf("arch_libc_test %s\n", fail ? "Failed" : "Passed");
  return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}

