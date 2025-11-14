/****************************************************************************
 * apps/system/nxinit/property.h
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

#ifndef __APPS_SYSTEM_NXINIT_PROPERTY_H
#define __APPS_SYSTEM_NXINIT_PROPERTY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "init.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Steps to enable action events
 *
 * - Define all init_property_*() interfaces declared in this file.
 * - Data structures or functions that will likely be used:
 *   - struct action_event_s
 *   - init_action_foreach_event()
 */

void init_property_handler(FAR struct init_poller_s *ctx);
void init_property_deinit(FAR struct init_poller_s *ctx);
int  init_property_init(FAR struct init_poller_s *ctx);
int  init_property_set(FAR struct init_poller_s *ctx,
                       FAR const char *key, FAR const char *value);
#endif /* __APPS_SYSTEM_NXINIT_PROPERTY_H */
