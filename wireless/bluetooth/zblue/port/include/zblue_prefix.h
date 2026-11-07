/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zblue_prefix.h
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

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZBLUE_PREFIX_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZBLUE_PREFIX_H

/* Included first in every zblue translation unit (-include).
 *
 * The Zephyr sources are built unmodified, so where NuttX and Zephyr
 * use the same name for different things, the Zephyr meaning has to win
 * inside zblue.  The NuttX equivalents (nuttx/atomic.h, the libc crc and
 * hex helpers) cannot be used in their place without changing the
 * Zephyr code.
 *
 * Atomics: <nuttx/atomic.h> is pulled in by C library headers (stdio.h,
 * pthread.h, semaphore.h).  It defines atomic_t as _Atomic int32_t and
 * atomic_*() as macros; Zephyr defines atomic_t as long and atomic_*()
 * as functions with other return values (e.g. atomic_set() returns the
 * previous value).  The NuttX headers are included here with the NuttX
 * type renamed, then the macros are removed.
 */

#define atomic_t   nx_atomic_t
#define atomic64_t nx_atomic64_t

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/atomic.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#undef atomic_t
#undef atomic64_t

/* Time constants: same values as in <zephyr/sys/clock.h>, which
 * defines them again.
 */

#undef NSEC_PER_USEC
#undef NSEC_PER_MSEC
#undef NSEC_PER_SEC
#undef USEC_PER_MSEC
#undef USEC_PER_SEC
#undef MSEC_PER_SEC
#undef SEC_PER_MIN
#undef SEC_PER_HOUR
#undef SEC_PER_DAY
#undef MIN_PER_HOUR
#undef HOUR_PER_DAY
#undef atomic_add
#undef atomic_add_acquire
#undef atomic_add_relaxed
#undef atomic_add_release
#undef atomic_and
#undef atomic_and_acquire
#undef atomic_and_relaxed
#undef atomic_and_release
#undef atomic_cmpxchg
#undef atomic_cmpxchg_acquire
#undef atomic_cmpxchg_relaxed
#undef atomic_cmpxchg_release
#undef atomic_or
#undef atomic_or_acquire
#undef atomic_or_relaxed
#undef atomic_or_release
#undef atomic_read
#undef atomic_read_acquire
#undef atomic_set
#undef atomic_set_release
#undef atomic_sub
#undef atomic_sub_acquire
#undef atomic_sub_relaxed
#undef atomic_sub_release
#undef atomic_try_cmpxchg
#undef atomic_try_cmpxchg_acquire
#undef atomic_try_cmpxchg_relaxed
#undef atomic_try_cmpxchg_release
#undef atomic_xchg
#undef atomic_xchg_acquire
#undef atomic_xchg_relaxed
#undef atomic_xchg_release
#undef atomic_xor
#undef atomic_xor_acquire
#undef atomic_xor_relaxed
#undef atomic_xor_release

/* Library functions: the NuttX C library has crc8(), crc16(), hex2bin()
 * etc. with other signatures than the Zephyr ones compiled in zblue
 * (e.g. crc16(src, len) and crc16(poly, seed, src, len)).  The Zephyr
 * ones are renamed so that both can be linked.
 */

/* Zephyr places some objects in a no-init RAM section; ordinary .bss is
 * equivalent here.
 */

#include <zephyr/linker/section_tags.h>
#undef __noinit
#define __noinit

#define crc8             zblue_crc8
#define crc16            zblue_crc16
#define crc16_reflect    zblue_crc16_reflect
#define crc32_ieee       zblue_crc32_ieee
#define hex2bin          zblue_hex2bin
#define bin2hex          zblue_bin2hex
#define char2hex         zblue_char2hex
#define hex2char         zblue_hex2char
#define uuid_from_string zblue_uuid_from_string
#define uuid_to_string   zblue_uuid_to_string

#endif
