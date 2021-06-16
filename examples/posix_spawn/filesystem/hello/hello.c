/****************************************************************************
 * apps/examples/posix_spawn/filesystem/hello/hello.c
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  int i;

  /* Mandatory "Hello, world!" */

  puts("Getting ready to say \"Hello, world\"\n");
  printf("Hello, world!\n");
  puts("It has been said.\n");

  /* Print arguments */

  printf("argc\t= %d\n", argc);
  printf("argv\t= 0x%p\n", argv);

  for (i = 0; i < argc; i++)
    {
      printf("argv[%d]\t= ", i);
      if (argv[i])
        {
          printf("(0x%p) \"%s\"\n", argv[i], argv[i]);
        }
      else
        {
          printf("NULL?\n");
        }
    }

  printf("argv[%d]\t= 0x%p\n", argc, argv[argc]);
  printf("Goodbye, world!\n");
  return 0;
}
