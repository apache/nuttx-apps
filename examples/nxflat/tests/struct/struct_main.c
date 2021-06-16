/****************************************************************************
 * apps/examples/nxflat/tests/struct/struct_main.c
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

#include <stdio.h>
#include <stdlib.h>

#include "struct.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct struct_dummy_s dummy_struct =
{
   DUMMY_SCALAR_VALUE3
};

int dummy_scalar = DUMMY_SCALAR_VALUE2;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  const struct struct_s *mystruct;

  printf("Calling getstruct()\n");
  mystruct = getstruct();
  printf("getstruct returned %p\n", mystruct);
  printf("  n = %d (vs %d) %s\n",
         mystruct->n, DUMMY_SCALAR_VALUE1,
         mystruct->n == DUMMY_SCALAR_VALUE1 ? "PASS" : "FAIL");

  printf("  pn = %p (vs %p) %s\n",
         mystruct->pn, &dummy_scalar,
         mystruct->pn == &dummy_scalar ? "PASS" : "FAIL");
  if (mystruct->pn == &dummy_scalar)
    {
      printf(" *pn = %d (vs %d) %s\n",
             *mystruct->pn, DUMMY_SCALAR_VALUE2,
             *mystruct->pn == DUMMY_SCALAR_VALUE2 ? "PASS" : "FAIL");
    }

  printf("  ps = %p (vs %p) %s\n",
         mystruct->ps, &dummy_struct,
         mystruct->ps == &dummy_struct ? "PASS" : "FAIL");
  if (mystruct->ps == &dummy_struct)
    {
      printf("  ps->n = %d (vs %d) %s\n",
             mystruct->ps->n, DUMMY_SCALAR_VALUE3,
             mystruct->ps->n == DUMMY_SCALAR_VALUE3 ? "PASS" : "FAIL");
    }

  printf("  pf = %p (vs %p) %s\n",
         mystruct->pf, dummyfunc,
         mystruct->pf == dummyfunc ? "PASS" : "FAIL");
  if (mystruct->pf == dummyfunc)
    {
      printf("Calling mystruct->pf()\n");
      mystruct->pf();
    }

  printf("Exit-ing\n");
  return 0;
}

void dummyfunc(void)
{
  printf("In dummyfunc() -- PASS\n");
}
