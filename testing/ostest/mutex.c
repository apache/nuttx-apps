/****************************************************************************
 * apps/testing/ostest/mutex.c
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
#include <pthread.h>
#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NLOOPS 32

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t mut;
static volatile int my_mutex = 0;
static unsigned long nloops[2] =
  {
    0,
    0
  };
static unsigned long nerrors[2] =
  {
    0,
    0
  };

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *thread_func(FAR void *parameter)
{
  int id  = (int)((intptr_t)parameter);
  int ndx = id - 1;
  int i;

  for (nloops[ndx] = 0; nloops[ndx] < NLOOPS; nloops[ndx]++)
    {
      int status = pthread_mutex_lock(&mut);
      if (status != 0)
        {
          printf("ERROR thread %d: pthread_mutex_lock failed, status=%d\n",
                  id, status);
        }

      if (my_mutex == 1)
        {
          printf("ERROR thread=%d: "
                 "my_mutex should be zero, instead my_mutex=%d\n",
                  id, my_mutex);
          nerrors[ndx]++;
        }

      my_mutex = 1;
      for (i = 0; i < 10; i++)
        {
          pthread_yield();
        }

      my_mutex = 0;

      status = pthread_mutex_unlock(&mut);
      if (status != 0)
        {
          printf("ERROR thread %d: pthread_mutex_unlock failed, status=%d\n",
                 id, status);
        }
    }

  pthread_exit(NULL);
  return NULL; /* Non-reachable -- needed for some compilers */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void mutex_test(void)
{
  pthread_t thread1;
  pthread_t thread2;
#ifdef SDCC
  pthread_addr_t result1;
  pthread_addr_t result2;
  pthread_attr_t attr;
#endif
  int status;

  /* Initialize the mutex */

  printf("Initializing mutex\n");
  pthread_mutex_init(&mut, NULL);

  /* Start two thread instances */

  printf("Starting thread 1\n");
#ifdef SDCC
  pthread_attr_init(&attr);
  status = pthread_create(&thread1, &attr, thread_func, (pthread_addr_t)1);
#else
  status = pthread_create(&thread1, NULL, thread_func, (pthread_addr_t)1);
#endif
  if (status != 0)
    {
      printf("ERROR in thread#1 creation\n");
    }

  printf("Starting thread 2\n");
#ifdef SDCC
  status = pthread_create(&thread2, &attr, thread_func, (pthread_addr_t)2);
#else
  status = pthread_create(&thread2, NULL, thread_func, (pthread_addr_t)2);
#endif
  if (status != 0)
    {
      printf("ERROR in thread#2 creation\n");
    }

#ifdef SDCC
  pthread_join(thread1, &result1);
  pthread_join(thread2, &result2);
#else
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
#endif

  printf("\t\tThread1\tThread2\n");
  printf("\tLoops\t%lu\t%lu\n", nloops[0], nloops[1]);
  printf("\tErrors\t%lu\t%lu\n", nerrors[0], nerrors[1]);
}
