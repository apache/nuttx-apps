/****************************************************************************
 * apps/examples/ble/ble.c
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

#include <nuttx/wireless/bluetooth/bt_core.h>
#include <nuttx/wireless/bluetooth/bt_gatt.h>
#include <nuttx/wireless/bluetooth/bt_ioctl.h>
#include <netpacket/bluetooth.h>
#include "sensors.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GASVC                     0x0001
#define NAME_CHR                  (GASVC + 0x0002)
#define NAME_DSC                  (GASVC + 0x0003)
#define APPEARANCE_CHR            (GASVC + 0x0004)
#define APPEARANCE_DSC            (GASVC + 0x0005)
#define SENSOR_SVC                (APPEARANCE_DSC + 0x0001)
#define TEMP_CHR                  (SENSOR_SVC + 0x0001)
#define TEMP_DSC                  (SENSOR_SVC + 0x0002)
#define HUM_CHR                   (TEMP_DSC + 0x0001)
#define HUM_DSC                   (TEMP_DSC + 0x0002)
#define GAS_CHR                   (HUM_DSC + 0x0001)
#define GAS_DSC                   (HUM_DSC + 0x0002)
#define PRESS_CHR                 (GAS_DSC + 0x0001)
#define PRESS_DSC                 (GAS_DSC + 0x0002)

/* Bluetooth UUIDs */

static struct bt_uuid_s g_gap_uuid =
{
  .type  = BT_UUID_16,
  .u.u16 = BT_UUID_GAP,
};

static struct bt_uuid_s g_device_name_uuid =
{
  .type  = BT_UUID_16,
  .u.u16 = BT_UUID_GAP_DEVICE_NAME,
};

static struct bt_uuid_s g_appearance_uuid =
{
  .type  = BT_UUID_16,
  .u.u16 = BT_UUID_GAP_APPEARANCE,
};

static struct bt_uuid_s g_sensor_uuid =
{
    .type = BT_UUID_128,
    .u.u128 =
    {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
    }
};

static struct bt_uuid_s g_temp_uuid =
{
  .type = BT_UUID_16,
  .u.u16 = 0x2a6e, /* Temperature UUID */
};

static struct bt_uuid_s g_hum_uuid =
{
  .type = BT_UUID_16,
  .u.u16 = 0x2a6f, /* Humidity UUID */
};

static struct bt_uuid_s g_press_uuid =
{
  .type = BT_UUID_16,
  .u.u16 = 0x2a6d, /* Pressure UUID */
};

/* GATT characteristics */

static struct bt_gatt_chrc_s g_name_chrc =
{
  .properties = BT_GATT_CHRC_READ,
  .value_handle = NAME_DSC,
  .uuid = &g_device_name_uuid,
};

static struct bt_gatt_chrc_s g_appearance_chrc =
{
  .properties = BT_GATT_CHRC_READ,
  .value_handle = APPEARANCE_DSC,
  .uuid = &g_appearance_uuid,
};

static struct bt_gatt_chrc_s g_temp_chrc =
{
  .properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
  .value_handle = TEMP_DSC,
  .uuid = &g_temp_uuid,
};

static struct bt_gatt_chrc_s g_hum_chrc =
{
  .properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
  .value_handle = HUM_DSC,
  .uuid = &g_hum_uuid,
};

static struct bt_gatt_chrc_s g_press_chrc =
{
  .properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
  .value_handle = PRESS_DSC,
  .uuid = &g_press_uuid,
};

static int read_name(FAR struct bt_conn_s *conn,
                     FAR const struct bt_gatt_attr_s *attr,
                     FAR void *buf, uint8_t len, uint16_t offset)
{
  const char *name = CONFIG_DEVICE_NAME;
  return bt_gatt_attr_read(conn, attr, buf, len, offset, name, strlen(name));
}

static int read_appearance(FAR struct bt_conn_s *conn,
                           FAR const struct bt_gatt_attr_s *attr,
                           FAR void *buf, uint8_t len, uint16_t offset)
{
  uint16_t appearance = 0; /* Set appropriate appearance value */
  return bt_gatt_attr_read(conn, attr, buf, len, offset,
    &appearance, sizeof(appearance));
}

