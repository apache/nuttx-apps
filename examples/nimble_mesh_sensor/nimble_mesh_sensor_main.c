/****************************************************************************
 * apps/examples/nimble_mesh_sensor/nimble_mesh_sensor_main.c
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

#define COMPANY_ID 0x05f1

/* Sensor Server opcodes */

#define SENSOR_DESC_GET    BT_MESH_MODEL_OP_2(0x82, 0x30)
#define SENSOR_GET         BT_MESH_MODEL_OP_2(0x82, 0x31)
#define SENSOR_DESC_STATUS BT_MESH_MODEL_OP_1(0x51)
#define SENSOR_STATUS      BT_MESH_MODEL_OP_1(0x52)

/* Present Ambient Temperature, Temperature 8 characteristic (0.5 C) */

#define SENSOR_PROPERTY    0x004f
#define SENSOR_VALUE_LEN   1
#define SENSOR_DESC_LEN    8

/* Marshalled Sensor Data, Format A header: property in bits 5-15,
 * length minus one in bits 1-4, format bit clear.
 */

#define SENSOR_HDR_A(prop, len) (((prop) << 5) | (((len) - 1) << 1))

/* Format B header for an unknown property: zero length */

#define SENSOR_HDR_B_EMPTY 0xff

/****************************************************************************
 * External Functions Prototypes
 ****************************************************************************/

void ble_hci_sock_ack_handler(FAR void *param);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int sensor_desc_get(FAR struct bt_mesh_model *model,
                           FAR struct bt_mesh_msg_ctx *ctx,
                           FAR struct os_mbuf *buf);
static int sensor_get(FAR struct bt_mesh_model *model,
                      FAR struct bt_mesh_msg_ctx *ctx,
                      FAR struct os_mbuf *buf);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_dev_uuid[16] =
{
  0xdd, 0xaa, 0x00, 0x02, 0x4d, 0x45, 0x53, 0x48,
  0x53, 0x45, 0x4e, 0x53, 0x00, 0x00, 0x00, 0x01
};

