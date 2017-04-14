/****************************************************************************
 * examples/ostest/pthread_cleanup.c
 *
 *   Copyright (C) 2017 Haltian Ltd. All rights reserved.
 *   Author: Juha Niskanen <juha.niskanen@haltian.com>
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

#include <pthread.h>
#include <stdio.h>
#include <errno.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sync_s
{
  pthread_cond_t cond;
  pthread_mutex_t lock;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void cleanup(FAR void * data)
{
  FAR struct sync_s *sync = (FAR struct sync_s *) data;
  int status;

  /* Note:  The behavior of canceling pthread_cond_wait() with asynchronous
   * cancellation is not defined. On NuttX we get EPERM here, but application
   * code must not rely on this.
   */

  status = pthread_mutex_unlock(&sync->lock);
#ifndef CONFIG_CANCELLATION_POINTS
  if (status == EPERM)
    {
      printf("pthread_cleanup: thread did not have mutex locked: %d\n", status);
      return;
    }
#endif
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_mutex_unlock in cleanup handler. "
             "Status: %d\n", status);
    }
}

static void *cleanup_thread(FAR void * data)
{
  FAR struct sync_s *sync = (FAR struct sync_s *) data;
  int status;

  status = pthread_mutex_lock(&sync->lock);
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_mutex_lock, status=%d\n", status);
      return NULL;
    }

  pthread_cleanup_push(&cleanup, sync);

  while(1)
    {
      status = pthread_cond_wait(&sync->cond, &sync->lock);
      if (status != 0)
        {
          printf("pthread_cleanup: ERROR wait returned. Status: %d\n", status);
        }
    }

  pthread_cleanup_pop(1);
  return NULL;
}

static void test_cleanup(void)
{
  pthread_t thread1;
  int status;
  void *result;
  struct sync_s sync = {
    PTHREAD_COND_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER
  };

  status = pthread_create(&thread1, NULL, cleanup_thread, &sync);
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_create, status=%d\n", status);
      return;
    }

  usleep(500 * 1000);

  status = pthread_cancel(thread1);
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_cancel, status=%d\n", status);
    }

  status = pthread_join(thread1, &result);
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_join, status=%d\n", status);
    }
  else if (result != PTHREAD_CANCELED)
    {
      printf("pthread_cleanup: ERROR pthread_join returned wrong result: %p\n", result);
    }

#ifdef CONFIG_CANCELLATION_POINTS
  /* Do some operations on lock in order to check if it is in usable state. */

  status = pthread_mutex_trylock(&sync.lock);
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_mutex_trylock, status=%d\n", status);
    }

  status = pthread_mutex_unlock(&sync.lock);
  if (status != 0)
    {
      printf("pthread_cleanup: ERROR pthread_mutex_unlock, status=%d\n", status);
    }
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pthread_cleanup_test(void)
{
  printf("pthread_cleanup: Starting test\n");
  test_cleanup();
}
