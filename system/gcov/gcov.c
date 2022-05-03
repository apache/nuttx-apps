/****************************************************************************
 * apps/system/gcov/gcov.c
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
#include <unistd.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("\nUsage: %s [-d] [-r] [-h]\n", progname);
  printf("\nWhere:\n");
  printf("  -d dump the coverage.\n");
  printf("  -r reset the coverage\n");
  printf("  -h show this text and exits.\n");
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void __gcov_dump(void);
void __gcov_reset(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int option;

  while ((option = getopt(argc, argv, "drh")) != ERROR)
    {
      switch (option)
        {
          case 'd':
            __gcov_dump();
            break;

          case 'r':
            __gcov_reset();
            break;

          case '?':
          default:
            fprintf(stderr, "ERROR: Unrecognized option\n");

          case 'h':
            show_usage(argv[0]);
        }
    }

  return 0;
}
