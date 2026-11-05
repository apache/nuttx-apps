/****************************************************************************
 * apps/wireless/bluetooth/zblue/examples/peripheral.c
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

/* Connectable advertising with the GAP and GATT services, written against
 * the Zephyr Bluetooth API exactly as a Zephyr application would be.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct bt_data g_ad[] =
{
  BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
  BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
          sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static K_SEM_DEFINE(g_disconnected, 0, 1);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void connected(struct bt_conn *conn, uint8_t err)
{
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  if (err != 0)
    {
      printf("zblue: connection to %s failed (0x%02x)\n", addr, err);
      return;
    }

  printf("zblue: connected %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  printf("zblue: disconnected %s (0x%02x)\n", addr, reason);
  k_sem_give(&g_disconnected);
}

BT_CONN_CB_DEFINE(g_conn_cb) =
{
  .connected    = connected,
  .disconnected = disconnected,
};

static int advertise(void)
{
  int err;

  err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, g_ad, ARRAY_SIZE(g_ad),
                        NULL, 0);
  if (err != 0)
    {
      printf("zblue: advertising failed: %d\n", err);
      return err;
    }

  printf("zblue: advertising as \"%s\"\n", CONFIG_BT_DEVICE_NAME);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  bt_addr_le_t addr;
  size_t count = 1;
  char str[BT_ADDR_LE_STR_LEN];
  int err;

  err = bt_enable(NULL);
  if (err != 0)
    {
      printf("zblue: bt_enable failed: %d\n", err);
      return 1;
    }

  bt_id_get(&addr, &count);
  bt_addr_le_to_str(&addr, str, sizeof(str));
  printf("zblue: Bluetooth ready, identity %s\n", str);

  for (; ; )
    {
      if (advertise() != 0)
        {
          return 1;
        }

      k_sem_take(&g_disconnected, K_FOREVER);
    }

  return 0;
}
