/****************************************************************************
 * apps/testing/ostest/timedmutex.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef NULL
# define NULL (void*)0
#endif

#ifndef OK
# define OK 0
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_mutex;
static bool g_running;
static int g_result;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *thread_func(FAR void *parameter)
{
  struct timespec ts;
  int status;

  printf("pthread:  Started\n");
  g_running = true;

  for (; ; )
    {
      printf("pthread:  Waiting for lock or timeout\n");

      /* Get the current time */

      status = clock_gettime(CLOCK_REALTIME, &ts);
      if (status < 0)
        {
          int errcode = errno;
          fprintf(stderr, "pthread: clock_gettime() failed: %d\n", errcode);
          g_result = errcode;
          break;
        }

      /* Get a time two seconds in the future (we presume that we cannot be
       * blocked for two seconds here!)
       */

      ts.tv_sec += 2;

      /* Now wait until either we get the lock or until the timeout occurs */

      status = pthread_mutex_timedlock(&g_mutex, &ts);
      if (status != 0)
        {
          if (status == ETIMEDOUT)
            {
              printf("pthread:  Got the timeout.  Terminating\n");
            }
          else
            {
              fprintf(stderr, "pthread: pthread_mutex_timedlock() failed: %d\n",
                      status);
            }

          g_result = status;
          break;
        }

      printf("pthread:  Got the lock\n");

      /* Release the lock and wait a bit in case the main thread wants it. */

      pthread_mutex_unlock(&g_mutex);
      usleep(500*1000);
    }

  g_running = false;
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void timedmutex_test(void)
{
  pthread_t thread;
#ifdef SDCC
  pthread_addr_t result;
#endif
  int status;

  /* Initialize the mutex */

  printf("mutex_test: Initializing mutex\n");
  pthread_mutex_init(&g_mutex, NULL);

  /* Lock the mutex */

  status = pthread_mutex_lock(&g_mutex);
  if (status != OK)
    {
      fprintf(stderr, "mutex_test: Failed to get mutex: %d\n", status);
      goto errout_with_mutex;
    }

  /* Start a thread */

  printf("mutex_test: Starting thread\n");
#ifdef SDCC
  pthread_attr_init(&attr);
  status = pthread_create(&thread, &attr, thread_func, (pthread_addr_t)0);
#else
  status = pthread_create(&thread, NULL, thread_func, (pthread_addr_t)0);
#endif
  if (status != 0)
    {
      fprintf(stderr, "mutex_test: Error in thread creation: %d\n", status);
      goto errout_with_lock;
    }

  /* Wait a bit to assure that the thread gets a chance to start */

  usleep(500*1000);

  /* Then unlock the mutex.  This should wake up the pthread. */

  printf("mutex_test: Unlocking\n");
  pthread_mutex_unlock(&g_mutex);

  /* Wait a bit to assure that the thread gets a chance to lock the mutex at
   * least once (it may probably loop and retake the mutex several times)
   */

  usleep(500*1000);

  /* The re-lock the mutex.  The pthread should now be waiting for the lock
   * or a timeout.
   */

  status = pthread_mutex_lock(&g_mutex);
  if (status != OK)
    {
      fprintf(stderr, "mutex_test: Failed to get mutex: %d\n", status);
      goto errout_with_mutex;
    }

  /* The pthread should timeout in two seconds.  Let's wait four.  At that
   * time, the pthread should no longer be running.
   */

  sleep(4);

  /* The pthread should no longer be running and it should have terminated
   * because of EAGAIN.
   */

  if (g_running)
    {
      fprintf(stderr, "mutex_test: ERROR: The pthread is still running!\n");
    }
  else if (g_result != ETIMEDOUT)
    {
      fprintf(stderr, "mutex_test: ERROR: Result was not ETIMEDOUT: %d\n",
              g_result);
    }
  else
    {
      printf("mutex_test: PASSED\n");
    }

  /* Let's reap any join droppings left from the pthread */

#ifdef SDCC
  pthread_join(thread, &result);
#else
  pthread_join(thread, NULL);
#endif

errout_with_lock:
  pthread_mutex_unlock(&g_mutex);
errout_with_mutex:
  pthread_mutex_destroy(&g_mutex);
}
