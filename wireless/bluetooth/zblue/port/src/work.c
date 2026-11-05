/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/src/work.c
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

/* Work queues with the semantics of kernel/work.c in Zephyr (return
 * values, RUNNING/QUEUED/DELAYED/CANCELING flags, resubmission to the
 * queue a work item runs on).  Each queue is served by one thread, which
 * also fires its delayed items.  One lock protects all work state; the
 * waits for cancel_sync() and flush() use one shared condition.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <zephyr/kernel.h>

#include "port.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_work_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_work_done = PTHREAD_COND_INITIALIZER;

struct k_work_q k_sys_work_q;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int submit_locked(struct k_work *work, struct k_work_q *queue)
{
  int ret;

  if (work->flags & K_WORK_CANCELING)
    {
      return -EBUSY;
    }

  if (work->flags & K_WORK_QUEUED)
    {
      return 0;
    }

  ret = 1;
  if (queue == NULL)
    {
      queue = work->queue;
    }

  if (work->flags & K_WORK_RUNNING)
    {
      queue = work->queue;
      ret = 2;
    }

  if (queue == NULL)
    {
      return -EINVAL;
    }

  sys_slist_append(&queue->pending, &work->node);
  work->flags |= K_WORK_QUEUED;
  work->queue = queue;
  pthread_cond_signal(&queue->cond);
  return ret;
}

static bool unschedule_locked(struct k_work_delayable *dwork)
{
  if ((dwork->work.flags & K_WORK_DELAYED) == 0)
    {
      return false;
    }

  dwork->work.flags &= ~K_WORK_DELAYED;
  sys_dlist_remove(&dwork->node);
  return true;
}

static int schedule_locked(struct k_work_q *queue,
                           struct k_work_delayable *dwork,
                           k_timeout_t delay)
{
  if (K_TIMEOUT_EQ(delay, K_NO_WAIT))
    {
      return submit_locked(&dwork->work, queue);
    }

  dwork->work.flags |= K_WORK_DELAYED;
  dwork->queue = queue;
  dwork->expiry = z_zblue_deadline(delay);
  sys_dlist_append(&queue->delayed, &dwork->node);
  pthread_cond_signal(&queue->cond);
  return 1;
}

static int cancel_async_locked(struct k_work *work)
{
  if ((work->flags & K_WORK_CANCELING) == 0 &&
      (work->flags & K_WORK_QUEUED))
    {
      sys_slist_find_and_remove(&work->queue->pending, &work->node);
      work->flags &= ~K_WORK_QUEUED;
      pthread_cond_broadcast(&g_work_done);
    }

  if (work->flags & K_WORK_MASK)
    {
      work->flags |= K_WORK_CANCELING;
    }

  return work->flags & K_WORK_MASK;
}

static void wait_locked(struct k_work *work, uint32_t mask)
{
  int depth;

  if ((work->flags & mask) == 0)
    {
      return;
    }

  /* g_work_lock is dropped while the coop lock is released, so the coop
   * lock is never taken with g_work_lock held.
   */

  pthread_mutex_unlock(&g_work_lock);
  depth = z_zblue_block_begin();
  pthread_mutex_lock(&g_work_lock);
  while (work->flags & mask)
    {
      pthread_cond_wait(&g_work_done, &g_work_lock);
    }

  pthread_mutex_unlock(&g_work_lock);
  z_zblue_block_end(depth);
  pthread_mutex_lock(&g_work_lock);
}

/* Move the delayed items of a queue that are due to its pending list and
 * return the time of the next one.
 */

static int64_t fire_delayed_locked(struct k_work_q *queue)
{
  struct k_work_delayable *dwork;
  struct k_work_delayable *next;
  int64_t now = k_uptime_ticks();
  int64_t first = INT64_MAX;

  SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&queue->delayed, dwork, next, node)
    {
      if (dwork->expiry <= now)
        {
          unschedule_locked(dwork);
          submit_locked(&dwork->work, dwork->queue);
        }
      else if (dwork->expiry < first)
        {
          first = dwork->expiry;
        }
    }

  return first;
}

