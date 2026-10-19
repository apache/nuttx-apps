/****************************************************************************
 * apps/examples/nxflat/tests/tls/tls.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tls_s
{
  const char *name;
  int *ptr;
  int n;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

int g_global = 117;

/* No initial value: no template.  An initial value: a template.  A struct
 * with pointers: a template that the loader must relocate.
 */

__thread int g_zero;
__thread int g_init = 42;
__thread struct tls_s g_struct =
{
  "tls-string", &g_global, 3
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_errors;
static int *g_childaddr;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void check(const char *who, const char *what, int ok)
{
  printf("%s: %-34s %s\n", who, what, ok ? "PASS" : "FAIL");
  if (!ok)
    {
      g_errors++;
    }
}

static void check_initial(const char *who)
{
  printf("%s: g_zero=%d g_init=%d name=%s *ptr=%d n=%d\n", who, g_zero,
         g_init, g_struct.name, *g_struct.ptr, g_struct.n);

  check(who, "zero-initialized variable", g_zero == 0);
  check(who, "variable with a template", g_init == 42);
  check(who, "string pointer in a template",
        strcmp(g_struct.name, "tls-string") == 0);
  check(who, "data pointer in a template", g_struct.ptr == &g_global);
  check(who, "value through that pointer", *g_struct.ptr == 117);
  check(who, "integer in a struct template", g_struct.n == 3);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* A function that is used through a pointer must have global scope:  see
 * the pthread test.
 */

void *child_start_routine(void *arg)
{
  check_initial("CHILD");

  g_childaddr = &g_zero;
  g_zero      = 2;
  g_init      = 102;
  g_struct.n  = 302;

  check("CHILD", "own values read back",
        g_zero == 2 && g_init == 102 && g_struct.n == 302);
  return NULL;
}

int main(int argc, char **argv)
{
  pthread_t thread;
  int status;

  check_initial("MAIN");

  g_zero     = 1;
  g_init     = 101;
  g_struct.n = 301;

  status = pthread_create(&thread, NULL, child_start_routine, NULL);
  if (status != 0)
    {
      printf("MAIN: pthread_create() returned %d\n", status);
      return EXIT_FAILURE;
    }

  pthread_join(thread, NULL);

  check("MAIN", "values kept after the child ran",
        g_zero == 1 && g_init == 101 && g_struct.n == 301);
  check("MAIN", "child has its own copy", g_childaddr != &g_zero);

  if (g_errors != 0)
    {
      printf("TLS test FAILED: %d error(s)\n", g_errors);
      return EXIT_FAILURE;
    }

  printf("TLS test PASSED\n");
  return EXIT_SUCCESS;
}
