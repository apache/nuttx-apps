/****************************************************************************
 * apps/examples/elf/tests/longjmp/longjmp.c
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
#include <setjmp.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAIN_VAL        47
#define FUNC_VAL        92
#define LEAF_VAL       163

#define FUNCTION_ARG   MAIN_VAL
#define LEAF_ARG      (FUNCTION_ARG + FUNC_VAL)
#define SETJMP_RETURN (LEAF_ARG + LEAF_VAL)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static jmp_buf env;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int leaf(int *some_arg)
{
  int some_local_variable = *some_arg + LEAF_VAL;

  printf("leaf: received %d\n", *some_arg);

  if (*some_arg != LEAF_ARG)
    printf("leaf: ERROR: expected %d\n", LEAF_ARG);

  printf("leaf: Calling longjmp() with %d\n", some_local_variable);

  longjmp(env, some_local_variable);

  /* We should not get here */

  return -ERROR;
}

static int function(int some_arg)
{
  int some_local_variable = some_arg + FUNC_VAL;
  int retval;

  printf("function: received %d\n", some_arg);

  if (some_arg != FUNCTION_ARG)
    printf("function: ERROR: expected %d\n", FUNCTION_ARG);

  printf("function: Calling leaf() with %d\n", some_local_variable);

  retval = leaf(&some_local_variable);

  printf("function: ERROR -- leaf returned!\n");
  return retval;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  int value;

  printf("main: Calling setjmp\n");
  value = setjmp(env);
  printf("main: setjmp returned %d\n", value);

  if (value == 0)
    {
      printf("main: Normal setjmp return\n");
      printf("main: Calling function with %d\n", MAIN_VAL);
      function(MAIN_VAL);
      printf("main: ERROR -- function returned!\n");
      return 1;
    }
  else if (value != SETJMP_RETURN)
    {
      printf("main: ERROR: Expected %d\n", SETJMP_RETURN);
      return 1;
    }
  else
    {
      printf("main: SUCCESS: setjmp return from longjmp call\n");
      return 0;
    }
}
