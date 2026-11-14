/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/kernel/thread_stack.h
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

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_KERNEL_THREAD_STACK_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_KERNEL_THREAD_STACK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>
#include <zephyr/toolchain.h>

/* Stacks are allocated by pthread_create(); the Zephyr stack objects only
 * carry their size.
 */

typedef struct
{
  size_t size;
} k_thread_stack_t;

#define K_KERNEL_STACK_DEFINE(sym, size) \
  k_thread_stack_t sym[1] = { { (size) } }
#define K_KERNEL_STACK_MEMBER(sym, size) k_thread_stack_t sym[1]
#define K_THREAD_STACK_DEFINE(sym, size) K_KERNEL_STACK_DEFINE(sym, size)
#define K_THREAD_STACK_MEMBER(sym, size) K_KERNEL_STACK_MEMBER(sym, size)
#define K_KERNEL_STACK_SIZEOF(sym) ((sym)[0].size)
#define K_THREAD_STACK_SIZEOF(sym) K_KERNEL_STACK_SIZEOF(sym)

#endif
