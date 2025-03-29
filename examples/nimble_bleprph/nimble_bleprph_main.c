/****************************************************************************
 * apps/examples/nimble_bleprph/nimble_bleprph_main.c
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* BLE */

#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/bas/ble_svc_bas.h"

/* Application-specified header. */

#include "bleprph.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Not used now */

#define TASK_DEFAULT_PRIORITY CONFIG_EXAMPLES_NIMBLE_BLEPRPH_PRIORITY
#define TASK_DEFAULT_STACK NULL
#define TASK_DEFAULT_STACK_SIZE 0

/****************************************************************************
 * External Functions Prototypes
 ****************************************************************************/

void ble_hci_sock_ack_handler(FAR void *param);
void ble_hci_sock_set_device(int dev);

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static void bleprph_advertise(void);
static void nimble_host_task(FAR void *param);
static FAR void *ble_hci_sock_task(FAR void *param);
static FAR void *ble_host_task(FAR void *param);
static int bleprph_gap_event(FAR struct ble_gap_event *event, FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bleprph_print_conn_desc
 ****************************************************************************/

static void bleprph_print_conn_desc(FAR struct ble_gap_conn_desc *desc)
{
  printf("handle=%d our_ota_addr_type=%d our_ota_addr=",
         desc->conn_handle, desc->our_ota_addr.type);
  print_addr(desc->our_ota_addr.val);
  printf(" our_id_addr_type=%d our_id_addr=",
         desc->our_id_addr.type);
  print_addr(desc->our_id_addr.val);
  printf(" peer_ota_addr_type=%d peer_ota_addr=",
         desc->peer_ota_addr.type);
  print_addr(desc->peer_ota_addr.val);
  printf(" peer_id_addr_type=%d peer_id_addr=",
         desc->peer_id_addr.type);
  print_addr(desc->peer_id_addr.val);
  printf(" conn_itvl=%d conn_latency=%d supervision_timeout=%d "
         "encrypted=%d authenticated=%d bonded=%d\n",
         desc->conn_itvl, desc->conn_latency,
         desc->supervision_timeout,
         desc->sec_state.encrypted,
         desc->sec_state.authenticated,
         desc->sec_state.bonded);
}

/****************************************************************************
 * Name: bleprph_advertise
 ****************************************************************************/

static void bleprph_advertise(void)
{
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  uint8_t own_addr_type;
  FAR const char *name;
  int rc;

  /* Figure out address to use while advertising (no privacy for now) */

  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0)
    {
      printf("error determining address type; rc=%d\n", rc);
      return;
    }

  /**
   *  Set the advertisement data included in our advertisements:
   *     o Flags (indicates advertisement type and other general info).
   *     o Advertising tx power.
   *     o Device name.
   *     o 16-bit service UUIDs (alert notifications).
   */

  memset(&fields, 0, sizeof fields);

  /* Advertise two flags:
   *     o Discoverability in forthcoming advertisement (general)
   *     o BLE-only (BR/EDR unsupported).
   */

  fields.flags = BLE_HS_ADV_F_DISC_GEN |
    BLE_HS_ADV_F_BREDR_UNSUP;

  /* Indicate that the TX power level field should be included; have the
   * stack fill this value automatically.  This is done by assiging the
   * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
   */

  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  name = ble_svc_gap_device_name();
  fields.name = (FAR uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  fields.uuids16 = (ble_uuid16_t[])
    {
      BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)
    };

  fields.num_uuids16 = 1;
  fields.uuids16_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0)
    {
      printf("error setting advertisement data; rc=%d\n", rc);
      return;
    }

  /* Begin advertising. */

  memset(&adv_params, 0, sizeof adv_params);
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                         &adv_params, bleprph_gap_event, NULL);
  if (rc != 0)
    {
      printf("error enabling advertisement; rc=%d\n", rc);
      return;
    }
}

/****************************************************************************
 * Name: bleprph_gap_event
 ****************************************************************************/

