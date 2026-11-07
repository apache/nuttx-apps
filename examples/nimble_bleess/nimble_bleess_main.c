/****************************************************************************
 * apps/examples/nimble_bleess/nimble_bleess_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "host/ble_hs.h"
#include "host/util/util.h"
#include "netutils/netinit.h"
#include "nimble/nimble_port.h"

#include "bleess.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLEESS_TASK_PRIORITY CONFIG_EXAMPLES_NIMBLE_BLEESS_PRIORITY

/****************************************************************************
 * External Function Prototypes
 ****************************************************************************/

void ble_hci_sock_ack_handler(FAR void *param);
void ble_hci_sock_set_device(int dev);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR void *bleess_hci_task(FAR void *param)
{
  ble_hci_sock_ack_handler(param);
  return NULL;
}

static FAR void *bleess_host_task(FAR void *param)
{
  nimble_port_run();
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ble_npl_task host_task;
  struct ble_npl_task hci_task;
  int16_t temperature_centi = 2400;
  uint16_t humidity_centi = 5000;
  int direction = 1;
  int ret;

  if (argc > 1)
    {
      ble_hci_sock_set_device(atoi(argv[1]));
    }

#ifndef CONFIG_NSH_NETINIT
  netinit_bringup();
#endif

  nimble_port_init();

  ble_hs_cfg.reset_cb = bleess_gap_on_reset;
  ble_hs_cfg.sync_cb = bleess_gap_on_sync;
  ble_hs_cfg.gatts_register_cb = bleess_gatt_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ret = bleess_gap_init();
  if (ret != 0)
    {
      printf("ERROR: GAP initialization failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = bleess_gatt_init();
  if (ret != 0)
    {
      printf("ERROR: GATT initialization failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = ble_npl_task_init(&hci_task, "hci_sock", bleess_hci_task, NULL,
                          BLEESS_TASK_PRIORITY, BLE_NPL_TIME_FOREVER,
                          NULL, 0);
  if (ret != 0)
    {
      printf("ERROR: HCI task creation failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  ret = ble_npl_task_init(&host_task, "ble_host", bleess_host_task, NULL,
                          BLEESS_TASK_PRIORITY, BLE_NPL_TIME_FOREVER,
                          NULL, 0);
  if (ret != 0)
    {
      printf("ERROR: Host task creation failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  while (true)
    {
      temperature_centi += direction;
      humidity_centi += direction * 2;

      if (temperature_centi >= 2410 || temperature_centi <= 2390)
        {
          direction = -direction;
        }

      bleess_gatt_update(temperature_centi, humidity_centi);
      printf("Temperature=%d.%02d C Humidity=%u.%02u %%\n",
             temperature_centi / 100, temperature_centi % 100,
             humidity_centi / 100, humidity_centi % 100);
      sleep(2);
    }

  return EXIT_SUCCESS;
}
