/****************************************************************************
 * apps/system/taskset/taskset.c
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

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("%s mask command ...\n", progname);
  printf("%s -p [mask] pid\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static bool get_cpuset(const char *arg, cpu_set_t *cpu_set)
{
  DEBUGASSERT(NULL != arg);

  bool ret = false;
  int val  = atoi(arg);

  if (0 < val && val < (1 << CONFIG_SMP_NCPUS))
    {
      *cpu_set = (cpu_set_t)val;
      ret = true;
    }
  else
    {
      fprintf(stderr, "invalid cpuset %s\n", arg);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char command[CONFIG_NSH_LINELEN];
  int exitcode;
  int option;
  int pid = -1;
  cpu_set_t cpuset;
  int rc;
  int i;

  memset(command, 0, sizeof(command));

  CPU_ZERO(&cpuset);

  /* Parse command line options */

  exitcode = EXIT_FAILURE;

  while ((option = getopt(argc, argv, ":p:h")) != ERROR)
    {
      switch (option)
        {
          case 'p':
            {
              pid = (int)atoi(argv[argc -1]);
            }
            break;

          case 'h':
            exitcode = EXIT_SUCCESS;
            goto errout_with_usage;
        }
    }

  if (-1 != pid)
    {
      if (4 == argc)
        {
          if (!get_cpuset(argv[2], &cpuset))
            {
              goto errout;
            }

          rc = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);

          if (-1 == rc)
            {
              fprintf(stderr,
                      "Err in sched_setaffinity() errno=%d\n", errno);
              goto errout;
            }
        }

      rc = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);

      if (-1 == rc)
        {
          fprintf(stderr, "Err in sched_getaffinity() errno=%d\n", errno);
          goto errout;
        }

      printf("pid %d's current affinity mask: %x\n", pid, cpuset);
    }
  else
    {
      if (3 <= argc)
        {
          if (!get_cpuset(argv[1], &cpuset))
            {
              goto errout;
            }

          /* Construct actual command with args
           * NOTE: total length does not exceed CONFIG_NSH_LINELEN
           */

          for (i = 0; i < argc - 2; i++)
            {
              strcat(command, argv[i + 2]);
              strcat(command, " ");
            }

          sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset);
          system(command);
        }
    }

errout:
  return EXIT_SUCCESS;
errout_with_usage:
  show_usage(argv[0], exitcode);
  return exitcode;  /* Not reachable */
}