static int bleprph_gap_event(FAR struct ble_gap_event *event, FAR void *arg)
{
  struct ble_gap_conn_desc desc;
  int rc;

  switch (event->type)
    {
      case BLE_GAP_EVENT_CONNECT:

        /* A new connection was established or a connection attempt failed.
         */

        printf("connection %s; status=%d ",
               event->connect.status == 0 ? "established" : "failed",
               event->connect.status);
        if (event->connect.status == 0)
          {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);
          }

        printf("\n");

        if (event->connect.status != 0)
          {
            /* Connection failed; resume advertising. */

            bleprph_advertise();
          }

        return 0;

      case BLE_GAP_EVENT_DISCONNECT:
        printf("disconnect; reason=%d ", event->disconnect.reason);
        bleprph_print_conn_desc(&event->disconnect.conn);
        printf("\n");

        /* Connection terminated; resume advertising. */

        bleprph_advertise();
        return 0;

      case BLE_GAP_EVENT_CONN_UPDATE:

        /* The central has updated the connection parameters. */

        printf("connection updated; status=%d ", event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        printf("\n");
        return 0;

      case BLE_GAP_EVENT_ADV_COMPLETE:
        printf("advertise complete; reason=%d", event->adv_complete.reason);
        bleprph_advertise();
        return 0;

      case BLE_GAP_EVENT_ENC_CHANGE:

        /* Encryption has been enabled or disabled for this connection. */

        printf("encryption change event; status=%d ",
               event->enc_change.status);
        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        printf("\n");
        return 0;

      case BLE_GAP_EVENT_SUBSCRIBE:
        printf("subscribe event; conn_handle=%d attr_handle=%d "
               "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
               event->subscribe.conn_handle, event->subscribe.attr_handle,
               event->subscribe.reason, event->subscribe.prev_notify,
               event->subscribe.cur_notify, event->subscribe.prev_indicate,
               event->subscribe.cur_indicate);
        return 0;

      case BLE_GAP_EVENT_MTU:
        printf("mtu update event; conn_handle=%d cid=%d mtu=%d\n",
               event->mtu.conn_handle, event->mtu.channel_id,
               event->mtu.value);
        return 0;

      case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */

        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host
         * should continue with the pairing operation.
         */

        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

  return 0;
}

/****************************************************************************
 * Name: bleprph_on_reset
 ****************************************************************************/

static void bleprph_on_reset(int reason)
{
  printf("Resetting state; reason=%d\n", reason);
}

/****************************************************************************
 * Name: bleprph_on_sync
 ****************************************************************************/

static void bleprph_on_sync(void)
{
  int rc;

  /* Make sure we have proper identity address set (public preferred) */

  rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  /* Begin advertising. */

  bleprph_advertise();
}

/****************************************************************************
 * Name: nimble_host_task
 ****************************************************************************/

static void nimble_host_task(FAR void *param)
{
  ble_svc_gap_device_name_set("NuttX NimBLE PRPH");
  nimble_port_run();
}

/****************************************************************************
 * Name: ble_hci_sock_task
 ****************************************************************************/

static FAR void *ble_hci_sock_task(FAR void *param)
{
  ble_hci_sock_ack_handler(param);
  return NULL;
}

/****************************************************************************
 * Name: ble_host_task
 ****************************************************************************/

static FAR void *ble_host_task(FAR void *param)
{
  nimble_host_task(param);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nimble_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ble_npl_task s_task_host;
  struct ble_npl_task s_task_hci;
  uint8_t batt_level = 0;
  int ret = 0;

  /* allow to specify custom hci */

  if (argc > 1)
    {
      ble_hci_sock_set_device(atoi(argv[1]));
    }

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization */

  boardctl(BOARDIOC_INIT, 0);
#endif

#ifndef CONFIG_NSH_NETINIT
  /* Bring up the network */

  netinit_bringup();
#endif

  nimble_port_init();

  /* Initialize the NimBLE host configuration. */

  ble_hs_cfg.reset_cb = bleprph_on_reset;
  ble_hs_cfg.sync_cb = bleprph_on_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  /* Initialize services */

  ret = gatt_svr_init();
  assert(ret == 0);

  /* Create task which handles HCI socket */

  ret = ble_npl_task_init(&s_task_hci, "hci_sock", ble_hci_sock_task,
                          NULL, TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                          TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);
  if (ret != 0)
    {
      printf("ERROR: starting hci task: %i\n", ret);
    }

  /* Create task which handles default event queue for host stack. */

  ret = ble_npl_task_init(&s_task_host, "ble_host", ble_host_task,
                          NULL, TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                          TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);
  if (ret != 0)
    {
      printf("ERROR: starting ble task: %i\n", ret);
    }

  while (true)
    {
      /* Simulate battery */

      batt_level += 1;
      if (batt_level > 100)
        {
          batt_level = 0;
        }

      ble_svc_bas_battery_level_set(batt_level);
      usleep(100000);
    }

  return 0;
}
