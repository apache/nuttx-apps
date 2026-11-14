/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/init.h
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

/* SYS_INIT() without a linker script: entries record their level and
 * priority, port/src/misc.c sorts and runs them before the Bluetooth host
 * starts.
 */

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_INIT_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_INIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include_next <zephyr/init.h>

struct z_zblue_init
{
  int (*init_fn)(void);
  int level;
  int prio;
};

#define Z_ZBLUE_INIT_LEVEL_EARLY        0
#define Z_ZBLUE_INIT_LEVEL_PRE_KERNEL_1 1
#define Z_ZBLUE_INIT_LEVEL_PRE_KERNEL_2 2
#define Z_ZBLUE_INIT_LEVEL_POST_KERNEL  3
#define Z_ZBLUE_INIT_LEVEL_APPLICATION  4
#define Z_ZBLUE_INIT_LEVEL_SMP          5

#undef SYS_INIT_NAMED
#define SYS_INIT_NAMED(name, init_fn_, level_, prio_) \
  static const struct z_zblue_init _CONCAT(z_zblue_init_, name) \
  __attribute__((__section__("zblue_init"))) __used = \
  { \
    .init_fn = (init_fn_), \
    .level = Z_ZBLUE_INIT_LEVEL_##level_, \
    .prio = (prio_), \
  }

#endif /* __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_INIT_H */
