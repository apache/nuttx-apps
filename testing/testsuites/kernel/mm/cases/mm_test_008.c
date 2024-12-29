/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_008.c
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
#include <sys/stat.h>
#include <sys/wait.h>
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

static int test_nuttx_08_routine_1(int argc, char *argv[])
{
  char *ptr = NULL;
  char *tmp_ptr = NULL;
  int malloc_size;
  int flag = 0;

  for (int n = 0; n < 1000; n++)
    {
      malloc_size = mmtest_get_rand_size(64, 512);
      tmp_ptr = ptr = malloc(sizeof(char) * malloc_size);
      if (ptr == NULL)
        {
          flag = 1;
          break;
        }

      for (int i = 0; i < malloc_size; i++)
        *tmp_ptr++ = 'X';
      tmp_ptr = ptr;
      for (int j = 0; j < malloc_size; j++)
        {
          if (*tmp_ptr++ != 'X')
            {
              flag = 1;
            }
        }

      free(ptr);
    }

  assert_int_equal(flag, 0);
  return 0;
}

static int test_nuttx_08_routine_2(int argc, char *argv[])
{
  char *temp_ptr = NULL;
  int flag = 0;

  for (int n = 0; n < 1000; n++)
    {
      temp_ptr = memalign(64, 1024 * sizeof(char));
      if (temp_ptr == NULL)
        {
          flag = 1;
          break;
        }

      assert_non_null(temp_ptr);

      memset(temp_ptr, 0x33, 1024 * sizeof(char));
      free(temp_ptr);
    }

  assert_int_equal(flag, 0);
  return 0;
}

static int test_nuttx_08_routine_3(int argc, char *argv[])
{
  char *pm;
  unsigned long memsize;
  struct mallinfo mem_info;
  memset(&mem_info, 0, sizeof(mem_info));
  for (int i = 0; i < 500; i++)
    {
      /* Apply for as much memory as a system allows */

      get_mem_info(&mem_info);
      memsize = mmtest_get_rand_size(1024, 2048);
      if (mem_info.mxordblk - 16 < 0)
        {
          syslog(LOG_INFO,
                 "TEST END because of the mem_info.mxordblk is:%d",
                 mem_info.mxordblk);
          break;
        }

      if (memsize > mem_info.mxordblk - 16)
        {
          syslog(LOG_INFO, "SET memsize to:%d", mem_info.mxordblk - 16);
          memsize = mem_info.mxordblk - 16;
        }

      pm = malloc(memsize);
      assert_non_null(pm);
      free(pm);
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm08
 ****************************************************************************/

void test_nuttx_mm08(FAR void **state)
{
  pid_t pid;
  int status;

  pid = task_create("test_nuttx_08_routine_1", TASK_PRIORITY,
                    DEFAULT_STACKSIZE, test_nuttx_08_routine_1, NULL);
  assert_true(pid > 0);
  pid = task_create("test_nuttx_08_routine_2", TASK_PRIORITY,
                    DEFAULT_STACKSIZE, test_nuttx_08_routine_2, NULL);
  assert_true(pid > 0);
  pid = task_create("test_nuttx_08_routine_3", TASK_PRIORITY,
                    DEFAULT_STACKSIZE, test_nuttx_08_routine_3, NULL);
  assert_true(pid > 0);
  waitpid(pid, &status, 0);
}
