/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/src/sync.c
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

/* All blocking objects follow the same pattern: the object lock is only
 * held for short sections, and a thread that has to wait first drops the
 * object lock, releases the coop lock, then waits with the object lock
 * held again.  The coop lock is retaken only after the object lock is
 * released, so the coop lock is always taken first.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <malloc.h>
#include <string.h>

#include <zephyr/kernel.h>

#include "port.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int z_zblue_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                      int64_t deadline)
{
  struct timespec ts;

  if (deadline == INT64_MAX)
    {
      return pthread_cond_wait(cond, mutex);
    }

  ts.tv_sec = deadline / 1000;
  ts.tv_nsec = (deadline % 1000) * 1000000;
  return pthread_cond_clockwait(cond, mutex, CLOCK_MONOTONIC, &ts);
}

/* Semaphores */

int k_sem_init(struct k_sem *sem, unsigned int initial_count,
               unsigned int limit)
{
  if (limit == 0 || initial_count > limit)
    {
      return -EINVAL;
    }

  pthread_mutex_init(&sem->lock, NULL);
  pthread_cond_init(&sem->cond, NULL);
  sem->count = initial_count;
  sem->limit = limit;
  return 0;
}

int k_sem_take(struct k_sem *sem, k_timeout_t timeout)
{
  int64_t deadline;
  int depth;
  int ret = 0;

  pthread_mutex_lock(&sem->lock);
  if (sem->count > 0)
    {
      sem->count--;
      pthread_mutex_unlock(&sem->lock);
      return 0;
    }

  pthread_mutex_unlock(&sem->lock);
  if (K_TIMEOUT_EQ(timeout, K_NO_WAIT))
    {
      return -EBUSY;
    }

  deadline = z_zblue_deadline(timeout);
  depth = z_zblue_block_begin();
  pthread_mutex_lock(&sem->lock);
  while (sem->count == 0 && ret != ETIMEDOUT)
    {
      ret = z_zblue_cond_wait(&sem->cond, &sem->lock, deadline);
    }

  if (sem->count > 0)
    {
      sem->count--;
      ret = 0;
    }
  else
    {
      ret = -EAGAIN;
    }

  pthread_mutex_unlock(&sem->lock);
  z_zblue_block_end(depth);
  return ret;
}

void k_sem_give(struct k_sem *sem)
{
  pthread_mutex_lock(&sem->lock);
  if (sem->count < sem->limit)
    {
      sem->count++;
    }

  pthread_cond_signal(&sem->cond);
  pthread_mutex_unlock(&sem->lock);
}

void k_sem_reset(struct k_sem *sem)
{
  pthread_mutex_lock(&sem->lock);
  sem->count = 0;
  pthread_mutex_unlock(&sem->lock);
}

unsigned int k_sem_count_get(struct k_sem *sem)
{
  unsigned int count;

  pthread_mutex_lock(&sem->lock);
  count = sem->count;
  pthread_mutex_unlock(&sem->lock);
  return count;
}

/* Mutexes */

int k_mutex_init(struct k_mutex *mutex)
{
  pthread_mutex_init(&mutex->lock, NULL);
  pthread_cond_init(&mutex->cond, NULL);
  mutex->owner = NULL;
  mutex->lock_count = 0;
  return 0;
}

int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
  struct k_thread *self = k_current_get();
  int64_t deadline;
  int depth;
  int ret = 0;

  pthread_mutex_lock(&mutex->lock);
  if (mutex->owner == NULL || mutex->owner == self)
    {
      mutex->owner = self;
      mutex->lock_count++;
      pthread_mutex_unlock(&mutex->lock);
      return 0;
    }

  pthread_mutex_unlock(&mutex->lock);
  if (K_TIMEOUT_EQ(timeout, K_NO_WAIT))
    {
      return -EBUSY;
    }

  deadline = z_zblue_deadline(timeout);
  depth = z_zblue_block_begin();
  pthread_mutex_lock(&mutex->lock);
  while (mutex->owner != NULL && ret != ETIMEDOUT)
    {
      ret = z_zblue_cond_wait(&mutex->cond, &mutex->lock, deadline);
    }

  if (mutex->owner == NULL)
    {
      mutex->owner = self;
      mutex->lock_count = 1;
      ret = 0;
    }
  else
    {
      ret = -EAGAIN;
    }

  pthread_mutex_unlock(&mutex->lock);
  z_zblue_block_end(depth);
  return ret;
}

