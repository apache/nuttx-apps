/****************************************************************************
 * apps/system/gcov/gcov.c
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

#include <fcntl.h>
#include <gcov.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/crc16.h>
#include <nuttx/streams.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("\nUsage: %s [-d path] [-t strip] [-r] [-h]\n", progname);
  printf("\nWhere:\n");
  printf("  -d dump the coverage, path is the path to the coverage file, "
         "the default output is to stdout\n");
  printf("  -t strip the path prefix number\n");
  printf("  -r reset the coverage\n");
  printf("  -h show this text and exits.\n");
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *strip = CONFIG_COVERAGE_DEFAULT_PREFIX_STRIP;
  FAR const char *path = NULL;
  int option;

  while ((option = getopt(argc, argv, "d::t:orh")) != ERROR)
    {
      switch (option)
        {
        case 'd':
          path = optarg;
          break;

        case 't':
          strip = optarg;
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

  setenv("GCOV_PREFIX_STRIP", strip, 1);
  setenv("GCOV_PREFIX", path, 1);
  __gcov_dump();
  return 0;
}