static void work_queue_main(void *p1, void *p2, void *p3)
{
  struct k_work_q *queue = p1;
  struct k_work *work;
  sys_snode_t *node;
  int64_t next;
  int depth;

  UNUSED(p2);
  UNUSED(p3);

  pthread_mutex_lock(&g_work_lock);
  for (; ; )
    {
      next = fire_delayed_locked(queue);
      node = sys_slist_get(&queue->pending);
      if (node == NULL)
        {
          pthread_mutex_unlock(&g_work_lock);
          depth = z_zblue_block_begin();
          pthread_mutex_lock(&g_work_lock);

          /* Items may have been submitted or scheduled while
           * g_work_lock was dropped: look again before waiting.
           */

          next = fire_delayed_locked(queue);
          if (sys_slist_is_empty(&queue->pending))
            {
              z_zblue_cond_wait(&queue->cond, &g_work_lock, next);
            }

          pthread_mutex_unlock(&g_work_lock);
          z_zblue_block_end(depth);
          pthread_mutex_lock(&g_work_lock);
          continue;
        }

      work = CONTAINER_OF(node, struct k_work, node);
      work->flags &= ~K_WORK_QUEUED;
      work->flags |= K_WORK_RUNNING;
      pthread_mutex_unlock(&g_work_lock);

      work->handler(work);

      pthread_mutex_lock(&g_work_lock);
      work->flags &= ~(K_WORK_RUNNING | K_WORK_CANCELING);
      pthread_cond_broadcast(&g_work_done);

      if ((queue->flags & 1) == 0)
        {
          /* Not no_yield: let other threads of the same priority run */

          pthread_mutex_unlock(&g_work_lock);
          k_yield();
          pthread_mutex_lock(&g_work_lock);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void k_work_init(struct k_work *work, k_work_handler_t handler)
{
  memset(work, 0, sizeof(*work));
  work->handler = handler;
}

int k_work_busy_get(const struct k_work *work)
{
  int ret;

  pthread_mutex_lock(&g_work_lock);
  ret = work->flags & K_WORK_MASK;
  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

int k_work_submit_to_queue(struct k_work_q *queue, struct k_work *work)
{
  int ret;

  pthread_mutex_lock(&g_work_lock);
  ret = submit_locked(work, queue);
  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

int k_work_submit(struct k_work *work)
{
  return k_work_submit_to_queue(&k_sys_work_q, work);
}

bool k_work_flush(struct k_work *work, struct k_work_sync *sync)
{
  bool busy;

  UNUSED(sync);
  pthread_mutex_lock(&g_work_lock);
  busy = (work->flags & (K_WORK_QUEUED | K_WORK_RUNNING)) != 0;
  wait_locked(work, K_WORK_QUEUED | K_WORK_RUNNING);
  pthread_mutex_unlock(&g_work_lock);
  return busy;
}

int k_work_cancel(struct k_work *work)
{
  int ret;

  pthread_mutex_lock(&g_work_lock);
  ret = cancel_async_locked(work);
  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

bool k_work_cancel_sync(struct k_work *work, struct k_work_sync *sync)
{
  bool pending;

  UNUSED(sync);
  pthread_mutex_lock(&g_work_lock);
  pending = (work->flags & K_WORK_MASK) != 0;
  if (pending)
    {
      cancel_async_locked(work);
      wait_locked(work, K_WORK_CANCELING);
    }

  pthread_mutex_unlock(&g_work_lock);
  return pending;
}

void k_work_queue_init(struct k_work_q *queue)
{
  memset(queue, 0, sizeof(*queue));
  sys_slist_init(&queue->pending);
  sys_dlist_init(&queue->delayed);
  pthread_cond_init(&queue->cond, NULL);
}

void k_work_queue_start(struct k_work_q *queue, k_thread_stack_t *stack,
                        size_t stack_size, int prio,
                        const struct k_work_queue_config *cfg)
{
  UNUSED(stack);

  if (queue->delayed.head == NULL)
    {
      k_work_queue_init(queue);
    }

  queue->flags = (cfg != NULL && cfg->no_yield) ? 1 : 0;
  queue->thread.name = (cfg != NULL && cfg->name != NULL) ?
                       cfg->name : "zblue_wq";
  queue->thread.entry = work_queue_main;
  queue->thread.p1 = queue;
  queue->thread_id = &queue->thread;

  if (z_zblue_thread_start(&queue->thread, stack_size, prio) < 0)
    {
      __ASSERT(0, "cannot start work queue %s", queue->thread.name);
    }
}

void k_work_init_delayable(struct k_work_delayable *dwork,
                           k_work_handler_t handler)
{
  memset(dwork, 0, sizeof(*dwork));
  dwork->work.handler = handler;
}

int k_work_delayable_busy_get(const struct k_work_delayable *dwork)
{
  return k_work_busy_get(&dwork->work);
}

k_ticks_t k_work_delayable_remaining_get(
  const struct k_work_delayable *dwork)
{
  k_ticks_t ret = 0;
  int64_t now;

  pthread_mutex_lock(&g_work_lock);
  if (dwork->work.flags & K_WORK_DELAYED)
    {
      now = k_uptime_ticks();
      ret = dwork->expiry > now ? dwork->expiry - now : 0;
    }

  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

int k_work_schedule_for_queue(struct k_work_q *queue,
                              struct k_work_delayable *dwork,
                              k_timeout_t delay)
{
  int ret = 0;

  pthread_mutex_lock(&g_work_lock);
  if ((dwork->work.flags & K_WORK_MASK & ~K_WORK_RUNNING) == 0)
    {
      ret = schedule_locked(queue, dwork, delay);
    }

  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
  return k_work_schedule_for_queue(&k_sys_work_q, dwork, delay);
}

int k_work_reschedule_for_queue(struct k_work_q *queue,
                                struct k_work_delayable *dwork,
                                k_timeout_t delay)
{
  int ret;

  pthread_mutex_lock(&g_work_lock);
  unschedule_locked(dwork);
  ret = schedule_locked(queue, dwork, delay);
  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
  return k_work_reschedule_for_queue(&k_sys_work_q, dwork, delay);
}

bool k_work_flush_delayable(struct k_work_delayable *dwork,
                            struct k_work_sync *sync)
{
  struct k_work *work = &dwork->work;

  UNUSED(sync);
  pthread_mutex_lock(&g_work_lock);
  if ((work->flags & K_WORK_MASK) == 0)
    {
      pthread_mutex_unlock(&g_work_lock);
      return false;
    }

  if (unschedule_locked(dwork))
    {
      submit_locked(work, dwork->queue);
    }

  wait_locked(work, K_WORK_QUEUED | K_WORK_RUNNING);
  pthread_mutex_unlock(&g_work_lock);
  return true;
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
  int ret;

  pthread_mutex_lock(&g_work_lock);
  unschedule_locked(dwork);
  ret = cancel_async_locked(&dwork->work);
  pthread_mutex_unlock(&g_work_lock);
  return ret;
}

bool k_work_cancel_delayable_sync(struct k_work_delayable *dwork,
                                  struct k_work_sync *sync)
{
  bool pending;

  UNUSED(sync);
  pthread_mutex_lock(&g_work_lock);
  pending = (dwork->work.flags & K_WORK_MASK) != 0;
  if (pending)
    {
      unschedule_locked(dwork);
      cancel_async_locked(&dwork->work);
      wait_locked(&dwork->work, K_WORK_CANCELING);
    }

  pthread_mutex_unlock(&g_work_lock);
  return pending;
}
