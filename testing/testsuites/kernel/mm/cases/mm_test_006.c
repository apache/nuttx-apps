/****************************************************************************
 * apps/testing/testsuites/kernel/mm/cases/mm_test_006.c
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
#include "MmTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Random size range, we will apply the memory size in this range */

#define MALLOC_MIN_SIZE 64
#define MALLOC_MAX_SIZE 8192

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm06
 ****************************************************************************/

void test_nuttx_mm06(FAR void **state)
{
  int malloc_size;
  int test_num = 1000;
  char check_character = 0x67; /* Memory write content check character */
  char *address_ptr = NULL;

  for (int i = 0; i < test_num; i++)
    {
      srand(i + gettid());

      /* Produces a random size */

      malloc_size =
          mmtest_get_rand_size(MALLOC_MIN_SIZE, MALLOC_MAX_SIZE);
      address_ptr = (char *)malloc(malloc_size * sizeof(char));
      assert_non_null(address_ptr);
      memset(address_ptr, check_character, malloc_size);

      /* Checking Content Consistency */

      for (int j = 0; j < malloc_size; j++)
        {
          if (address_ptr[j] != check_character)
            {
              free(address_ptr);
              fail_msg("check fail !\n");
            }
        }

      /* Free test memory */

      free(address_ptr);
    }
}
