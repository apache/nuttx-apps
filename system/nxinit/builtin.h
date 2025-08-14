/****************************************************************************
 * apps/system/nxinit/builtin.h
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

#ifndef __APPS_SYSTEM_NXINIT_BUILTIN_H
#define __APPS_SYSTEM_NXINIT_BUILTIN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "action.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int init_builtin_run(FAR struct action_manager_s *am,
                     int argc, FAR char **argv);
#endif