int k_mutex_unlock(struct k_mutex *mutex)
{
  int ret = 0;

  pthread_mutex_lock(&mutex->lock);
  if (mutex->owner != k_current_get())
    {
      ret = mutex->owner == NULL ? -EINVAL : -EPERM;
    }
  else if (--mutex->lock_count == 0)
    {
      mutex->owner = NULL;
      pthread_cond_signal(&mutex->cond);
    }

  pthread_mutex_unlock(&mutex->lock);
  return ret;
}

/* Condition variables: use the lock of the k_mutex they are paired with */

int k_condvar_init(struct k_condvar *condvar)
{
  return -pthread_cond_init(&condvar->cond, NULL);
}

int k_condvar_signal(struct k_condvar *condvar)
{
  return -pthread_cond_signal(&condvar->cond);
}

int k_condvar_broadcast(struct k_condvar *condvar)
{
  return -pthread_cond_broadcast(&condvar->cond);
}

int k_condvar_wait(struct k_condvar *condvar, struct k_mutex *mutex,
                   k_timeout_t timeout)
{
  struct k_thread *self = k_current_get();
  int64_t deadline = z_zblue_deadline(timeout);
  uint32_t count;
  int depth;
  int ret;

  depth = z_zblue_block_begin();
  pthread_mutex_lock(&mutex->lock);
  __ASSERT_NO_MSG(mutex->owner == self);

  count = mutex->lock_count;
  mutex->owner = NULL;
  mutex->lock_count = 0;
  pthread_cond_signal(&mutex->cond);

  ret = K_TIMEOUT_EQ(timeout, K_NO_WAIT) ? ETIMEDOUT :
        z_zblue_cond_wait(&condvar->cond, &mutex->lock, deadline);

  while (mutex->owner != NULL)
    {
      pthread_cond_wait(&mutex->cond, &mutex->lock);
    }

  mutex->owner = self;
  mutex->lock_count = count;
  pthread_mutex_unlock(&mutex->lock);
  z_zblue_block_end(depth);
  return ret == ETIMEDOUT ? -EAGAIN : 0;
}

/* Queues */

void k_queue_init(struct k_queue *queue)
{
  sys_sflist_init(&queue->data_q);
  pthread_mutex_init(&queue->lock, NULL);
  pthread_cond_init(&queue->cond, NULL);
}

void k_queue_append(struct k_queue *queue, void *data)
{
  pthread_mutex_lock(&queue->lock);
  sys_sflist_append(&queue->data_q, data);
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->lock);
}

void k_queue_prepend(struct k_queue *queue, void *data)
{
  pthread_mutex_lock(&queue->lock);
  sys_sflist_prepend(&queue->data_q, data);
  pthread_cond_signal(&queue->cond);
  pthread_mutex_unlock(&queue->lock);
}

void *k_queue_get(struct k_queue *queue, k_timeout_t timeout)
{
  int64_t deadline;
  void *data;
  int depth;
  int ret = 0;

  pthread_mutex_lock(&queue->lock);
  data = sys_sflist_get(&queue->data_q);
  pthread_mutex_unlock(&queue->lock);
  if (data != NULL || K_TIMEOUT_EQ(timeout, K_NO_WAIT))
    {
      return data;
    }

  deadline = z_zblue_deadline(timeout);
  depth = z_zblue_block_begin();
  pthread_mutex_lock(&queue->lock);
  while ((data = sys_sflist_get(&queue->data_q)) == NULL &&
         ret != ETIMEDOUT)
    {
      ret = z_zblue_cond_wait(&queue->cond, &queue->lock, deadline);
    }

  pthread_mutex_unlock(&queue->lock);
  z_zblue_block_end(depth);
  return data;
}

