/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/logging/log.h
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

/* Zephyr logging macros on top of syslog().  The level of each module is
 * fixed at build time, from LOG_LEVEL or the second argument of
 * LOG_MODULE_REGISTER()/LOG_MODULE_DECLARE(), as in Zephyr.
 */

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_LOGGING_LOG_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_LOGGING_LOG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERR  1
#define LOG_LEVEL_WRN  2
#define LOG_LEVEL_INF  3
#define LOG_LEVEL_DBG  4

#ifndef CONFIG_LOG
#  define Z_ZBLUE_LOG_MAX 0
#else
#  define Z_ZBLUE_LOG_MAX CONFIG_LOG_MAX_LEVEL
#endif

#ifdef LOG_LEVEL
#  define Z_ZBLUE_LOG_DEFAULT LOG_LEVEL
#else
#  define Z_ZBLUE_LOG_DEFAULT CONFIG_LOG_DEFAULT_LEVEL
#endif

#define Z_ZBLUE_LOG_RESOLVE(...) \
  GET_ARG_N(2, __VA_ARGS__, Z_ZBLUE_LOG_DEFAULT)

#define LOG_MODULE_REGISTER(...) \
  static const char __unused z_log_name[] = STRINGIFY(GET_ARG_N(1, __VA_ARGS__)); \
  enum { z_log_level = Z_ZBLUE_LOG_RESOLVE(__VA_ARGS__) }

#define LOG_MODULE_DECLARE(...) LOG_MODULE_REGISTER(__VA_ARGS__)

void z_zblue_log(int level, const char *module, const char *fmt, ...)
  __printf_like(3, 4);
void z_zblue_hexdump(int level, const char *module, const void *data,
                     size_t length, const char *str);

#define Z_ZBLUE_LOG(_level, fmt, ...) \
  do \
    { \
      if (_level <= z_log_level && _level <= Z_ZBLUE_LOG_MAX) \
        { \
          z_zblue_log(_level, z_log_name, fmt, ##__VA_ARGS__); \
        } \
    } \
  while (0)

#define Z_ZBLUE_HEXDUMP(_level, _data, _len, _str) \
  do \
    { \
      if (_level <= z_log_level && _level <= Z_ZBLUE_LOG_MAX) \
        { \
          z_zblue_hexdump(_level, z_log_name, _data, _len, _str); \
        } \
    } \
  while (0)

/* <syslog.h> uses LOG_ERR for a priority */

#undef LOG_ERR
#define LOG_ERR(...) Z_ZBLUE_LOG(LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_WRN(...) Z_ZBLUE_LOG(LOG_LEVEL_WRN, __VA_ARGS__)
#define LOG_INF(...) Z_ZBLUE_LOG(LOG_LEVEL_INF, __VA_ARGS__)
#define LOG_DBG(...) Z_ZBLUE_LOG(LOG_LEVEL_DBG, __VA_ARGS__)

#define LOG_HEXDUMP_ERR(d, l, s) Z_ZBLUE_HEXDUMP(LOG_LEVEL_ERR, d, l, s)
#define LOG_HEXDUMP_WRN(d, l, s) Z_ZBLUE_HEXDUMP(LOG_LEVEL_WRN, d, l, s)
#define LOG_HEXDUMP_INF(d, l, s) Z_ZBLUE_HEXDUMP(LOG_LEVEL_INF, d, l, s)
#define LOG_HEXDUMP_DBG(d, l, s) Z_ZBLUE_HEXDUMP(LOG_LEVEL_DBG, d, l, s)

#define LOG_PRINTK(...) printk(__VA_ARGS__)
#define LOG_RAW(...)    printk(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_LOGGING_LOG_H */
