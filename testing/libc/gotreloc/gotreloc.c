/****************************************************************************
 * apps/testing/libc/gotreloc/gotreloc.c
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
#include <string.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

int g_gotreloc_data = 5;
extern int g_gotreloc_missing_data weak_data;
extern void gotreloc_missing_function(void) weak_function;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_fails;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void check(FAR const char *what, int ok)
{
  g_fails += !ok;
  printf("gotreloc: %-48s %s\n", what, ok ? "PASS" : "FAIL");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  size_t (*volatile fn)(FAR const char *) = strlen;
  FAR int *viagot;
  FAR int *direct;

  /* The GOT slot and a PC-relative address must agree */

  __asm__ volatile("adrp %0, :got:g_gotreloc_data\n"
                   "ldr  %0, [%0, #:got_lo12:g_gotreloc_data]\n"
                   : "=r"(viagot));
  __asm__ volatile("adrp %0, g_gotreloc_data\n"
                   "add  %0, %0, #:lo12:g_gotreloc_data\n"
                   : "=r"(direct));
  check("GOT slot holds the address of the data", viagot == direct);
  *viagot = 7;
  check("a write through the slot reaches the data", g_gotreloc_data == 7);

  /* What -fPIC makes */

  check("libc function address through the GOT", fn("abcd") == 4);
  check("undefined weak data is NULL", &g_gotreloc_missing_data == NULL);
  check("undefined weak function is NULL",
        gotreloc_missing_function == NULL);

  printf("gotreloc: %s\n", g_fails != 0 ? "FAIL" : "PASS");
  return g_fails != 0;
}
