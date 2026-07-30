/****************************************************************************
 * apps/examples/fdpicxip/modules/funcdesc.c
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

/* Exercises R_ARM_FUNCDESC -- the relocation that asks the loader to
 * manufacture a function descriptor and hand back its address.
 *
 * It is emitted for an *address-taken imported* function, which is a
 * different thing from a called one.  A call gets a PLT entry and an
 * R_ARM_FUNCDESC_VALUE relocation on a descriptor the linker already laid
 * out; taking the address instead needs a pointer to a descriptor that does
 * not exist yet, so the loader carves one out of the pool it reserves
 * behind the writable segment.  No other module here emits one, which is
 * why this exists.
 *
 * Every pointer here is called *through*, not merely printed: a descriptor
 * with the wrong entry or the wrong GOT half only shows up when it is
 * branched through.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef int   (*atoifn)(const char *);
typedef void *(*memcpyfn)(void *, const void *, size_t);
typedef int   (*strcmpfn)(const char *, const char *);
typedef void  (*qsortfn)(void *, size_t, size_t,
                         int (*)(const void *, const void *));

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* volatile is load bearing, and the reason is worth knowing before anyone
 * tidies it away.
 *
 * Without it these are static pointers that are never written, so GCC
 * constant-propagates them, turns every indirect call back into a direct
 * one, and deletes the variables.  The descriptor address then never has to
 * materialise, the linker emits R_ARM_FUNCDESC_VALUE for the direct calls
 * instead, and not one R_ARM_FUNCDESC survives -- the module still builds,
 * still runs, still passes, and tests nothing it claims to.
 *
 * volatile forces each pointer to be loaded from memory at the point of
 * call, which is what makes the slot genuinely hold the address of a
 * descriptor the loader has to supply.
 */

static atoifn   volatile g_atoi   = atoi;
static memcpyfn volatile g_memcpy = memcpy;
static strcmpfn volatile g_strcmp = strcmp;
static qsortfn  volatile g_qsort  = qsort;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* A local function's address is R_ARM_FUNCDESC_VALUE, not FUNCDESC -- the
 * linker can lay that descriptor out itself.  Kept here so the module
 * exercises both kinds side by side.
 */

static int cmp_int(const void *a, const void *b)
{
  return *(const int *)a - *(const int *)b;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int  v[5] =
  {
    5, 3, 1, 4, 2
  };

  int  copy[5];
  char buf[8];
  int  seed;
  int  fails = 0;
  int  i;

  seed = g_atoi((argc > 1) ? argv[1] : "7");
  if (seed != 7 && seed != 1 && seed != 2)
    {
      syslog(LOG_INFO, "[funcdesc] FAIL atoi through a descriptor: %d\n",
             seed);
      fails++;
    }

  g_memcpy(copy, v, sizeof(v));
  for (i = 0; i < 5; i++)
    {
      if (copy[i] != v[i])
        {
          syslog(LOG_INFO, "[funcdesc] FAIL memcpy through a descriptor\n");
          fails++;
          break;
        }
    }

  g_memcpy(buf, "abcdef", 7);
  if (g_strcmp(buf, "abcdef") != 0)
    {
      syslog(LOG_INFO, "[funcdesc] FAIL strcmp through a descriptor\n");
      fails++;
    }

  /* A manufactured descriptor for qsort, called with a descriptor for a
   * function of our own -- so the firmware calls back into module code
   * that has to arrive with this module's data base in the FDPIC register.
   */

  g_qsort(copy, 5, sizeof(int), cmp_int);
  for (i = 0; i < 5; i++)
    {
      if (copy[i] != i + 1)
        {
          syslog(LOG_INFO, "[funcdesc] FAIL qsort through a descriptor\n");
          fails++;
          break;
        }
    }

  syslog(LOG_INFO, "[funcdesc] seed %d, %d failures\n", seed, fails);
  return fails == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
