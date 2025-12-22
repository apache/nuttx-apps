/****************************************************************************
 * apps/testing/nuts/devices/tests.h
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

#ifndef __APPS_TESTING_NUTS_DEVICES_H
#define __APPS_TESTING_NUTS_DEVICES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "../nuts.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVNULL
int nuts_devices_devnull(void);
#else
#define nuts_devices_devnull()
#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVNULL */

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVZERO
int nuts_devices_devzero(void);
#else
#define nuts_devices_devzero()
#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVZERO */

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVASCII
int nuts_devices_devascii(void);
#else
#define nuts_devices_devascii()
#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVASCII */

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVCONSOLE
int nuts_devices_devconsole(void);
#else
#define nuts_devices_devconsole()
#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVCONSOLE */

#ifdef CONFIG_TESTING_NUTS_DEVICES_DEVURANDOM
int nuts_devices_devurandom(void);
#else
#define nuts_devices_devurandom()
#endif /* CONFIG_TESTING_NUTS_DEVICES_DEVURANDOM */

#endif /* __APPS_TESTING_NUTS_DEVICES_H */
