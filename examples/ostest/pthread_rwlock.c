/****************************************************************************
 * apps/examples/pthread_rwlock.c
 *
 *   Copyright (C) 2017 Mark Schulte. All rights reserved.
 *   Author: Mark Schulte <mark@mjs.pw>
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

struct race_cond_s
{
  sem_t * sem1;
  sem_t * sem2;
  pthread_rwlock_t *rw_lock;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *race_cond_thread1(FAR void *data)
{
  FAR struct race_cond_s *rc = (FAR struct race_cond_s *) data;
  int status;

  /* Runs 1st */

  status = pthread_rwlock_wrlock(rc->rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: Failed to lock for writing\n");
    }

  sem_post(rc->sem2);
  sem_wait(rc->sem1);

  /* Context Switch -> Runs 3rd */

  status = pthread_rwlock_unlock(rc->rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: Failed to unlock lock held for writing\n");
    }

  status = pthread_rwlock_rdlock(rc->rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: Failed to open rwlock for reading. Status: %d\n", status);
    }

  sem_wait(rc->sem1);

  /* Context Switch - Runs 5th */

  status = pthread_rwlock_unlock(rc->rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: Failed to unlock lock held for writing\n");
    }

  return NULL;
}

static FAR void *race_cond_thread2(FAR void *data)
{
  FAR struct race_cond_s *rc = (FAR struct race_cond_s *) data;
  int status;

  status = sem_wait(rc->sem2);

  /* Runs 2nd */

  if (status != 0)
    {
      printf("pthread_rwlock: Failed to wait on semaphore. Status: %d\n", status);
    }

  status = pthread_rwlock_tryrdlock(rc->rw_lock);
  if (status != EBUSY)
    {
      printf("pthread_rwlock: Opened rw_lock for rd when locked for writing: %d\n", status);
    }

  status = pthread_rwlock_trywrlock(rc->rw_lock);
  if (status != EBUSY)
    {
      printf("pthread_rwlock: Opened rw_lock for wr when locked for writing: %d\n", status);
    }
  sem_post(rc->sem1);
  status = pthread_rwlock_rdlock(rc->rw_lock);

  /* Context - Switch Runs 4th */

  if (status != 0)
    {
      printf("pthread_rwlock: Failed to open rwlock for reading. Status: %d\n", status);
    }

  status = pthread_rwlock_unlock(rc->rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: Failed to unlock lock held for writing\n");
    }

  sem_post(rc->sem1);
  return NULL;
}

static void test_two_threads(void)
{
  pthread_rwlock_t rw_lock;
  struct race_cond_s rc;
  pthread_t thread1;
  pthread_t thread2;
  sem_t sem1;
  sem_t sem2;
  int status;

  status = pthread_rwlock_init(&rw_lock, NULL);
  if (status != 0)
    {
      printf("pthread_rwlock: ERROR pthread_rwlock_init failed, status=%d\n", status);
    }

  status = sem_init(&sem1, 0, 0);
  if (status != 0)
    {
      printf("pthread_rwlock: ERROR sem_init failed, status=%d\n", status);
    }

  status = sem_init(&sem2, 0, 0);
  if (status != 0)
    {
      printf("pthread_rwlock: ERROR sem_init failed, status=%d\n", status);
    }

  rc.sem1 = &sem1;
  rc.sem2 = &sem2;
  rc.rw_lock = &rw_lock;

  status = pthread_create(&thread1, NULL, race_cond_thread1, &rc);
  status = pthread_create(&thread2, NULL, race_cond_thread2, &rc);
  (void) pthread_join(thread1, NULL);
  (void) pthread_join(thread2, NULL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pthread_rwlock_test(void)
{
  pthread_rwlock_t rw_lock;
  int status;

  printf("pthread_rwlock: Initializing rwlock\n");

  status = pthread_rwlock_init(&rw_lock, NULL);
  if (status != 0)
    {
      printf("pthread_rwlock: "
             "ERROR pthread_rwlock_init failed, status=%d\n",
             status);
    }

  status = pthread_rwlock_trywrlock(&rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: "
             "ERROR pthread_rwlock_trywrlock failed, status=%d\n",
             status);
    }

  status = pthread_rwlock_unlock(&rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: "
             "ERROR pthread_rwlock_unlock failed, status=%d\n",
             status);
    }

  status = pthread_rwlock_trywrlock(&rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: "
             "ERROR pthread_rwlock_trywrlock failed, status=%d\n",
             status);
    }

  status = pthread_rwlock_trywrlock(&rw_lock);
  if (status != EBUSY)
    {
      printf("pthread_rwlock: ""ERROR able to acquire to write locks\n");
    }

  status = pthread_rwlock_tryrdlock(&rw_lock);
  if (status != EBUSY)
    {
      printf("pthread_rwlock: "
             "ERROR able to acquire read lock when read lock already acquired\n");
    }

  status = pthread_rwlock_unlock(&rw_lock);
  if (status != 0)
    {
      printf("pthread_rwlock: "
             "ERROR pthread_rwlock_unlock failed, status=%d\n",
             status);
    }

  test_two_threads();
}
