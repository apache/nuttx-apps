/****************************************************************************
 * apps/system/zbus/zbus_priv.h
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

#ifndef __APPS_SYSTEM_ZBUS_ZBUS_PRIV_H
#define __APPS_SYSTEM_ZBUS_ZBUS_PRIV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <semaphore.h>
#include <stdint.h>
#include <time.h>

#include <system/zbus.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Boundary symbols of the zbus iterable sections */

STRUCT_SECTION_DECLARE(zbus_channel);
STRUCT_SECTION_DECLARE(zbus_observer);
STRUCT_SECTION_DECLARE(zbus_channel_observation);

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Deadline computed once per API call and honored by every internal wait */

enum zbus_deadline_mode_e
{
  ZBUS_DEADLINE_FOREVER = 0,
  ZBUS_DEADLINE_NOWAIT,
  ZBUS_DEADLINE_ABS
};

struct zbus_deadline
{
  enum zbus_deadline_mode_e mode;
  struct timespec abs;          /* CLOCK_MONOTONIC absolute deadline */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* One-time lazy initialization (observation indexes, observer queues) */

void zbus_port_init_once(void);

/* Deadline helpers */

void zbus_deadline_init(int32_t timeout_ms, struct zbus_deadline *d);

/* Take a semaphore honoring the deadline.  Returns 0, -EBUSY (no-wait) or
 * -EAGAIN (timed out).
 */

int zbus_sem_take(sem_t *sem, const struct zbus_deadline *d);

#endif /* __APPS_SYSTEM_ZBUS_ZBUS_PRIV_H */
