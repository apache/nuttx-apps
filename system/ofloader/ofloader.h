/****************************************************************************
 * apps/system/ofloader/ofloader.h
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

#ifndef __APPS_SYSTEM_OFLOADER_OFLOADER_H
#define __APPS_SYSTEM_OFLOADER_OFLOADER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OFLOADER_QNAME "ofloader"

#define OFLOADER_WRITE  1
#define OFLOADER_READ   2
#define OFLOADER_VERIFY 3
#define OFLOADER_SYNC   4
#define OFLOADER_ERROR  5
#define OFLOADER_FINSH  6

#ifdef CONFIG_SYSTEM_OFLOADER_DEBUG
#define OFLOADER_DEBUG(...) syslog(LOG_INFO, ##__VA_ARGS__)
#else
#define OFLOADER_DEBUG(...)
#endif

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

struct ofloader_msg
{
  int atcion;
  off_t addr;
  size_t size;
  FAR void *buff;
};

/****************************************************************************
 * Public data
 ****************************************************************************/

extern FAR void *g_create_idle;

#endif /* __APPS_SYSTEM_OFLOADER_OFLOADER_H */
