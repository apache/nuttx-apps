/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/sys/printk.h
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

/* Zephyr printk() family mapped to syslog() and the C library */

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SYS_PRINTK_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SYS_PRINTK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define printk(...)                   syslog(LOG_INFO, __VA_ARGS__)
#define vprintk(fmt, ap)              vsyslog(LOG_INFO, fmt, ap)
#define snprintk(...)                 snprintf(__VA_ARGS__)
#define vsnprintk(str, size, fmt, ap) vsnprintf(str, size, fmt, ap)

#endif /* __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_SYS_PRINTK_H */