static const struct bt_mesh_model_op g_sensor_ops[] =
{
  { SENSOR_DESC_GET, BT_MESH_LEN_MIN(0), sensor_desc_get },
  { SENSOR_GET, BT_MESH_LEN_MIN(0), sensor_get },
  BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_health_srv g_health_srv;
static struct bt_mesh_model_pub g_health_pub;

/* Simulated temperature in 0.5 C steps, incremented on every read */

static int8_t g_temperature = 44;

static struct bt_mesh_model g_root_models[] =
{
  BT_MESH_MODEL_CFG_SRV,
  BT_MESH_MODEL_HEALTH_SRV(&g_health_srv, &g_health_pub),
  BT_MESH_MODEL(BT_MESH_MODEL_ID_SENSOR_SRV, g_sensor_ops, NULL, NULL),
};

static struct bt_mesh_elem g_elements[] =
{
  {
    .loc = 0x0000,
    .model_count = sizeof(g_root_models) / sizeof(g_root_models[0]),
    .models = g_root_models,
    .vnd_model_count = 0,
    .vnd_models = BT_MESH_MODEL_NONE,
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
 * Name: sensor_property
 *
 * Description:
 *   Return the optional Property ID from a Get message, or -1 for all.
 *
 ****************************************************************************/

static int sensor_property(FAR struct os_mbuf *buf)
{
  if (buf->om_len < 2)
    {
      return -1;
    }

  return net_buf_simple_pull_le16(buf);
}

/****************************************************************************
 * Name: sensor_desc_get
 *
 * Description:
 *   Reply with the Sensor Descriptor Status.  Tolerances, sampling
 *   function, measurement period and update interval are unspecified.
 *
 ****************************************************************************/

static int sensor_desc_get(FAR struct bt_mesh_model *model,
                           FAR struct bt_mesh_msg_ctx *ctx,
                           FAR struct os_mbuf *buf)
{
  FAR struct os_mbuf *msg;
  int property;
  int ret;

  property = sensor_property(buf);
  printf("nimble_mesh_sensor: descriptor get %d from 0x%04x\n",
         property, ctx->addr);

  msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(SENSOR_DESC_STATUS,
                                             SENSOR_DESC_LEN));
  if (msg == NULL)
    {
      return -ENOMEM;
    }

  bt_mesh_model_msg_init(msg, SENSOR_DESC_STATUS);
  if (property < 0 || property == SENSOR_PROPERTY)
    {
      net_buf_simple_add_le16(msg, SENSOR_PROPERTY);
      net_buf_simple_add_le24(msg, 0);
      net_buf_simple_add_u8(msg, 0);
      net_buf_simple_add_u8(msg, 0);
      net_buf_simple_add_u8(msg, 0);
    }
  else
    {
      /* Unknown property: the Property ID alone */

      net_buf_simple_add_le16(msg, property);
    }

  ret = bt_mesh_model_send(model, ctx, msg, NULL, NULL);
  if (ret != 0)
    {
      printf("nimble_mesh_sensor: status send failed: %d\n", ret);
    }

  os_mbuf_free_chain(msg);
  return ret;
}

/****************************************************************************
 * Name: sensor_get
 *
 * Description:
 *   Reply with the Sensor Status carrying the present temperature.
 *
 ****************************************************************************/

static int sensor_get(FAR struct bt_mesh_model *model,
                      FAR struct bt_mesh_msg_ctx *ctx,
                      FAR struct os_mbuf *buf)
{
  FAR struct os_mbuf *msg;
  int property;
  int ret;

  property = sensor_property(buf);
  printf("nimble_mesh_sensor: get %d from 0x%04x\n", property, ctx->addr);

  msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(SENSOR_STATUS,
                                             2 + SENSOR_VALUE_LEN));
  if (msg == NULL)
    {
      return -ENOMEM;
    }

  bt_mesh_model_msg_init(msg, SENSOR_STATUS);
  if (property < 0 || property == SENSOR_PROPERTY)
    {
      net_buf_simple_add_le16(msg, SENSOR_HDR_A(SENSOR_PROPERTY,
                                                SENSOR_VALUE_LEN));
      net_buf_simple_add_u8(msg, (uint8_t)g_temperature);
      g_temperature++;
    }
  else
    {
      /* Unknown property: Format B header without a value */

      net_buf_simple_add_u8(msg, SENSOR_HDR_B_EMPTY);
      net_buf_simple_add_le16(msg, property);
    }

  ret = bt_mesh_model_send(model, ctx, msg, NULL, NULL);
  if (ret != 0)
    {
      printf("nimble_mesh_sensor: status send failed: %d\n", ret);
    }

  os_mbuf_free_chain(msg);
  return ret;
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
      printf("nimble_mesh_sensor: address setup failed: %d\n", ret);
      return;
    }

  ret = bt_mesh_init(addr.type, &g_prov, &g_comp);
  if (ret < 0)
    {
      printf("nimble_mesh_sensor: bt_mesh_init failed: %d\n", ret);
      return;
    }

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr,
                            CONFIG_EXAMPLES_NIMBLE_MESH_SENSOR_STACKSIZE);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  ret = pthread_create(&thread, &attr, mesh_adv_task, NULL);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      printf("nimble_mesh_sensor: advertising thread failed: %d\n", ret);
      return;
    }

  ret = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
  if (ret < 0)
    {
      printf("nimble_mesh_sensor: bt_mesh_prov_enable failed: %d\n", ret);
      return;
    }

  printf("nimble_mesh_sensor: provisioning enabled\n");
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
                          CONFIG_EXAMPLES_NIMBLE_MESH_SENSOR_PRIORITY,
                          BLE_NPL_TIME_FOREVER, NULL, 0);
  if (ret != 0)
    {
      printf("nimble_mesh_sensor: hci task failed: %d\n", ret);
      return ret;
    }

  /* Run the host event queue on this task */

  nimble_port_run();
  return 0;
}
