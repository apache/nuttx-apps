/****************************************************************************
 * apps/examples/nimble_mesh_vendor/nimble_mesh_vendor_main.c
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

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "host/ble_hs.h"
#include "netutils/netinit.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "mesh/glue.h"
#include "mesh/access.h"
#include "mesh/cfg_srv.h"
#include "mesh/health_srv.h"
#include "mesh/main.h"
#include "mesh/msg.h"
#include "mesh/porting.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define COMPANY_ID      0x05f1
#define VENDOR_MODEL_ID 0x0001

/* Vendor opcodes: Get and Set carry a one byte value in Status */

#define VENDOR_GET    BT_MESH_MODEL_OP_3(0x01, COMPANY_ID)
#define VENDOR_STATUS BT_MESH_MODEL_OP_3(0x02, COMPANY_ID)
#define VENDOR_SET    BT_MESH_MODEL_OP_3(0x03, COMPANY_ID)

/****************************************************************************
 * External Functions Prototypes
 ****************************************************************************/

void ble_hci_sock_ack_handler(FAR void *param);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int vendor_get(FAR struct bt_mesh_model *model,
                      FAR struct bt_mesh_msg_ctx *ctx,
                      FAR struct os_mbuf *buf);
static int vendor_set(FAR struct bt_mesh_model *model,
                      FAR struct bt_mesh_msg_ctx *ctx,
                      FAR struct os_mbuf *buf);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_dev_uuid[16] =
{
  0xdd, 0xaa, 0x00, 0x03, 0x4d, 0x45, 0x53, 0x48,
  0x56, 0x4e, 0x44, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const struct bt_mesh_model_op g_vendor_ops[] =
{
  { VENDOR_GET, BT_MESH_LEN_EXACT(0), vendor_get },
  { VENDOR_SET, BT_MESH_LEN_EXACT(1), vendor_set },
  BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_health_srv g_health_srv;
static struct bt_mesh_model_pub g_health_pub;
static uint8_t g_vendor_value;

static struct bt_mesh_model g_root_models[] =
{
  BT_MESH_MODEL_CFG_SRV,
  BT_MESH_MODEL_HEALTH_SRV(&g_health_srv, &g_health_pub),
};

static struct bt_mesh_model g_vendor_models[] =
{
  BT_MESH_MODEL_VND(COMPANY_ID, VENDOR_MODEL_ID, g_vendor_ops, NULL, NULL),
};

static struct bt_mesh_elem g_elements[] =
{
  {
    .loc = 0x0000,
    .model_count = sizeof(g_root_models) / sizeof(g_root_models[0]),
    .models = g_root_models,
    .vnd_model_count = sizeof(g_vendor_models) / sizeof(g_vendor_models[0]),
    .vnd_models = g_vendor_models,
  },
};

static const struct bt_mesh_comp g_comp =
{
  .cid = COMPANY_ID,
  .elem_count = sizeof(g_elements) / sizeof(g_elements[0]),
  .elem = g_elements,
};

static const struct bt_mesh_prov g_prov =
{
  .uuid = g_dev_uuid,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vendor_status
 *
 * Description:
 *   Send a vendor Status with the stored value.
 *
 ****************************************************************************/

static int vendor_status(FAR struct bt_mesh_model *model,
                         FAR struct bt_mesh_msg_ctx *ctx)
{
  FAR struct os_mbuf *msg;
  int ret;

  msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(VENDOR_STATUS, 1));
  if (msg == NULL)
    {
      return -ENOMEM;
    }

  bt_mesh_model_msg_init(msg, VENDOR_STATUS);
  net_buf_simple_add_u8(msg, g_vendor_value);

  ret = bt_mesh_model_send(model, ctx, msg, NULL, NULL);
  if (ret != 0)
    {
      printf("nimble_mesh_vendor: status send failed: %d\n", ret);
    }

  os_mbuf_free_chain(msg);
  return ret;
}

/****************************************************************************
 * Name: vendor_get
 ****************************************************************************/

static int vendor_get(FAR struct bt_mesh_model *model,
                      FAR struct bt_mesh_msg_ctx *ctx,
                      FAR struct os_mbuf *buf)
{
  UNUSED(buf);

  printf("nimble_mesh_vendor: get from 0x%04x\n", ctx->addr);
  return vendor_status(model, ctx);
}

/****************************************************************************
 * Name: vendor_set
 ****************************************************************************/

static int vendor_set(FAR struct bt_mesh_model *model,
                      FAR struct bt_mesh_msg_ctx *ctx,
                      FAR struct os_mbuf *buf)
{
  g_vendor_value = net_buf_simple_pull_u8(buf);
  printf("nimble_mesh_vendor: set %u from 0x%04x\n",
         g_vendor_value, ctx->addr);
  return vendor_status(model, ctx);
}

/****************************************************************************
 * Name: mesh_hci_task
 ****************************************************************************/

static FAR void *mesh_hci_task(FAR void *arg)
{
  ble_hci_sock_ack_handler(arg);
  return NULL;
}

/****************************************************************************
 * Name: mesh_adv_task
 ****************************************************************************/

static FAR void *mesh_adv_task(FAR void *arg)
{
  mesh_adv_thread(arg);
  return NULL;
}

/****************************************************************************
 * Name: mesh_on_sync
 *
 * Description:
 *   Host is synced with the controller: set a random address, initialize
 *   the mesh stack, start the mesh advertising thread and enable
 *   provisioning.
 *
 ****************************************************************************/

static void mesh_on_sync(void)
{
  pthread_attr_t attr;
  pthread_t thread;
  ble_addr_t addr;
  int ret;

  ret = ble_hs_id_gen_rnd(1, &addr);
  if (ret == 0)
    {
      ret = ble_hs_id_set_rnd(addr.val);
    }

  if (ret != 0)
    {
      printf("nimble_mesh_vendor: address setup failed: %d\n", ret);
      return;
    }

  ret = bt_mesh_init(addr.type, &g_prov, &g_comp);
  if (ret < 0)
    {
      printf("nimble_mesh_vendor: bt_mesh_init failed: %d\n", ret);
      return;
    }

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr,
                            CONFIG_EXAMPLES_NIMBLE_MESH_VENDOR_STACKSIZE);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  ret = pthread_create(&thread, &attr, mesh_adv_task, NULL);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      printf("nimble_mesh_vendor: advertising thread failed: %d\n", ret);
      return;
    }

  ret = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
  if (ret < 0)
    {
      printf("nimble_mesh_vendor: bt_mesh_prov_enable failed: %d\n", ret);
      return;
    }

  printf("nimble_mesh_vendor: provisioning enabled\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ble_npl_task hci_task;
  int ret;

  UNUSED(argc);
  UNUSED(argv);

#ifndef CONFIG_NSH_NETINIT
  /* Bring up the network */

  netinit_bringup();
#endif

  nimble_port_init();

  /* Initialize services */

  ble_svc_gap_init();
  ble_svc_gatt_init();
  bt_mesh_register_gatt();
  g_health_pub.msg = BT_MESH_HEALTH_FAULT_MSG(0);

  ble_hs_cfg.sync_cb = mesh_on_sync;

  /* Create task which handles HCI socket */

  ret = ble_npl_task_init(&hci_task, "hci_sock", mesh_hci_task, NULL,
                          CONFIG_EXAMPLES_NIMBLE_MESH_VENDOR_PRIORITY,
                          BLE_NPL_TIME_FOREVER, NULL, 0);
  if (ret != 0)
    {
      printf("nimble_mesh_vendor: hci task failed: %d\n", ret);
      return ret;
    }

  /* Run the host event queue on this task */

  nimble_port_run();
  return 0;
}
