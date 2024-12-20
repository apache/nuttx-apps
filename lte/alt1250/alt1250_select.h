/****************************************************************************
 * apps/lte/alt1250/alt1250_select.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_SELECT_H
#define __APPS_LTE_ALT1250_ALT1250_SELECT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "alt1250_daemon.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void init_selectcontainer(FAR struct alt1250_s *dev);
uint64_t perform_select_event(FAR struct alt1250_s *dev, uint64_t bitmap);
void restart_select(FAR struct alt1250_s *dev);

#endif  /* __APPS_LTE_ALT1250_ALT1250_SELECT_H */
