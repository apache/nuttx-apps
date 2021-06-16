/****************************************************************************
 * apps/testing/ostest/pthread_rwlock_cancel.c
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
  pthread_rwlock_t *read_lock;
  pthread_rwlock_t *write_lock;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void * timeout_thread1(FAR void * data)
{
  FAR struct sync_s * sync = (FAR struct sync_s *) data;
  struct timespec time;
  int status;

  while(1)
    {
      clock_gettime(CLOCK_REALTIME, &time);
      time.tv_sec += 1;

      status = pthread_rwlock_timedrdlock(sync->write_lock, &time);
      if (status != ETIMEDOUT)
        {
          printf("pthread_rwlock_cancel: ERROR Acquired held write_lock. Status: %d\n", status);
        }
    }

  return NULL;
}

static void * timeout_thread2(FAR void * data)
{
  FAR struct sync_s * sync = (FAR struct sync_s *) data;
  struct timespec time;
  int status;

  while (1)
    {
      clock_gettime(CLOCK_REALTIME, &time);
      time.tv_sec += 1;

      status = pthread_rwlock_timedrdlock(sync->read_lock, &time);
      if (status != 0)
        {
          printf("pthread_rwlock_cancel: Failed to acquire read_lock. Status: %d\n", status);
        }

      sched_yield(); /* Not a cancellation point. */

      if (status == 0)
        {
          status = pthread_rwlock_unlock(sync->read_lock);
          if (status != 0)
            {
              printf("pthread_rwlock_cancel: Failed to release read_lock. Status: %d\n", status);
            }
        }

      clock_gettime(CLOCK_REALTIME, &time);
      time.tv_sec += 1;

      status = pthread_rwlock_timedwrlock(sync->read_lock, &time);
      if (status != ETIMEDOUT)
        {
          printf("pthread_rwlock_cancel: "
                 "ERROR Acquired held read_lock for writing. Status: %d\n", status);
        }
    }

  return NULL;
}

static void test_timeout(void)
{
  pthread_rwlock_t read_lock;
  pthread_rwlock_t write_lock;
  struct sync_s sync;
  pthread_t thread1, thread2;
  int status, i;

  status = pthread_rwlock_init(&read_lock, NULL);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_rwlock_init(read_lock), status=%d\n",
              status);
    }

  status = pthread_rwlock_init(&write_lock, NULL);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_rwlock_init(write_lock), status=%d\n",
              status);
    }

  status = pthread_rwlock_rdlock(&read_lock);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_rwlock_rdlock, status=%d\n",
              status);
    }

  status = pthread_rwlock_wrlock(&write_lock);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_rwlock_wrlock, status=%d\n",
              status);
    }

  sync.read_lock = &read_lock;
  sync.write_lock = &write_lock;

  status = pthread_create(&thread1, NULL, timeout_thread1, &sync);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_create, status=%d\n", status);
    }

  status = pthread_create(&thread2, NULL, timeout_thread2, &sync);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_create, status=%d\n", status);
    }

  for (i = 0; i < 10; i++)
    {
      usleep(300 * 1000); /* Give threads few seconds to run */
    }

  status = pthread_cancel(thread1);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_cancel, status=%d\n", status);
    }

  status = pthread_cancel(thread2);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: ERROR pthread_cancel, status=%d\n", status);
    }

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  /* Do some operations on locks in order to check if they are still in
   * usable state after deferred cancellation. */

#ifdef CONFIG_PTHREAD_CLEANUP
#ifdef CONFIG_CANCELLATION_POINTS
  status = pthread_rwlock_trywrlock(&write_lock);
  if (status != EBUSY)
    {
      printf("pthread_rwlock_cancel: "
             "ERROR able to acquire write lock when write lock already acquired, "
             "status=%d\n", status);
    }

  status = pthread_rwlock_tryrdlock(&write_lock);
  if (status != EBUSY)
    {
      printf("pthread_rwlock_cancel: "
             "ERROR able to acquire read lock when write lock already acquired, "
             "status=%d\n", status);
    }

  status = pthread_rwlock_unlock(&read_lock);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: "
             "ERROR pthread_rwlock_unlock, status=%d\n", status);
    }

  status = pthread_rwlock_unlock(&write_lock);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: "
             "ERROR pthread_rwlock_unlock, status=%d\n", status);
    }

  status = pthread_rwlock_rdlock(&read_lock);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: "
             "ERROR pthread_rwlock_rdlock, status=%d\n", status);
    }

  status = pthread_rwlock_wrlock(&write_lock);
  if (status != 0)
    {
      printf("pthread_rwlock_cancel: "
             "ERROR pthread_rwlock_wrlock, status=%d\n", status);
    }
#endif /* CONFIG_CANCELLATION_POINTS */
#endif /* CONFIG_PTHREAD_CLEANUP */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pthread_rwlock_cancel_test(void)
{
  printf("pthread_rwlock_cancel: Starting test\n");
  test_timeout();
}
