/****************************************************************************
 * apps/examples/pffault/pffault_main.c
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Deliberately touch a kernel-space address from an unprivileged (WORLD1)
 *   user task to exercise the ESP32-S3 PMS isolation boundary.  In a working
 *   protected build this raises a precise Load/StoreProhibited fault that
 *   the kernel's recoverable-fault dispatcher must handle (terminating just
 *   this task); the shell should survive.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Default target: the base of the kernel DRAM region, to which the user
   * world has no PMS permission.  A second argument "w" makes it a store.
   */

  volatile uint32_t *kaddr = (volatile uint32_t *)0x3fc98000;
  bool store = (argc > 1 && argv[1][0] == 'w');

  if (argc > 2)
    {
      kaddr = (volatile uint32_t *)strtoul(argv[2], NULL, 0);
    }

  printf("pffault: user-space %s of kernel addr %p ...\n",
         store ? "write" : "read", (void *)kaddr);
  fflush(stdout);

  if (store)
    {
      *kaddr = 0xdeadbeef;   /* Expect a precise StoreProhibited (WORLD1) */
    }
  else
    {
      uint32_t v = *kaddr;   /* Expect a precise LoadProhibited (WORLD1) */
      printf("pffault: SURVIVED unexpectedly, read %08lx\n",
             (unsigned long)v);
    }

  printf("pffault: returned from the faulting access (unexpected)\n");
  return 0;
}
