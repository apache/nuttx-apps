/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_007.c
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
#include <stdlib.h>
#include <syslog.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <malloc.h>
#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MEMORY_LIST_LENGTH 200

/* Random size range, we will apply the memory size in this range */

#define MALLOC_MIN_SIZE 32
#define MALLOC_MAX_SIZE 2048

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm07
 ****************************************************************************/

void test_nuttx_mm07(FAR void **state)
{
  char *mem_list[MEMORY_LIST_LENGTH];
  char *tmp_str = NULL;
  int malloc_size;
  int index = 0;
  int total_size = 0;
  int flag = 0;
  struct mallinfo test_befor_info;
  struct mallinfo test_during_info;
  struct mallinfo test_after_info;
  struct mallinfo mem_info;
  memset(&mem_info, 0, sizeof(mem_info));
  for (int i = 0; i < MEMORY_LIST_LENGTH; i++)
    {
      mem_list[i] = NULL;
    }

  test_befor_info = mallinfo();

  /* get a random size */

  for (int k = 0; k < MEMORY_LIST_LENGTH; k++)
    {
      get_mem_info(&mem_info);
      malloc_size =
          mmtest_get_rand_size(MALLOC_MIN_SIZE, MALLOC_MAX_SIZE);
      if (mem_info.mxordblk - 16 < 0)
        {
          syslog(LOG_INFO,
                 "TEST END because of the mem_info.mxordblk is:%d",
                 mem_info.mxordblk);
          break;
        }

      if (malloc_size > mem_info.mxordblk - 16)
        {
          syslog(LOG_INFO, "SET memsize to:%d", mem_info.mxordblk - 16);
          malloc_size = mem_info.mxordblk - 16;
        }

      if (malloc_size > 0)
        {
          tmp_str = (char *)malloc(malloc_size * sizeof(char));
        }

      else
        {
          malloc_size = 512;
          tmp_str = (char *)malloc(malloc_size * sizeof(char));
        }

      mem_list[k] = tmp_str;
      if (tmp_str != NULL)
        {
          memset(tmp_str, 0x67, malloc_size);
          total_size = total_size + malloc_size;
          for (int j = 0; j < malloc_size; j++)
            {
              if (*tmp_str++ != 0x67)
                {
                  syslog(LOG_ERR, "check error !\n");
                  flag = 1;
                }
            }
        }
    }

  test_during_info = mallinfo();

  /* Random memory release */

  for (int n = 0; n < MEMORY_LIST_LENGTH; n++)
    {
      srand(n);
      index = rand() % 2;
      if (index == 0 && mem_list[n] != NULL)
        {
          free(mem_list[n]);
          mem_list[n] = NULL;
        }
    }

  for (int l = 0; l < MEMORY_LIST_LENGTH; l++)
    {
      if (mem_list[l] != NULL)
        free(mem_list[l]);
    }

  test_after_info = mallinfo();
  assert_int_in_range(test_befor_info.fordblks,
                      test_during_info.fordblks, test_befor_info.arena);
  assert_int_in_range(test_after_info.fordblks,
                      test_during_info.fordblks, test_befor_info.arena);
  assert_int_equal(flag, 0);
}
