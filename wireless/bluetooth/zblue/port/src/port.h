/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/src/port.h
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

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_SRC_PORT_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_SRC_PORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <time.h>

#include <zephyr/kernel.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Release the coop lock before blocking, and take it back after.  Never
 * call z_zblue_block_end() while holding another lock: the coop lock is
 * always the outermost one.
 */

int z_zblue_block_begin(void);
void z_zblue_block_end(int depth);

void z_zblue_set_self(struct k_thread *thread);
int z_zblue_priority(int prio);
int z_zblue_thread_start(struct k_thread *thread, size_t stack_size,
                         int prio);

/* Timeouts: absolute uptime in ticks (ms), INT64_MAX for K_FOREVER */

int64_t z_zblue_deadline(k_timeout_t timeout);
void z_zblue_abstime(k_timeout_t timeout, struct timespec *ts);

/* Wait on cond with mutex held, until woken or the deadline passes.
 * Returns ETIMEDOUT on timeout.
 */

int z_zblue_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                      int64_t deadline);

void z_zblue_kernel_init(void);

#endif /* __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_SRC_PORT_H */
