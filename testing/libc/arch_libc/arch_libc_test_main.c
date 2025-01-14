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
#include <debug.h>
#include <assert.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCPY
#  define TEST_MAX_STRING_LEN    128
#  define TEST_SHORT_SRC_LEN     50
#  define TEST_LONG_SRC_LEN      128
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCPY
static char g_test_src_str[TEST_MAX_STRING_LEN];
static char g_test_dst_str[TEST_MAX_STRING_LEN];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_TESTING_ARCH_LIBC_STRCPY

/****************************************************************************
 * Name: arch_libc_test_strcpy
 ****************************************************************************/

void init_short_test_str(int dst_offset, int src_offset)
{
  int off;

  memset(g_test_src_str, '\0', sizeof(g_test_src_str));
  memset(g_test_dst_str, '\0', sizeof(g_test_dst_str));

  for (off = 0; off < TEST_SHORT_SRC_LEN; off++)
    {
      g_test_src_str[off + src_offset] = '0' + off % 10;
    }
}

/****************************************************************************
 * Name: arch_libc_test_strcpy_offset
 ****************************************************************************/

int arch_libc_test_strcpy_offset(int dst_offset, int src_offset)
{
  int off;
  FAR char *dest;
  FAR char *src;
  FAR char *result_str;
  bool pass = true;

  init_short_test_str(dst_offset, src_offset);
  dest = g_test_dst_str + dst_offset;
  src = g_test_src_str + src_offset;
  result_str = strcpy(dest, src);

  /* check strcpy data */

  for (off = 0; off < TEST_SHORT_SRC_LEN; off++)
    {
      if (result_str[off] != '0' + off % 10)
        {
          pass = false;
          printf("dest copied data error, index %d\n", off);
        }

      if (src[off] != '0' + off % 10)
        {
          pass = false;
          printf("src data error, index %d\n", off);
        }
    }

  for (off = TEST_SHORT_SRC_LEN; off < TEST_MAX_STRING_LEN - dst_offset;
       off++)
    {
      if (result_str[off] != '\0')
        {
          pass = false;
          printf("dest tailing zero error, index %d\n", off);
        }
    }

  for (off = TEST_SHORT_SRC_LEN; off < TEST_MAX_STRING_LEN - src_offset;
       off++)
    {
      if (src[off] != '\0')
        {
          pass = false;
          printf("src tailing zero error, index %d\n", off);
        }
    }

  /* strcpy shouln't change arch_libc_test_strcpy's local variable */

  if (dest != (g_test_dst_str + dst_offset) ||
      src != (g_test_src_str + src_offset))
    {
      pass = false;
      printf("local var changed after calling strcpy\n");
    }

  if (!pass)
    {
      printf("Test Failed at dst/src offset [%d, %d]\n", dst_offset,
             src_offset);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: arch_libc_test_strcpy
 ****************************************************************************/

int arch_libc_test_strcpy(void)
{
  int dest_off;
  int src_off;
  int ret = 0;

  for (dest_off = 0; dest_off <= 4; dest_off++)
    {
      for (src_off = 0; src_off <= 4; src_off++)
        {
          if (arch_libc_test_strcpy_offset(dest_off, src_off) != 0)
            {
              ret = -1;
            }
        }
    }

  if (ret != 0)
    {
      printf("arch_libc_test_strcpy Test Failed\n");
    }
  else
    {
      printf("arch_libc_test_strcpy Test Passed\n");
    }

  return ret;
}

/****************************************************************************
 * Name: arch_libc_strcpy_speed_offset
 ****************************************************************************/

clock_t arch_libc_strcpy_speed_offset(int dst_offset, int src_offset)
{
  FAR char *dest;
  FAR char *src;
  clock_t start;
  clock_t end;

  init_short_test_str(dst_offset, src_offset);
  dest = g_test_dst_str + dst_offset;
  src = g_test_src_str + src_offset;
  start = perf_gettime();
  strcpy(dest, src);
  end = perf_gettime();

  return end - start;
}

/****************************************************************************
 * Name: arch_libc_strcpy_speed
 ****************************************************************************/

int arch_libc_strcpy_speed(void)
{
  int dest_off;
  int src_off;
  clock_t cycles = 0;

  for (dest_off = 0; dest_off <= 4; dest_off++)
    {
      for (src_off = 0; src_off <= 4; src_off++)
        {
          cycles += arch_libc_strcpy_speed_offset(dest_off, src_off);
        }
    }

  printf("strcpy total(run 25 times) cpu cycles %" PRIu32 "\n", cycles);
  printf("strcpy average cpu cycles %" PRIu32 "\n", cycles / 25);

  return 0;
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
#ifdef CONFIG_TESTING_ARCH_LIBC_STRCPY
  arch_libc_test_strcpy();
  arch_libc_strcpy_speed();
#endif

  return 0;
}

