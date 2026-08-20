/****************************************************************************
 * apps/system/expat/expat_config.h
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

#ifndef __APPS_SYSTEM_EXPAT_EXPAT_CONFIG_H
#define __APPS_SYSTEM_EXPAT_EXPAT_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ENDIAN_BIG
#  define BYTEORDER 4321
#  define WORDS_BIGENDIAN 1
#else
#  define BYTEORDER 1234
#endif

#define HAVE_ARC4RANDOM_BUF 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define PACKAGE "expat"
#define PACKAGE_NAME "expat"
#define PACKAGE_STRING "expat " CONFIG_LIB_EXPAT_VERSION
#define PACKAGE_TARNAME "expat"
#define PACKAGE_URL "https://libexpat.github.io/"
#define PACKAGE_VERSION CONFIG_LIB_EXPAT_VERSION
#define STDC_HEADERS 1
#define XML_CONTEXT_BYTES CONFIG_LIB_EXPAT_CONTEXT_BYTES
#ifdef CONFIG_LIB_EXPAT_GENERAL_ENTITIES
#  define XML_GE 1
#else
#  define XML_GE 0
#endif
#define XML_NS 1

#endif /* __APPS_SYSTEM_EXPAT_EXPAT_CONFIG_H */
