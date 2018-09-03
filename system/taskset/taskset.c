/****************************************************************************
 * apps/system/taskset/taskset.c
 *
 *   Copyright 2018 Sony Video & Sound Products Inc.
 *   Author: Masayuki Ishikawa <Masayuki.Ishikawa@jp.sony.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include "nshlib/nshlib.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
  printf("%s mask command ... \n", progname);
  printf("%s -p [mask] pid \n", progname);
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
      fprintf(stderr, "invalid cpuset %s \n", arg);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int taskset_main(int argc, char **argv)
#endif
{
  FAR char *nshargv[2];
  int exitcode;
  int option;
  int pid = -1;
  cpu_set_t cpuset;
  int rc;

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
              fprintf(stderr, "Err in sched_setaffinity() errno=%d \n", errno);
              goto errout;
            }
        }

      rc = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);

      if (-1 == rc)
        {
          fprintf(stderr, "Err in sched_getaffinity() errno=%d \n", errno);
          goto errout;
        }

      printf("pid %d's current affinity mask: %x \n", pid, cpuset);
    }
  else
    {
      if (3 <= argc)
        {
          if (!get_cpuset(argv[1], &cpuset))
            {
              goto errout;
            }

          nshargv[0] = argv[2];
          nshargv[1] = NULL;

          sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset);
          usleep(10 * 1000);

          pid = task_create("system", CONFIG_SYSTEM_TASKSET_PRIORITY,
                            CONFIG_SYSTEM_TASKSET_STACKSIZE, nsh_system,
                            (FAR char * const *)nshargv);

          (void)waitpid(pid, &rc, 0);
        }
    }

errout:
  return EXIT_SUCCESS;
errout_with_usage:
  show_usage(argv[0], exitcode);
  return exitcode;  /* Not reachable */
}