bool k_queue_remove(struct k_queue *queue, void *data)
{
  bool ret;

  pthread_mutex_lock(&queue->lock);
  ret = sys_sflist_find_and_remove(&queue->data_q, data);
  pthread_mutex_unlock(&queue->lock);
  return ret;
}

void *k_queue_peek_head(struct k_queue *queue)
{
  void *data;

  pthread_mutex_lock(&queue->lock);
  data = sys_sflist_peek_head(&queue->data_q);
  pthread_mutex_unlock(&queue->lock);
  return data;
}

void *k_queue_peek_tail(struct k_queue *queue)
{
  void *data;

  pthread_mutex_lock(&queue->lock);
  data = sys_sflist_peek_tail(&queue->data_q);
  pthread_mutex_unlock(&queue->lock);
  return data;
}

int k_queue_is_empty(struct k_queue *queue)
{
  int ret;

  pthread_mutex_lock(&queue->lock);
  ret = sys_sflist_is_empty(&queue->data_q);
  pthread_mutex_unlock(&queue->lock);
  return ret;
}

void k_queue_cancel_wait(struct k_queue *queue)
{
  pthread_mutex_lock(&queue->lock);
  pthread_cond_broadcast(&queue->cond);
  pthread_mutex_unlock(&queue->lock);
}

/* Memory slabs: the free blocks are kept in a LIFO, filled on first use
 * because K_MEM_SLAB_DEFINE() must be a static initializer.
 */

static void slab_fill(struct k_mem_slab *slab)
{
  uint32_t i;

  pthread_mutex_lock(&slab->free_q._queue.lock);
  if (!slab->ready)
    {
      for (i = 0; i < slab->num_blocks; i++)
        {
          sys_sflist_append(&slab->free_q._queue.data_q,
                            (sys_sfnode_t *)(slab->buffer +
                                             i * slab->block_size));
        }

      slab->ready = true;
    }

  pthread_mutex_unlock(&slab->free_q._queue.lock);
}

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
                    size_t block_size, uint32_t num_blocks)
{
  memset(slab, 0, sizeof(*slab));
  k_lifo_init(&slab->free_q);
  slab->buffer = buffer;
  slab->block_size = block_size;
  slab->num_blocks = num_blocks;
  return 0;
}

int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem,
                     k_timeout_t timeout)
{
  if (!slab->ready)
    {
      slab_fill(slab);
    }

  *mem = k_lifo_get(&slab->free_q, timeout);
  if (*mem == NULL)
    {
      return K_TIMEOUT_EQ(timeout, K_NO_WAIT) ? -ENOMEM : -EAGAIN;
    }

  __atomic_fetch_add(&slab->num_used, 1, __ATOMIC_RELAXED);
  return 0;
}

void k_mem_slab_free(struct k_mem_slab *slab, void *mem)
{
  __atomic_fetch_sub(&slab->num_used, 1, __ATOMIC_RELAXED);
  k_lifo_put(&slab->free_q, mem);
}

uint32_t k_mem_slab_num_used_get(struct k_mem_slab *slab)
{
  return slab->num_used;
}

uint32_t k_mem_slab_num_free_get(struct k_mem_slab *slab)
{
  return slab->num_blocks - slab->num_used;
}

/* Heap */

void *k_heap_alloc(struct k_heap *h, size_t bytes, k_timeout_t timeout)
{
  UNUSED(h);
  UNUSED(timeout);
  return malloc(bytes);
}

void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
                           k_timeout_t timeout)
{
  UNUSED(h);
  UNUSED(timeout);
  return memalign(align, bytes);
}

void k_heap_free(struct k_heap *h, void *mem)
{
  UNUSED(h);
  free(mem);
}

void *k_malloc(size_t size)
{
  return malloc(size);
}

void *k_calloc(size_t nmemb, size_t size)
{
  return calloc(nmemb, size);
}

void *k_aligned_alloc(size_t align, size_t size)
{
  return memalign(align, size);
}

void k_free(void *ptr)
{
  free(ptr);
}
