/****************************************************************************
 * apps/examples/nimble_bleess/bleess.h
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

#ifndef __APPS_EXAMPLES_NIMBLE_BLEESS_BLEESS_H
#define __APPS_EXAMPLES_NIMBLE_BLEESS_BLEESS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include "host/ble_gatt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLEESS_DEVICE_NAME       "NuttX NimBLE ESS"
#define BLEESS_SERVICE_UUID      0x181a
#define BLEESS_TEMPERATURE_UUID  0x2a6e
#define BLEESS_HUMIDITY_UUID     0x2a6f

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int bleess_gap_init(void);
void bleess_gap_on_reset(int reason);
void bleess_gap_on_sync(void);

int bleess_gatt_init(void);
void bleess_gatt_register_cb(FAR struct ble_gatt_register_ctxt *ctxt,
                             FAR void *arg);
void bleess_gatt_update(int16_t temperature_centi,
                        uint16_t humidity_centi);

#endif /* __APPS_EXAMPLES_NIMBLE_BLEESS_BLEESS_H */
