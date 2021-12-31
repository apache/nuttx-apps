/****************************************************************************
 * apps/system/hostname/hostname_main.c
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

#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  char hostname[HOST_NAME_MAX + 1];

  if (argc > 1)
    {
      if (argc > 2)
        {
          fprintf(stderr, "ERROR: Too many arguments\n");
          return EXIT_FAILURE;
        }

      ret = sethostname(argv[1], strlen(argv[1]));
      if (ret != 0)
        {
          printf("sethostname() returned %d\n", ret);
          return EXIT_FAILURE;
        }
    }
  else
    {
      ret = gethostname(hostname, HOST_NAME_MAX);
      if (ret != 0)
        {
          printf("gethostname() returned %d\n", ret);
          return EXIT_FAILURE;
        }

      printf("%s\n", hostname);
    }

  return EXIT_SUCCESS;
}
