/****************************************************************************
 * apps/examples/nxflatxip/module/xipmod.c
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
 * The module the demo downloads into xipfs and runs.
 *
 * Given a seed on the command line it fills an array derived from it,
 * sleeps long enough for a second instance to do the same with a different
 * seed, checks its own values are untouched, and hands the result to the
 * firmware.  It reports the address of its own text, which is the point of
 * the exercise: that address is inside the memory mapped flash window, it
 * is the same in both instances, and the module is executing from it rather
 * than from a copy in RAM.
 *
 * Two things this module deliberately does not have, both because of what
 * the ldnxflat in circulation does with them:
 *
 *   - No global or static variables.  Those are reached through the GOT,
 *     and the GOT entries come out pointing into the import table rather
 *     than into .bss, so the module overwrites the addresses of the
 *     functions it imports and jumps into nothing on its next call out.
 *   - No string constants.  With this linker script .rodata lives in the
 *     text segment and is reached PC-relatively, and the addend already
 *     in the instruction is dropped when that relocation is resolved, so
 *     the pointer lands somewhere in the middle of the code.
 *
 * Hence the reporting callback: the module passes numbers to the firmware
 * and the firmware, which has a working printf and a format string, does
 * the printing.  It also exercises the other direction of the interface --
 * firmware code entered with the module's data base still in r10.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NVALUES  64

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Exported by the demo in the firmware and imported by this module */

void xipmod_report(int seed, const void *text, const void *stack,
                   long sum, int errors);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int values[NVALUES];
  int errors = 0;
  int seed = 1;
  long sum = 0;
  int i;

  if (argc > 1)
    {
      seed = atoi(argv[1]);
    }

  for (i = 0; i < NVALUES; i++)
    {
      values[i] = seed * (i + 1);
    }

  /* Long enough that the other instance has certainly filled its own copy
   * by the time this one checks.
   */

  usleep(200000);

  for (i = 0; i < NVALUES; i++)
    {
      if (values[i] != seed * (i + 1))
        {
          errors++;
        }

      sum += values[i];
    }

  xipmod_report(seed, (const void *)main, (const void *)values, sum, errors);

  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
