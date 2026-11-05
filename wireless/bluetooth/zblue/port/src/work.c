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

/* Zephyr work queues on NuttX work queues (work_queue_create()).
 *
 * Each k_work_q is a NuttX work queue with one thread.  The NuttX work
 * item of a k_work only runs work_entry() on that thread when the item is
 * due.  The Zephyr state (QUEUED, DELAYED, RUNNING, CANCELING) is kept in
 * the k_work flags, under one lock, and gives the Zephyr return values;
 * work_entry() checks it and does nothing for an item that was cancelled
 * or rescheduled after NuttX had already taken it.  The waits of flush()
 * and cancel_sync() use one shared condition.
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

static void work_entry(FAR void *arg);

/* A delayed item that is due counts as queued, as in Zephyr */

static uint32_t busy_locked(const struct k_work *work)
{
  uint32_t flags = work->flags;

  if ((flags & K_WORK_DELAYED) &&
      CONTAINER_OF(work, struct k_work_delayable, work)->expiry <=
      k_uptime_ticks())
    {
      flags = (flags & ~K_WORK_DELAYED) | K_WORK_QUEUED;
    }

  return flags & K_WORK_MASK;
}

static void expire_locked(struct k_work *work)
{
  work->flags = (work->flags & ~K_WORK_MASK) | busy_locked(work);
}

/* Queue the NuttX work item for the time the k_work is due */

static void arm_locked(struct k_work *work)
{
  struct k_work_q *queue = work->queue;
  int64_t delay = 0;
  int64_t ticks;
  int ret;

  if ((work->flags & (K_WORK_QUEUED | K_WORK_DELAYED)) == 0)
    {
      return;
    }

  if ((work->flags & K_WORK_QUEUED) == 0)
    {
      delay = k_work_delayable_from_work(work)->expiry;
      if (delay == INT64_MAX)
        {
          /* K_FOREVER: never due */

          work_cancel_wq(queue->wq, &work->nxwork);
          return;
        }

      delay = MAX(delay - k_uptime_ticks(), 0);
    }

  /* Zephyr ticks are milliseconds.  A delay longer than NuttX allows
   * fires early; work_entry() then queues the item again.
   */

  ticks = (delay * 1000 + CONFIG_USEC_PER_TICK - 1) / CONFIG_USEC_PER_TICK;
  ret = work_queue_wq(queue->wq, &work->nxwork, work_entry, work,
                      MIN(ticks, WDOG_MAX_DELAY));
  __ASSERT(ret == 0, "work_queue_wq: %d", ret);
  ARG_UNUSED(ret);
}

static int submit_locked(struct k_work *work, struct k_work_q *queue)
{
  int ret;

  expire_locked(work);
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

  work->queue = queue;
  work->flags |= K_WORK_QUEUED;
  arm_locked(work);
  return ret;
}

static bool unschedule_locked(struct k_work_delayable *dwork)
{
  struct k_work *work = &dwork->work;

  if ((work->flags & K_WORK_DELAYED) == 0)
    {
      return false;
    }

  work->flags &= ~K_WORK_DELAYED;
  if ((work->flags & K_WORK_QUEUED) == 0)
    {
      work_cancel_wq(work->queue->wq, &work->nxwork);
    }

  return true;
}

static int schedule_locked(struct k_work_q *queue,
                           struct k_work_delayable *dwork,
                           k_timeout_t delay)
{
  struct k_work *work = &dwork->work;

  if (K_TIMEOUT_EQ(delay, K_NO_WAIT))
    {
      return submit_locked(work, queue);
    }

  /* A queued or running item stays on its queue */

  if ((work->flags & (K_WORK_QUEUED | K_WORK_RUNNING)) == 0)
    {
      work->queue = queue;
    }

  work->flags |= K_WORK_DELAYED;
  dwork->expiry = z_zblue_deadline(delay);
  arm_locked(work);
  return 1;
}

static int cancel_async_locked(struct k_work *work)
{
  if ((work->flags & K_WORK_CANCELING) == 0 &&
      (work->flags & K_WORK_QUEUED))
    {
      work->flags &= ~K_WORK_QUEUED;
      if ((work->flags & K_WORK_DELAYED) == 0)
        {
          work_cancel_wq(work->queue->wq, &work->nxwork);
        }

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

/* Run by NuttX on the queue thread */

static void work_entry(FAR void *arg)
{
  struct k_work *work = arg;
  struct k_work_q *queue;

  pthread_mutex_lock(&g_work_lock);
  expire_locked(work);
  queue = work->queue;
  if (k_current_get() != queue->thread_id ||
      (work->flags & K_WORK_QUEUED) == 0)
    {
      /* Not due here: cancelled, rescheduled or on another queue */

      if (work->nxwork.worker == NULL)
        {
          arm_locked(work);
        }

      pthread_mutex_unlock(&g_work_lock);
      return;
    }

  work->flags &= ~K_WORK_QUEUED;
  work->flags |= K_WORK_RUNNING;
  pthread_mutex_unlock(&g_work_lock);

  if (queue->thread.coop)
    {
      k_sched_lock();
    }

  work->handler(work);

  if (queue->thread.coop)
    {
      k_sched_unlock();
    }

  pthread_mutex_lock(&g_work_lock);
  work->flags &= ~(K_WORK_RUNNING | K_WORK_CANCELING);
  pthread_cond_broadcast(&g_work_done);

  /* Rescheduled while it ran, behind an item still queued */

  if (work->nxwork.worker == NULL)
    {
      arm_locked(work);
    }

  pthread_mutex_unlock(&g_work_lock);
}

/* First item of every queue: makes k_current_get() the queue thread */

static void queue_start_entry(FAR void *arg)
{
  struct k_work_q *queue = arg;

  queue->thread.pthread = pthread_self();
  pthread_setname_np(queue->thread.pthread, queue->thread.name);
  z_zblue_set_self(&queue->thread);
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
  ret = busy_locked(work);
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
}

void k_work_queue_start(struct k_work_q *queue, k_thread_stack_t *stack,
                        size_t stack_size, int prio,
                        const struct k_work_queue_config *cfg)
{
  UNUSED(stack);

  queue->thread.name = (cfg != NULL && cfg->name != NULL) ?
                       cfg->name : "zblue_wq";
  queue->thread.prio = prio;
  queue->thread.coop = prio < 0;
  queue->thread_id = &queue->thread;

  queue->wq = work_queue_create(queue->thread.name, z_zblue_priority(prio),
                                NULL, stack_size + ZBLUE_STACK_EXTRA, 1);
  if (queue->wq == NULL ||
      work_queue_wq(queue->wq, &queue->start, queue_start_entry,
                    queue, 0) < 0)
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
  expire_locked(&dwork->work);
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
  expire_locked(&dwork->work);
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
  expire_locked(work);
  if ((work->flags & K_WORK_MASK) == 0)
    {
      pthread_mutex_unlock(&g_work_lock);
      return false;
    }

  if (unschedule_locked(dwork))
    {
      submit_locked(work, work->queue);
    }

  wait_locked(work, K_WORK_QUEUED | K_WORK_RUNNING);
  pthread_mutex_unlock(&g_work_lock);
  return true;
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
  int ret;

  pthread_mutex_lock(&g_work_lock);
  expire_locked(&dwork->work);
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
  expire_locked(&dwork->work);
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
