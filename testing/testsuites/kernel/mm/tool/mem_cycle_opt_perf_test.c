/****************************************************************************
 * apps/testing/testsuites/kernel/mm/tool/mem_cycle_opt_perf_test.c
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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include "MmTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(void)
{
  printf(
      "\nUsage: mm_stress_test  <min_size>  <max_size>  <test_num> "
      "<delay_time>\n");
  printf("\nWhere:\n");
  printf("  <min_size>    Minimum number of memory requests.\n");
  printf("  <max_size>    Maximum number of memory requests.\n");
  printf("  <test_num>    Number of tests.\n");
  printf("  <delay_time>  Malloc delay time, Unit: microseconds.\n");
}

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int malloc_size;
  int mallc_min_size; /* The minimum memory length requested in the test
                       */
  int mallc_max_size; /* The maximum memory length requested in the test
                       */
  int test_num;
  int application_delay_time = 1; /* Delay in test */

  /* Memory write content check character */

  char check_character;
  char *address_ptr = NULL;
  struct timespec t_start;
  struct timespec t_end;
  long timedif;

  if (argc < 5)
    {
      syslog(LOG_WARNING, "Missing required arguments\n");
      show_usage();
      exit(1);
    }

  mallc_min_size = atoi(argv[1]);
  mallc_max_size = atoi(argv[2]);
  test_num = atoi(argv[3]);
  application_delay_time = atoi(argv[4]);

  for (int i = 0; i < test_num; i++)
    {
      srand(i + gettid());
      malloc_size = mmtest_get_rand_size(mallc_min_size, mallc_max_size);
      check_character = 0x65;
      clock_gettime(CLOCK_MONOTONIC, &t_start);
      address_ptr = (char *)malloc(malloc_size * sizeof(char));
      clock_gettime(CLOCK_MONOTONIC, &t_end);
      timedif = 1000000 * (t_end.tv_sec - t_start.tv_sec) +
                (t_end.tv_nsec - t_start.tv_nsec) / 1000;
      if (address_ptr != NULL)
        {
          syslog(LOG_INFO,
                 "[Test malloc] (address:%p size:%d) takes:%ld "
                 "microseconds\n",
                 address_ptr, malloc_size, timedif);
          memset(address_ptr, check_character, malloc_size);
        }

      else
        {
          syslog(LOG_ERR,
                 "Malloc failed ! The remaining memory may be "
                 "insufficient\n");
          syslog(LOG_ERR, "Continue to test !!\n");
          continue;
        }

      /* Checking Content Consistency */

      for (int j = 0; j < malloc_size; j++)
        {
          if (address_ptr[j] != check_character)
            {
              syslog(LOG_ERR, "ERROR:Inconsistent content checking\n");
              free(address_ptr);
              return -1;
            }
        }

      clock_gettime(CLOCK_MONOTONIC, &t_start);

      /* Free test memory */

      clock_gettime(CLOCK_MONOTONIC, &t_end);
      timedif = 1000000 * (t_end.tv_sec - t_start.tv_sec) +
                (t_end.tv_nsec - t_start.tv_nsec) / 1000;
      syslog(
          LOG_INFO,
          "[Test free] (address:%p size:%d) takes:%ld microseconds\n\n",
          address_ptr, malloc_size, timedif);
      free(address_ptr);
      usleep(application_delay_time);
    }

  return 0;
}
