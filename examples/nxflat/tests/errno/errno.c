/****************************************************************************
 * apps/examples/nxflat/tests/errno/errno.c
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
#include <errno.h>

/****************************************************************************
 * Included Files
 ****************************************************************************/

static const char g_nonexistent[] = "aflav-sautga-ay";

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  FILE *test_stream;

  /* Try using stdout and stderr explicitly.  These are global variables
   * exported from the base code.
   */

  fprintf(stdout, "Hello, World on stdout\n");
  fprintf(stderr, "Hello, World on stderr\n");

  /* Try opening a non-existent file using buffered IO. */

  test_stream = fopen(g_nonexistent, "r");
  if (test_stream)
    {
      fprintf(stderr, "Hmm... Delete \"%s\" and try this again\n",
              g_nonexistent);
      exit(1);
    }

  /* Now print the errno on stderr.  Errno is also a global
   * variable exported by the base code.
   */

  fprintf(stderr, "We failed to open \"%s!\" errno is %d\n",
          g_nonexistent, errno);

  return 0;
}
