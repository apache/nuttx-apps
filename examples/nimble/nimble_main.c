/****************************************************************************
 * apps/examples/nimble/nimble_main.c
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/boardctl.h>
#include <assert.h>

#include "netutils/netinit.h"

#include "nimble/nimble_npl.h"
#include "nimble/nimble_port.h"

#include "host/ble_hs.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/ans/ble_svc_ans.h"
#include "services/ias/ble_svc_ias.h"
#include "services/lls/ble_svc_lls.h"
#include "services/tps/ble_svc_tps.h"
#include "services/bas/ble_svc_bas.h"
#include "services/dis/ble_svc_dis.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Not used now */

#define TASK_DEFAULT_PRIORITY       CONFIG_EXAMPLES_NIMBLE_PRIORITY
#define TASK_DEFAULT_STACK          NULL
#define TASK_DEFAULT_STACK_SIZE     0

/****************************************************************************
 * External Functions Prototypes
 ****************************************************************************/

void ble_hci_sock_ack_handler(FAR void *param);
void ble_hci_sock_set_device(int dev);

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static void put_ad(uint8_t ad_type, uint8_t ad_len, FAR const void *ad,
                   FAR uint8_t *buf, FAR uint8_t *len);
static void update_ad(void);
static void start_advertise(void);
static int gap_event_cb(FAR struct ble_gap_event *event, FAR void *arg);
static void app_ble_sync_cb(void);
static void nimble_host_task(FAR void *param);
static FAR void *ble_hci_sock_task(FAR void *param);
static FAR void *ble_host_task(FAR void *param);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_gap_name = "NuttX NimBLE";
static uint8_t g_own_addr_type;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: put_ad
 ****************************************************************************/

static void put_ad(uint8_t ad_type, uint8_t ad_len, FAR const void *ad,
                   FAR uint8_t *buf, FAR uint8_t *len)
{
  buf[(*len)++] = ad_len + 1;
  buf[(*len)++] = ad_type;

  memcpy(&buf[*len], ad, ad_len);

  *len += ad_len;
}

/****************************************************************************
 * Name: update_ad
 ****************************************************************************/

static void update_ad(void)
{
  uint8_t ad_flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  uint8_t ad_len   = 0;
  uint8_t ad[BLE_HS_ADV_MAX_SZ];

  put_ad(BLE_HS_ADV_TYPE_FLAGS, 1, &ad_flags, ad, &ad_len);
  put_ad(BLE_HS_ADV_TYPE_COMP_NAME, sizeof(g_gap_name), g_gap_name,
         ad, &ad_len);

  ble_gap_adv_set_data(ad, ad_len);
}

/****************************************************************************
 * Name: start_advertise
 ****************************************************************************/

static void start_advertise(void)
{
  struct ble_gap_adv_params advp;
  int                       rc;

  printf("advertise\n");

  update_ad();

  memset(&advp, 0, sizeof advp);
  advp.conn_mode = BLE_GAP_CONN_MODE_UND;
  advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
  rc = ble_gap_adv_start(g_own_addr_type, NULL, BLE_HS_FOREVER,
                         &advp, gap_event_cb, NULL);
  assert(rc == 0);
}

/****************************************************************************
 * Name: gap_event_cb
 ****************************************************************************/

static int gap_event_cb(FAR struct ble_gap_event *event, FAR void *arg)
{
  switch (event->type)
    {
      case BLE_GAP_EVENT_CONNECT:
        {
          if (event->connect.status)
            {
              start_advertise();
            }
          break;
        }

      case BLE_GAP_EVENT_DISCONNECT:
        {
          start_advertise();
          break;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: app_ble_sync_cb
 ****************************************************************************/

static void app_ble_sync_cb(void)
{
  ble_addr_t addr;
  int        rc;

  /* Generate new non-resolvable private address */

  rc = ble_hs_id_gen_rnd(1, &addr);
  assert(rc == 0);

  /* Set generated address */

  rc = ble_hs_id_set_rnd(addr.val);
  assert(rc == 0);

  rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
  assert(rc == 0);

  start_advertise();
}

/****************************************************************************
 * Name: nimble_host_task
 ****************************************************************************/

static void nimble_host_task(FAR void *param)
{
  ble_hs_cfg.sync_cb = app_ble_sync_cb;
  ble_svc_gap_device_name_set(g_gap_name);
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
  uint8_t             batt_level = 0;
  int                 ret        = 0;

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

  /* Initialize services */

  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_svc_ans_init();
  ble_svc_ias_init();
  ble_svc_lls_init();
  ble_svc_tps_init();
  ble_svc_bas_init();
  ble_svc_dis_init();

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

      sleep(1);
    }

  return 0;
}