static int read_temperature(FAR struct bt_conn_s *conn,
                            FAR const struct bt_gatt_attr_s *attr,
                            FAR void *buf, uint8_t len, uint16_t offset)
{
  uint16_t temp = (uint16_t) (get_temperature() * 100);
  return bt_gatt_attr_read(conn, attr, buf, len, offset,
    &temp, sizeof(uint16_t));
}

static int read_humidity(FAR struct bt_conn_s *conn,
                         FAR const struct bt_gatt_attr_s *attr,
                         FAR void *buf, uint8_t len, uint16_t offset)
{
  uint16_t hum = (uint16_t) (get_humidity() * 100);
  return bt_gatt_attr_read(conn, attr, buf, len, offset,
    &hum, sizeof(uint16_t));
}

static int read_gas(FAR struct bt_conn_s *conn,
                    FAR const struct bt_gatt_attr_s *attr,
                    FAR void *buf, uint8_t len, uint16_t offset)
{
  uint16_t gas = (uint16_t) (get_gas_resistance() * 100);
  return bt_gatt_attr_read(conn, attr, buf, len, offset,
    &gas, sizeof(uint16_t));
}

static int read_pressure(FAR struct bt_conn_s *conn,
                         FAR const struct bt_gatt_attr_s *attr,
                         FAR void *buf, uint8_t len, uint16_t offset)
{
  uint32_t press = (uint32_t) (get_pressure() * 1000);
  return bt_gatt_attr_read(conn, attr, buf, len, offset,
    &press, sizeof(uint32_t));
}

/* GATT attributes */

static const struct bt_gatt_attr_s attrs[] =
{
  BT_GATT_PRIMARY_SERVICE(GASVC, &g_gap_uuid),
  BT_GATT_CHARACTERISTIC(NAME_CHR, &g_name_chrc),
  BT_GATT_DESCRIPTOR(NAME_DSC, &g_device_name_uuid, BT_GATT_PERM_READ,
                     read_name, NULL, NULL),
  BT_GATT_CHARACTERISTIC(APPEARANCE_CHR, &g_appearance_chrc),
  BT_GATT_DESCRIPTOR(APPEARANCE_DSC, &g_appearance_uuid,
                     BT_GATT_PERM_READ, read_appearance, NULL, NULL),

  BT_GATT_PRIMARY_SERVICE(SENSOR_SVC, &g_sensor_uuid),
  BT_GATT_CHARACTERISTIC(TEMP_CHR, &g_temp_chrc),
  BT_GATT_DESCRIPTOR(TEMP_DSC, &g_temp_uuid, BT_GATT_PERM_READ,
    read_temperature, NULL, NULL),
  BT_GATT_CHARACTERISTIC(HUM_CHR, &g_hum_chrc),
  BT_GATT_DESCRIPTOR(HUM_DSC, &g_hum_uuid, BT_GATT_PERM_READ,
    read_humidity, NULL, NULL),
  BT_GATT_CHARACTERISTIC(PRESS_CHR, &g_press_chrc),
  BT_GATT_DESCRIPTOR(PRESS_DSC, &g_press_uuid, BT_GATT_PERM_READ,
    read_pressure, NULL, NULL),
};

static void start_scanning(void)
{
  struct btreq_s btreq;
  memset(&btreq, 0, sizeof(struct btreq_s));
  strlcpy(btreq.btr_name, "bnep0", 16);
  btreq.btr_dupenable = false;

  int sockfd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP);
  if (sockfd < 0)
  {
    fprintf(stderr, "ERROR: failed to create socket\n");
    return;
  }
  int ret = ioctl(sockfd, SIOCBTSCANSTART,
                (unsigned long)((uintptr_t)&btreq));
  if (ret < 0)
  {
    fprintf(stderr, "ERROR: ioctl(SIOCBTSCANSTART) failed\n");
  }
  close(sockfd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * setup_ble
 ****************************************************************************/

void setup_ble(void)
{
  /* Scanning is enabled here to ensure advertising packets are sent. */

  start_scanning();
  bt_gatt_register(attrs, nitems(attrs));
}
