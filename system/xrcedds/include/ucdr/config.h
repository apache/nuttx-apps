/****************************************************************************
 * apps/system/xrcedds/include/ucdr/config.h
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
 ****************************************************************************/

#ifndef __APPS_SYSTEM_XRCEDDS_INCLUDE_UCDR_CONFIG_H
#define __APPS_SYSTEM_XRCEDDS_INCLUDE_UCDR_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#define MICROCDR_VERSION_MAJOR 2
#define MICROCDR_VERSION_MINOR 0
#define MICROCDR_VERSION_MICRO 2
#define MICROCDR_VERSION_STR "2.0.2"

#ifdef CONFIG_ENDIAN_BIG
#  define UCDR_MACHINE_ENDIANNESS UCDR_BIG_ENDIANNESS
#else
#  define UCDR_MACHINE_ENDIANNESS UCDR_LITTLE_ENDIANNESS
#endif

#endif /* __APPS_SYSTEM_XRCEDDS_INCLUDE_UCDR_CONFIG_H */
