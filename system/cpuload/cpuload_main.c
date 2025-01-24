/****************************************************************************
 * apps/system/cpuload/cpuload_main.c
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

#include <nuttx/config.h>
#include <nuttx/arch.h>

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_SCHED_CPULOAD_EXTCLK
#  define CPULOAD_US        (USEC_PER_SEC / CONFIG_SCHED_CPULOAD_TICKSPERSEC)
#else
#  define CPULOAD_US        (USEC_PER_SEC / CLOCKS_PER_SEC)
#endif

#define CPULOAD_DELAY       (100 * CPULOAD_US)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  optind = 0;

  printf("\nUsage: %s [-c cpu] -p percent\n", progname);
  printf("\nWhere:\n");
  printf("  -c bind to specific CPU, don't bind CPU if no this option\n");
  printf("  -p process percent[1-100], exectime / (exectime + idletime)\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cpuload_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *endptr;
  int option;
  int cpu = -1;
  int per = 50;

  while ((option = getopt(argc, argv, "c:p:")) != ERROR)
    {
      if (option == 'c')
        {
          cpu = strtol(optarg, &endptr, 10);
        }
      else if (option == 'p')
        {
          per = strtol(optarg, &endptr, 10);
        }
      else
        {
          printf("Unrecognized option: '%c'\n", option);
          show_usage(argv[0], EXIT_FAILURE);
        }
    }

  /* There should be two parameters remaining on the command line */

  if (per < 1 || per > 100 || optind > argc)
    {
      printf("Missing required arguments\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

#ifdef CONFIG_SMP
  if (cpu > 0 && cpu < CONFIG_SMP_NCPUS)
    {
      cpu_set_t cpu_mask;

      CPU_ZERO(&cpu_mask);
      CPU_SET(cpu, &cpu_mask);

      pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_mask);
    }
#endif

  while (1)
    {
      up_udelay(per * CPULOAD_DELAY / 100);
      usleep((100 - per) * CPULOAD_DELAY / 100);
    }

  return 0;
}
