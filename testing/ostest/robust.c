/****************************************************************************
 * apps/testing/ostest/robust.c
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

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include "ostest.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_robust_mutex;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *robust_waiter(FAR void *parameter)
{
  int status;

  /* Take the mutex */

  printf("robust_waiter: Taking mutex\n");
  status = pthread_mutex_lock(&g_robust_mutex);
  if (status != 0)
    {
       printf("thread_waiter: ERROR: pthread_mutex_lock failed, status=%d\n",
               status);
    }

  if (status != 0)
    {
       printf("robust_waiter: ERROR: pthread_mutex_lock failed, status=%d\n",
               status);
    }
  else
    {
      printf("robust_waiter: Exiting with mutex\n");
    }

  sleep(2);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void robust_test(void)
{
  pthread_attr_t pattr;
  pthread_mutexattr_t mattr;
  pthread_t waiter;
  void *result;
  int nerrors = 0;
  int status;

  /* Initialize the mutex */

  printf("robust_test: Initializing mutex\n");

  status = pthread_mutexattr_init(&mattr);
  if (status != 0)
    {
      printf("robust_test: ERROR: "
             "pthread_mutexattr_init failed, status=%d\n",
             status);
      nerrors++;
    }

  status = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
  if (status != 0)
    {
      printf("robust_test: ERROR: "
             "pthread_mutexattr_setrobust failed, status=%d\n",
             status);
      nerrors++;
    }

  status = pthread_mutex_init(&g_robust_mutex, &mattr);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_mutex_init failed, status=%d\n",
             status);
      nerrors++;
    }

  /* Set up pthread attributes */

  printf("robust_test: Starting thread\n");

  status = pthread_attr_init(&pattr);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_attr_init failed, status=%d\n",
             status);
      nerrors++;
    }

  status = pthread_attr_setstacksize(&pattr, STACKSIZE);
  if (status != 0)
    {
      printf("robust_test: ERROR: "
             "pthread_attr_setstacksize failed, status=%d\n",
             status);
      nerrors++;
    }

  /* Start the robust waiter thread.  It will take the mutex, sleep for two
   * seconds, and exit holding the mutex.
   */

  status = pthread_create(&waiter, &pattr, robust_waiter, NULL);
  if (status != 0)
    {
      printf("robust_test: ERROR: "
             "pthread_create failed, status=%d\n", status);
      printf("             ERROR: Terminating test\n");
      nerrors++;
      return;
    }

  /* Wait one second.. the robust waiter should still be waiting */

  sleep(1);

  /* Now try to take the mutex held by the robust waiter.  This should wait
   * one second there fail with EOWNERDEAD.
   */

  status = pthread_mutex_lock(&g_robust_mutex);
  if (status == 0)
    {
      printf("robust_test: ERROR: pthread_mutex_lock succeeded\n");
      nerrors++;
    }
  else if (status != EOWNERDEAD)
    {
      printf("robust_test: ERROR: pthread_mutex_lock failed with %d\n",
              status);
      printf("             ERROR: expected %d (EOWNERDEAD)\n", EOWNERDEAD);
      nerrors++;
    }

  /* Try again,
   * this should return immediately, still failing with EOWNERDEAD
   */

  printf("robust_test: Take the lock again\n");
  status = pthread_mutex_lock(&g_robust_mutex);
  if (status == 0)
    {
      printf("robust_test: ERROR: pthread_mutex_lock succeeded\n");
      nerrors++;
    }
  else if (status != EOWNERDEAD)
    {
      printf("robust_test: ERROR: pthread_mutex_lock failed with %d\n",
              status);
      printf("             ERROR: expected %d (EOWNERDEAD)\n", EOWNERDEAD);
      nerrors++;
    }

  /* Make the mutex consistent and try again.  It should succeed this time. */

  printf("robust_test: Make the mutex consistent again.\n");
  status = pthread_mutex_consistent(&g_robust_mutex);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_mutex_consistent failed: %d\n",
              status);
      nerrors++;
    }

  printf("robust_test: Take the lock again\n");
  status = pthread_mutex_lock(&g_robust_mutex);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_mutex_lock failed with: %d\n",
              status);
      nerrors++;
    }

  /* Then join to the thread to pick up the result (if we don't do this we
   * will have a memory leak!)
   */

  printf("robust_test: Joining\n");
  status = pthread_join(waiter, &result);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_join failed, status=%d\n", status);
      nerrors++;
    }
  else
    {
      printf("robust_test: waiter exited with result=%p\n", result);
      if (result != NULL)
        {
          printf("robust_test: ERROR: expected result=%p\n",
                  PTHREAD_CANCELED);
          nerrors++;
        }
    }

  /* Release and destroy the mutex then  return success */

  status = pthread_mutex_unlock(&g_robust_mutex);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_mutex_unlock failed, status=%d\n",
              status);
      nerrors++;
    }

  status = pthread_mutex_destroy(&g_robust_mutex);
  if (status != 0)
    {
      printf("robust_test: ERROR: pthread_mutex_unlock failed, status=%d\n",
              status);
      nerrors++;
    }

  printf("robust_test: Test complete with nerrors=%d\n", nerrors);
}
