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

k_spinlock_key_t k_spin_lock(struct k_spinlock *l);
int k_spin_trylock(struct k_spinlock *l, k_spinlock_key_t *k);
void k_spin_unlock(struct k_spinlock *l, k_spinlock_key_t key);

#define K_SPINLOCK(lck) \
  for (k_spinlock_key_t __k = k_spin_lock(lck), *__once = &__k; __once; \
       k_spin_unlock(lck, __k), __once = NULL)

unsigned int irq_lock(void);
void irq_unlock(unsigned int key);

#ifdef __cplusplus
}
#endif

#endif
