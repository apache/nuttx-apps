/****************************************************************************
 * apps/examples/nimble_bleess/bleess_gap.c
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
#include <string.h>

#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "bleess.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void bleess_gap_start_advertising(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void bleess_gap_print_address(FAR const uint8_t *address)
{
  printf("%02x:%02x:%02x:%02x:%02x:%02x",
         address[5], address[4], address[3],
         address[2], address[1], address[0]);
}

static void
bleess_gap_print_connection(FAR const struct ble_gap_conn_desc *desc)
{
  printf("handle=%u peer=", desc->conn_handle);
  bleess_gap_print_address(desc->peer_ota_addr.val);
  printf(" interval=%u latency=%u timeout=%u encrypted=%u bonded=%u\n",
         desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
         desc->sec_state.encrypted, desc->sec_state.bonded);
}

static int bleess_gap_event(FAR struct ble_gap_event *event, FAR void *arg)
{
  struct ble_gap_conn_desc desc;
  int ret;

  switch (event->type)
    {
      case BLE_GAP_EVENT_CONNECT:
        printf("GAP connect: status=%d\n", event->connect.status);
        if (event->connect.status == 0)
          {
            ret = ble_gap_conn_find(event->connect.conn_handle, &desc);
            if (ret == 0)
              {
                bleess_gap_print_connection(&desc);
              }
          }
        else
          {
            bleess_gap_start_advertising();
          }

        return 0;

      case BLE_GAP_EVENT_DISCONNECT:
        printf("GAP disconnect: reason=%d\n", event->disconnect.reason);
        bleess_gap_start_advertising();
        return 0;

      case BLE_GAP_EVENT_CONN_UPDATE:
        printf("GAP connection update: status=%d\n",
               event->conn_update.status);
        return 0;

      case BLE_GAP_EVENT_ADV_COMPLETE:
        printf("GAP advertising complete: reason=%d\n",
               event->adv_complete.reason);
        bleess_gap_start_advertising();
        return 0;

      case BLE_GAP_EVENT_SUBSCRIBE:
        printf("GATT subscribe: attr=%u notify=%u indicate=%u\n",
               event->subscribe.attr_handle,
               event->subscribe.cur_notify,
               event->subscribe.cur_indicate);
        return 0;

      case BLE_GAP_EVENT_MTU:
        printf("ATT MTU: conn=%u mtu=%u\n",
               event->mtu.conn_handle, event->mtu.value);
        return 0;

      default:
        return 0;
    }
}

static void bleess_gap_start_advertising(void)
{
  struct ble_gap_adv_params parameters;
  struct ble_hs_adv_fields fields;
  ble_uuid16_t service_uuid = BLE_UUID16_INIT(BLEESS_SERVICE_UUID);
  FAR const char *name;
  uint8_t own_address_type;
  int ret;

  ret = ble_hs_id_infer_auto(0, &own_address_type);
  if (ret != 0)
    {
      printf("GAP address selection failed: %d\n", ret);
      return;
    }

  memset(&fields, 0, sizeof(fields));
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  name = ble_svc_gap_device_name();
  fields.name = (FAR uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;
  fields.uuids16 = &service_uuid;
  fields.num_uuids16 = 1;
  fields.uuids16_is_complete = 1;

  ret = ble_gap_adv_set_fields(&fields);
  if (ret != 0)
    {
      printf("GAP advertising data failed: %d\n", ret);
      return;
    }

  memset(&parameters, 0, sizeof(parameters));
  parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
  parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;

  ret = ble_gap_adv_start(own_address_type, NULL, BLE_HS_FOREVER,
                          &parameters, bleess_gap_event, NULL);
  printf("GAP advertising start: ret=%d name=%s service=0x%04x\n",
         ret, name, BLEESS_SERVICE_UUID);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bleess_gap_init(void)
{
  ble_svc_gap_init();
  return ble_svc_gap_device_name_set(BLEESS_DEVICE_NAME);
}

void bleess_gap_on_reset(int reason)
{
  printf("NimBLE reset: reason=%d\n", reason);
}

void bleess_gap_on_sync(void)
{
  int ret;

  ret = ble_hs_util_ensure_addr(0);
  if (ret != 0)
    {
      printf("NimBLE identity address failed: %d\n", ret);
      return;
    }

  bleess_gap_start_advertising();
}
