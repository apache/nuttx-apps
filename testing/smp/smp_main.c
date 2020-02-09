/****************************************************************************
 * apps/testing/smp/smp_main.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HOG_MSEC       1000
#define YIELD_MSEC     100
#define IMPOSSIBLE_CPU -1

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_barrier_t g_smp_barrier;
static volatile int g_thread_cpu[CONFIG_TESTING_SMP_NBARRIER_THREADS+1];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_cpu / show_cpu_conditional
 *
 * Description:
 *   This depends on internal OS interfaces that are not generally available
 *   but can be accessed (albeit inappropriatley) in a FLAT build
 *
 ****************************************************************************/

static void show_cpu(FAR const char *caller, int threadno)
{
  g_thread_cpu[threadno] = sched_getcpu();
  printf("%s[%d]: Running on CPU%d\n", caller, threadno, g_thread_cpu[threadno]);
}

static void show_cpu_conditional(FAR const char *caller, int threadno)
{
  int cpu = sched_getcpu();

  if (cpu != g_thread_cpu[threadno])
    {
      g_thread_cpu[threadno] = cpu;
      printf("%s[%d]: Now running on CPU%d\n", caller, threadno, cpu);
    }
}

/****************************************************************************
 * Name: hog_milliseconds
 *
 * Description:
 *   Delay inline for the specified number of milliseconds.
 *
 ****************************************************************************/

static void hog_milliseconds(unsigned int milliseconds)
{
  volatile unsigned int i;
  volatile unsigned int j;

  for (i = 0; i < milliseconds; i++)
    {
      for (j = 0; j < CONFIG_BOARD_LOOPSPERMSEC; j++)
        {
        }
    }
}

/****************************************************************************
 * Name: hog_time
 *
 * Description:
 *   Delay for awhile, calling pthread_yield() now and then to allow other
 *   pthreads to get CPU time.
 *
 ****************************************************************************/

static void hog_time(FAR const char *caller, int threadno)
{
  unsigned int remaining = HOG_MSEC;
  unsigned int hogmsec;

  while (remaining > 0)
    {
      /* Hog some CPU */

      hogmsec = YIELD_MSEC;
      if (hogmsec > remaining)
        {
          hogmsec = remaining;
        }

      hog_milliseconds(hogmsec);
      remaining -= hogmsec;

      /* Let other threads run */

      pthread_yield();
      show_cpu_conditional(caller, threadno);
    }
}

/****************************************************************************
 * Name: barrier_thread
 ****************************************************************************/

static pthread_addr_t barrier_thread(pthread_addr_t parameter)
{
  int threadno  = (int)((intptr_t)parameter);
  int ret;

  printf("Thread[%d]: Started\n",  threadno);
  show_cpu("Thread", threadno);

  /* Hog some CPU time */

  hog_time("Thread", threadno);

  /* Wait at the barrier until all threads are synchronized. */

  printf("Thread[%d]: Calling pthread_barrier_wait()\n",
         threadno);
  fflush(stdout);
  show_cpu_conditional("Thread", threadno);

  ret = pthread_barrier_wait(&g_smp_barrier);
  if (ret == 0)
    {
      printf("Thread[%d]: Back with ret=0 (I am not special)\n",
             threadno);
    }
  else if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
    {
      printf("Thread[%d]: Back with "
             "ret=PTHREAD_BARRIER_SERIAL_THREAD (I AM SPECIAL)\n",
             threadno);
    }
  else
    {
      printf("Thread[%d]: ERROR could not get semaphore value\n",
             threadno);
    }

  fflush(stdout);
  show_cpu_conditional("Thread", threadno);

  /* Hog some more CPU time */

  hog_time("Thread", threadno);

  /* Then exit */

  printf("Thread[%d]: Done\n",  threadno);
  fflush(stdout);
  show_cpu_conditional("Thread", threadno);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * smp_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pthread_t threadid[CONFIG_TESTING_SMP_NBARRIER_THREADS];
  pthread_addr_t result;
  pthread_attr_t attr;
  pthread_barrierattr_t barrierattr;
  int errcode = EXIT_SUCCESS;
  int ret;
  int i;

  /* Initialize data */

  memset(threadid, 0, sizeof(pthread_t) * CONFIG_TESTING_SMP_NBARRIER_THREADS);
  for (i = 0; i <= CONFIG_TESTING_SMP_NBARRIER_THREADS; i++)
    {
      g_thread_cpu[i] = IMPOSSIBLE_CPU;
      if (i < CONFIG_TESTING_SMP_NBARRIER_THREADS)
        {
          threadid[i] = 0;
        }
    }

  show_cpu("  Main", 0);
  printf("  Main[0]: Initializing barrier\n");

  ret = pthread_barrierattr_init(&barrierattr);
  if (ret != OK)
    {
      printf("  Main[0]: pthread_barrierattr_init failed, ret=%d\n", ret);

      errcode = EXIT_FAILURE;
      goto errout;
    }

  ret = pthread_barrier_init(&g_smp_barrier, &barrierattr,
                             CONFIG_TESTING_SMP_NBARRIER_THREADS);
  if (ret != OK)
    {
      printf("  Main[0]: pthread_barrierattr_init failed, ret=%d\n", ret);

      errcode = EXIT_FAILURE;
      goto errout_with_attr;
    }

  /* Create the barrier */

  pthread_barrierattr_init(&barrierattr);

  /* Start CONFIG_TESTING_SMP_NBARRIER_THREADS thread instances */

  ret = pthread_attr_init(&attr);
  if (ret != OK)
    {
      printf("  Main[0]: pthread_attr_init failed, ret=%d\n", ret);

      errcode = EXIT_FAILURE;
      goto errout_with_barrier;
    }

  for (i = 0; i < CONFIG_TESTING_SMP_NBARRIER_THREADS; i++)
    {
      ret = pthread_create(&threadid[i], &attr, barrier_thread,
                           (pthread_addr_t)((uintptr_t)i+1));
      if (ret != 0)
        {
          printf("  Main[0]: Error in thread %d create, ret=%d\n",  i+1, ret);
          printf("  Main[0]: Test aborted with waiting threads\n");

          errcode = EXIT_FAILURE;
          break;
        }
      else
        {
          printf("  Main[0]: Thread %d created\n", i+1);
        }

      show_cpu_conditional("  Main", 0);
    }

  fflush(stdout);
  show_cpu_conditional("  Main", 0);

  /* Wait for all thread instances to complete */

  for (i = 0; i < CONFIG_TESTING_SMP_NBARRIER_THREADS; i++)
    {
      if (threadid[i] != 0)
        {
          ret = pthread_join(threadid[i], &result);
          show_cpu_conditional("  Main", 0);

          if (ret != 0)
            {
              printf("  Main[0]: Error in thread %d join, ret=%d\n", i+1, ret);
              errcode = EXIT_FAILURE;
            }
          else
            {
              printf("  Main[0]: Thread %d completed with result=%p\n", i+1, result);
            }
        }
    }

  /* Destroy the barrier */

errout_with_barrier:
  ret = pthread_barrier_destroy(&g_smp_barrier);
  if (ret != OK)
    {
      printf("  Main[0]: pthread_barrier_destroy failed, ret=%d\n", ret);
    }

errout_with_attr:
  ret = pthread_barrierattr_destroy(&barrierattr);
  if (ret != OK)
    {
      printf("  Main[0]: pthread_barrierattr_destroy failed, ret=%d\n",
             ret);
    }

errout:
  fflush(stdout);
  show_cpu_conditional("  Main", 0);
  return errcode;
}
