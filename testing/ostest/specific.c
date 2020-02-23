/****************************************************************************
 * apps/testing/ostest/specific.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <pthread.h>
#include "ostest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef NULL
# define NULL (void*)0
#endif

#define NLOOPS 32

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_key_t g_pthread_key;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *thread_func(FAR void *parameter)
{
  FAR void *data;
  int ret;

  /* Set the pthread specific data for the main thread */

  ret = pthread_setspecific(g_pthread_key, (FAR const void *)0xcafebabe);
  if (ret != 0)
    {
      printf("ERROR: pthread_setspecific() failed: %d\n", ret);
    }

  /* Check the thread-specific data */

  data = pthread_getspecific(g_pthread_key);
  if (data != (FAR void *)0xcafebabe)
    {
      printf("ERROR: pthread_getspecific() failed: Returned %p vs %p\n",
             data, (FAR void *)0xcafebabe);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void specific_test(void)
{
  FAR void *data;
  pthread_t thread;
#ifdef SDCC
  pthread_addr_t result;
  pthread_attr_t attr;
#endif
  int ret;

  /* Create the pthread key */

  ret = pthread_key_create(&g_pthread_key, NULL);
  if (ret != 0)
    {
      printf("ERROR: pthread_key_create() failed: %d\n", ret);
    }

  /* Set the pthread specific data for the main thread */

  ret = pthread_setspecific(g_pthread_key, (FAR const void *)0xf00dface);
  if (ret != 0)
    {
      printf("ERROR: pthread_setspecific() failed: %d\n", ret);
    }

  /* Check the thread-specific data */

  data = pthread_getspecific(g_pthread_key);
  if (data != (FAR void *)0xf00dface)
    {
      printf("ERROR: pthread_getspecific() failed: Returned %p vs %p\n",
             data, (FAR void *)0xf00dface);
    }

  /* Start a thread */

  printf("Starting thread\n");
#ifdef SDCC
  pthread_attr_init(&attr);
  ret = pthread_create(&thread, &attr, thread_func, (pthread_addr_t)0);
#else
  ret = pthread_create(&thread, NULL, thread_func, (pthread_addr_t)0);
#endif
  if (ret != 0)
    {
      printf("ERROR: pthread_create() failed: %d\n", ret);
    }

  /* Wait for the thread to terminate */

#ifdef SDCC
  pthread_join(thread, result);
#else
  pthread_join(thread, NULL);
#endif

  /* Re-check the thread-specific data */

  data = pthread_getspecific(g_pthread_key);
  if (data != (FAR void *)0xf00dface)
    {
      printf("ERROR: pthread_getspecific() failed: Returned %p vs %p\n",
             data, (FAR void *)0xf00dface);
    }

  /* Destroy the pthread key */

  ret = pthread_key_delete(g_pthread_key);
  if (ret != 0)
    {
      printf("ERROR: pthread_key_delete() failed: %d\n", ret);
    }
}
