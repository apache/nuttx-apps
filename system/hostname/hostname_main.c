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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("Usage: %s [<hostname>|-F <file>]\n", progname);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  bool set = false;
  FAR FILE *f;
  char hostname[HOST_NAME_MAX + 2];

  while ((ret = getopt(argc, argv, ":F:h")) != ERROR)
    {
      switch (ret)
        {
          case 'F':
            set = true;
            f = fopen(optarg, "r");
            if (f == NULL)
              {
                fprintf(stderr, "ERROR: Failed to open '%s': %s\n", optarg,
                        strerror(errno));
                return EXIT_FAILURE;
              }

            if (fgets(hostname, sizeof(hostname), f) == NULL)
              {
                if (errno != 0)
                  {
                    fprintf(stderr, "ERROR: Failed to read '%s': %s\n",
                            optarg, strerror(errno));
                    fclose(f);
                    return EXIT_FAILURE;
                  }

                hostname[0] = '\0';
              }
            else
              {
                *strchrnul(hostname, '\n') = '\0';
              }

            fclose(f);
            break;

          case 'h':
            show_usage(argv[0]);
            return EXIT_SUCCESS;

          case ':':
            fprintf(stderr, "ERROR: Option needs a value: '%c'\n", optopt);
            show_usage(argv[0]);
            return EXIT_FAILURE;

          default:
          case '?':
            fprintf(stderr, "ERROR: Unrecognized option: '%c'\n", optopt);
            show_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  if (optind < argc && !set)
    {
      set = true;
      strlcpy(hostname, argv[optind++], sizeof(hostname));
    }

  if (optind < argc)
    {
      fprintf(stderr, "ERROR: Too many arguments\n");
      return -1;
    }

  if (set)
    {
      if (strlen(hostname) == 0 || strlen(hostname) > HOST_NAME_MAX)
        {
          fprintf(stderr, "ERROR: The specified hostname is invalid\n");
          return EXIT_FAILURE;
        }

      ret = sethostname(hostname, strlen(hostname));
      if (ret != 0)
        {
          fprintf(stderr, "sethostname() returned %d\n", ret);
          return EXIT_FAILURE;
        }
    }
  else
    {
      ret = gethostname(hostname, HOST_NAME_MAX);
      if (ret != 0)
        {
          fprintf(stderr, "gethostname() returned %d\n", ret);
          return EXIT_FAILURE;
        }

      printf("%s\n", hostname);
    }

  return EXIT_SUCCESS;
}
