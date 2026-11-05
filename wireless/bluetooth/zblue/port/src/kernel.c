/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/src/kernel.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <zephyr/kernel.h>

#include "port.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Zephyr stack sizes are tuned for 32-bit MCUs and no C library stdio.
 * Give each thread some headroom on top.
 */

#define ZBLUE_STACK_EXTRA (2048 * sizeof(void *) / 4)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_coop = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_spin;
static pthread_once_t g_once = PTHREAD_ONCE_INIT;
static pthread_key_t g_self;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void kernel_once(void)
{
  pthread_mutexattr_t attr;
  int ret;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&g_spin, &attr);
  pthread_mutexattr_destroy(&attr);
  ret = pthread_key_create(&g_self, free);
  __ASSERT(ret == 0, "pthread_key_create: %d", ret);
  ARG_UNUSED(ret);
}

static void coop_lock(struct k_thread *self)
{
  if (self->coop_depth++ == 0)
    {
      pthread_mutex_lock(&g_coop);
    }
}

static void coop_unlock(struct k_thread *self)
{
  if (--self->coop_depth == 0)
    {
      pthread_mutex_unlock(&g_coop);
    }
}

static void *thread_main(void *arg)
{
  struct k_thread *thread = arg;

  z_zblue_set_self(thread);

  if (thread->coop)
    {
      coop_lock(thread);
    }

  thread->entry(thread->p1, thread->p2, thread->p3);

  if (thread->coop)
    {
      coop_unlock(thread);
    }

  /* The record is not heap allocated, keep the key destructor away */

  pthread_setspecific(g_self, NULL);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void z_zblue_set_self(struct k_thread *thread)
{
  pthread_once(&g_once, kernel_once);
  pthread_setspecific(g_self, thread);
}

k_tid_t k_current_get(void)
{
  struct k_thread *self;

  pthread_once(&g_once, kernel_once);
  self = pthread_getspecific(g_self);
  if (self == NULL)
    {
      /* A thread not created by zblue, e.g. the application calling
       * bt_enable().  It is a preemptive thread.  The key destructor
       * frees the record when the thread exits.
       */

      self = calloc(1, sizeof(*self));
      __ASSERT_NO_MSG(self != NULL);
      self->pthread = pthread_self();
      self->name = "app";
      pthread_setspecific(g_self, self);
    }

  return self;
}

int z_zblue_block_begin(void)
{
  struct k_thread *self = k_current_get();
  int depth = self->coop_depth;

  if (depth > 0)
    {
      self->coop_depth = 0;
      pthread_mutex_unlock(&g_coop);
    }

  return depth;
}

void z_zblue_block_end(int depth)
{
  struct k_thread *self;

  if (depth > 0)
    {
      self = k_current_get();
      pthread_mutex_lock(&g_coop);
      self->coop_depth = depth;
    }
}

void k_sched_lock(void)
{
  coop_lock(k_current_get());
}

void k_sched_unlock(void)
{
  coop_unlock(k_current_get());
}

int z_zblue_priority(int prio)
{
  int nxprio = CONFIG_ZBLUE_THREAD_PRIORITY - prio;

  return CLAMP(nxprio, SCHED_PRIORITY_MIN + 1, SCHED_PRIORITY_MAX - 1);
}

int z_zblue_thread_start(struct k_thread *thread, size_t stack_size,
                         int prio)
{
  struct sched_param param;
  pthread_attr_t attr;
  int ret;

  pthread_once(&g_once, kernel_once);

  thread->prio = prio;
  thread->coop = prio < 0;
  thread->coop_depth = 0;

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, stack_size + ZBLUE_STACK_EXTRA);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = z_zblue_priority(prio);
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

  ret = pthread_create(&thread->pthread, &attr, thread_main, thread);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      return -ret;
    }

  if (thread->name != NULL)
    {
      pthread_setname_np(thread->pthread, thread->name);
    }

  return 0;
}

