/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/spinlock.h
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

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SPINLOCK_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SPINLOCK_H

/* Spinlocks protect short sections against other threads.  In userspace
 * there are no interrupts to mask, so all of them map to one recursive
 * lock; nothing blocks while holding it.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct k_spinlock
{
  char unused;
};

typedef struct
{
  int key;
} k_spinlock_key_t;

/* The one lock behind all spinlocks and irq_lock(), see kernel.c */

extern pthread_mutex_t g_zblue_spin;

static inline k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
  k_spinlock_key_t key =
    {
      0
    };

  (void)l;
  pthread_mutex_lock(&g_zblue_spin);
  return key;
}

static inline int k_spin_trylock(struct k_spinlock *l, k_spinlock_key_t *k)
{
  (void)l;
  (void)k;
  return pthread_mutex_trylock(&g_zblue_spin) == 0 ? 0 : -EBUSY;
}

static inline void k_spin_unlock(struct k_spinlock *l, k_spinlock_key_t key)
{
  (void)l;
  (void)key;
  pthread_mutex_unlock(&g_zblue_spin);
}

#define K_SPINLOCK(lck) \
  for (k_spinlock_key_t __k = k_spin_lock(lck), *__once = &__k; __once; \
       k_spin_unlock(lck, __k), __once = NULL)

static inline unsigned int irq_lock(void)
{
  pthread_mutex_lock(&g_zblue_spin);
  return 0;
}

static inline void irq_unlock(unsigned int key)
{
  (void)key;
  pthread_mutex_unlock(&g_zblue_spin);
}

#ifdef __cplusplus
}
#endif

#endif
