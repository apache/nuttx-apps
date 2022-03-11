/****************************************************************************
 * apps/testing/ostest/setjmp.c
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

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void setjmp_test(void)
{
  int value;
  jmp_buf buf;

  printf("setjmp_test: Initializing jmp_buf\n");

  if ((value = setjmp(buf)) == 0)
    {
      printf("setjmp_test: Try jump\n");
      longjmp(buf, 123);

      /* Unreachable */

      ASSERT(0);
    }
  else
    {
      ASSERT(value == 123);
      printf("setjmp_test: Jump succeed\n");
    }
}