k_tid_t k_thread_create(struct k_thread *new_thread, k_thread_stack_t *stack,
                        size_t stack_size,
                        void (*entry)(void *, void *, void *),
                        void *p1, void *p2, void *p3,
                        int prio, uint32_t options, k_timeout_t delay)
{
  UNUSED(stack);
  UNUSED(options);
  __ASSERT(K_TIMEOUT_EQ(delay, K_NO_WAIT), "delayed start not supported");

  new_thread->entry = entry;
  new_thread->p1 = p1;
  new_thread->p2 = p2;
  new_thread->p3 = p3;

  if (z_zblue_thread_start(new_thread, stack_size, prio) < 0)
    {
      return NULL;
    }

  return new_thread;
}

int k_thread_name_set(k_tid_t thread, const char *str)
{
  thread->name = str;
  if (thread->pthread != 0)
    {
      pthread_setname_np(thread->pthread, str);
    }

  return 0;
}

void k_yield(void)
{
  int depth = z_zblue_block_begin();

  sched_yield();
  z_zblue_block_end(depth);
}

int32_t k_sleep(k_timeout_t timeout)
{
  struct timespec ts;
  int depth;

  if (K_TIMEOUT_EQ(timeout, K_FOREVER))
    {
      depth = z_zblue_block_begin();
      for (; ; )
        {
          pause();
        }
    }

  z_zblue_abstime(timeout, &ts);
  depth = z_zblue_block_begin();
  while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) ==
         EINTR);
  z_zblue_block_end(depth);
  return 0;
}

int32_t k_msleep(int32_t ms)
{
  return k_sleep(K_MSEC(ms));
}

void k_busy_wait(uint32_t usec_to_wait)
{
  usleep(usec_to_wait);
}

/* Time: one tick is one millisecond, one cycle one microsecond */

int64_t k_uptime_ticks(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t k_cycle_get_64(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

uint32_t k_cycle_get_32(void)
{
  return (uint32_t)k_cycle_get_64();
}

int64_t z_zblue_deadline(k_timeout_t timeout)
{
  if (K_TIMEOUT_EQ(timeout, K_FOREVER))
    {
      return INT64_MAX;
    }

  if (timeout.ticks < K_TICKS_FOREVER)
    {
      /* Absolute timeout, see Z_TICK_ABS() */

      return K_TICKS_FOREVER - 1 - timeout.ticks;
    }

  return k_uptime_ticks() + timeout.ticks;
}

void z_zblue_abstime(k_timeout_t timeout, struct timespec *ts)
{
  int64_t ms = z_zblue_deadline(timeout);

  ts->tv_sec = ms / 1000;
  ts->tv_nsec = (ms % 1000) * 1000000;
}

k_timepoint_t sys_timepoint_calc(k_timeout_t timeout)
{
  k_timepoint_t tp;

  if (K_TIMEOUT_EQ(timeout, K_FOREVER))
    {
      tp.tick = UINT64_MAX;
    }
  else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT))
    {
      tp.tick = 0;
    }
  else
    {
      tp.tick = z_zblue_deadline(timeout);
    }

  return tp;
}

k_timeout_t sys_timepoint_timeout(k_timepoint_t timepoint)
{
  int64_t now;

  if (timepoint.tick == UINT64_MAX)
    {
      return K_FOREVER;
    }

  if (timepoint.tick == 0)
    {
      return K_NO_WAIT;
    }

  now = k_uptime_ticks();
  return (int64_t)timepoint.tick > now ?
         K_TICKS(timepoint.tick - now) : K_NO_WAIT;
}

/* Spinlocks and irq_lock(): one recursive lock */

k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
  k_spinlock_key_t key;

  key.key = 0;

  UNUSED(l);
  pthread_once(&g_once, kernel_once);
  pthread_mutex_lock(&g_spin);
  return key;
}

int k_spin_trylock(struct k_spinlock *l, k_spinlock_key_t *k)
{
  UNUSED(l);
  UNUSED(k);
  pthread_once(&g_once, kernel_once);
  return pthread_mutex_trylock(&g_spin) == 0 ? 0 : -EBUSY;
}

void k_spin_unlock(struct k_spinlock *l, k_spinlock_key_t key)
{
  UNUSED(l);
  UNUSED(key);
  pthread_mutex_unlock(&g_spin);
}

unsigned int irq_lock(void)
{
  k_spin_lock(NULL);
  return 0;
}

void irq_unlock(unsigned int key)
{
  k_spinlock_key_t k;

  k.key = 0;

  UNUSED(key);
  k_spin_unlock(NULL, k);
}
