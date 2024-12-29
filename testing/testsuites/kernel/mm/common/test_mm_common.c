/****************************************************************************
 * apps/testing/testsuites/kernel/mm/common/test_mm_common.c
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
#include <malloc.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include "MmTest.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_rand_size
 ****************************************************************************/

int mmtest_get_rand_size(int min, int max)
{
  int rval = 1;
  if (min > 0 && max > 0 && (max - min) > 0)
    {
      rval = rand() % (max - min) + min;
    }

  return rval;
}

/****************************************************************************
 * Name: mm_showmallinfo
 ****************************************************************************/

void mmtest_showmallinfo(void)
{
  struct mallinfo alloc_info;
  alloc_info = mallinfo();
  syslog(LOG_INFO, "     mallinfo:\n");
  syslog(LOG_INFO, "       Total space allocated from system = %lu\n",
         (unsigned long)alloc_info.arena);
  syslog(LOG_INFO, "       Number of non-inuse chunks        = %lu\n",
         (unsigned long)alloc_info.ordblks);
  syslog(LOG_INFO, "       Largest non-inuse chunk           = %lu\n",
         (unsigned long)alloc_info.mxordblk);
  syslog(LOG_INFO, "       Total allocated space             = %lu\n",
         (unsigned long)alloc_info.uordblks);
  syslog(LOG_INFO, "       Total non-inuse space             = %lu\n",
         (unsigned long)alloc_info.fordblks);
}

/****************************************************************************
 * Name: mmtest_get_memsize
 ****************************************************************************/

unsigned long mmtest_get_memsize(void)
{
#ifdef CONFIG_ARCH_SIM
  return 2048;
#else
  unsigned long memsize;
  unsigned long mem_largest;
  struct mallinfo alloc_info;

  alloc_info = mallinfo();
  mem_largest = alloc_info.mxordblk;
  memsize = mem_largest * 0.5;

  /* During the test, the maximum memory is less than 2M */

  if (memsize > 1024 * 1024 * 2)
    {
      memsize = 1024 * 1024 * 2;
    }

  return memsize;
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_mm_setup
 ****************************************************************************/

int test_nuttx_mm_setup(void **state)
{
  return 0;
}

/****************************************************************************
 * Name: test_nuttx_mm_teardown
 ****************************************************************************/

int test_nuttx_mm_teardown(void **state)
{
  return 0;
}
