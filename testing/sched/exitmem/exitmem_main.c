/****************************************************************************
 * apps/testing/sched/exitmem/exitmem_main.c
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

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EXITMEM_ROUNDS   8    /* Default number of children */
#define EXITMEM_PERCENT  40   /* Default part of the free memory */
#define EXITMEM_WAIT     30   /* Free memory checks, 100 ms apart */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: free_kib
 *
 * Description:
 *   Return the free memory in KiB, or -1.
 *
 ****************************************************************************/

static long free_kib(void)
{
  long npages = sysconf(_SC_AVPHYS_PAGES);
  long pgsize = sysconf(_SC_PAGESIZE);

  if (npages < 0 || pgsize < 1024)
    {
      return -1;
    }

  return npages * (pgsize / 1024);
}

/****************************************************************************
 * Name: pin
 *
 * Description:
 *   Run the calling thread on one CPU only.
 *
 ****************************************************************************/

static void pin(int cpu)
{
#ifdef CONFIG_SMP
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0)
    {
      printf("exitmem: sched_setaffinity(%d) failed: %d\n", cpu, errno);
    }

  sched_yield();
#endif
}

/****************************************************************************
 * Name: side_thread
 *
 * Description:
 *   Run on another CPU and end: that CPU then idles after the child.
 *
 ****************************************************************************/

static FAR void *side_thread(FAR void *arg)
{
  pin((int)(intptr_t)arg);
  return NULL;
}

/****************************************************************************
 * Name: child
 *
 * Description:
 *   Run on cpu (and a thread on third), take size KiB, write every page
 *   and end.
 *
 ****************************************************************************/

static void child(int cpu, int third, long size)
{
  long pgsize = sysconf(_SC_PAGESIZE);
  pthread_t thread;
  FAR char *buf;
  long off;

  pin(cpu);
  if (pthread_create(&thread, NULL, side_thread,
                     (FAR void *)(intptr_t)third) == 0)
    {
      pthread_join(thread, NULL);
    }

  /* A heap of fixed size can be smaller than the free memory */

  while ((buf = malloc(size * 1024)) == NULL && size > 64)
    {
      size /= 2;
    }

  if (buf == NULL)
    {
      _exit(2);
    }

  for (off = 0; off < size * 1024; off += pgsize)
    {
      buf[off] = (char)off;
    }

  _exit(0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * exitmem_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int rounds = argc > 1 ? atoi(argv[1]) : EXITMEM_ROUNDS;
  int percent = argc > 2 ? atoi(argv[2]) : EXITMEM_PERCENT;
  int ncpus = 1;
  long start;
  int fail = 0;
  int round;

#ifdef CONFIG_SMP
  ncpus = CONFIG_SMP_NCPUS;
#endif

  start = free_kib();
  if (start < 0 || rounds < 1 || percent < 1 || percent > 90)
    {
      printf("Usage: %s [rounds [percent]]\n", argv[0]);
      return EXIT_FAILURE;
    }

  printf("exitmem: %d CPUs, %d rounds, %d%% of the free memory\n",
         ncpus, rounds, percent);

  pin(0);
  for (round = 1; round <= rounds; round++)
    {
      int cpu = ncpus > 1 ? 1 + (round - 1) % (ncpus - 1) : 0;
      int third = ncpus > 2 ? 1 + cpu % (ncpus - 1) : cpu;
      long before = free_kib();
      long size = before / 100 * percent;
      long after = -1;
      int status = -1;
      pid_t pid;
      int i;

      fflush(stdout);
      pid = fork();
      if (pid == 0)
        {
          child(cpu, third, size);
        }

      if (pid < 0 || waitpid(pid, &status, 0) != pid)
        {
          printf("exitmem: round %d: fork or waitpid failed: %d\n",
                 round, errno);
          fail++;
          continue;
        }

      /* A work queue frees the address environment: give it some time */

      for (i = 0; i < EXITMEM_WAIT; i++)
        {
          after = free_kib();
          if (after >= before - before / 16)
            {
              break;
            }

          usleep(100 * 1000);
        }

      if (status != 0 || after < before - before / 16)
        {
          printf("exitmem: round %d CPU %d: %ld KiB, status %d, "
                 "free %ld KiB before, %ld KiB after: FAIL\n",
                 round, cpu, size, status, before, after);
          fail++;
        }
      else
        {
          printf("exitmem: round %d CPU %d: %ld KiB, "
                 "free %ld KiB before, %ld KiB after\n",
                 round, cpu, size, before, after);
        }
    }

  if (free_kib() < start - start / 16)
    {
      printf("exitmem: free %ld KiB at the start, %ld KiB at the end: "
             "FAIL\n", start, free_kib());
      fail++;
    }

  printf("exitmem: %s\n", fail > 0 ? "FAIL" : "PASS");
  return fail > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
