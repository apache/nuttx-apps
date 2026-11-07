/****************************************************************************
 * apps/examples/nimble_bleess/bleess_gatt.c
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

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gatt/ble_svc_gatt.h"

#include "bleess.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int bleess_gatt_access(uint16_t connection_handle,
                              uint16_t attribute_handle,
                              FAR struct ble_gatt_access_ctxt *context,
                              FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const ble_uuid16_t g_bleess_service_uuid =
  BLE_UUID16_INIT(BLEESS_SERVICE_UUID);
static const ble_uuid16_t g_bleess_temperature_uuid =
  BLE_UUID16_INIT(BLEESS_TEMPERATURE_UUID);
static const ble_uuid16_t g_bleess_humidity_uuid =
  BLE_UUID16_INIT(BLEESS_HUMIDITY_UUID);

static int16_t g_bleess_temperature_centi = 2400;
static uint16_t g_bleess_humidity_centi = 5000;
static uint16_t g_bleess_temperature_handle;
static uint16_t g_bleess_humidity_handle;

static const struct ble_gatt_svc_def g_bleess_services[] =
{
  {
    /* Service: Environmental Sensing. */

    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &g_bleess_service_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[])
    {
      {
        /* Characteristic: Temperature. */

        .uuid = &g_bleess_temperature_uuid.u,
        .access_cb = bleess_gatt_access,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      },

      {
        /* Characteristic: Humidity. */

        .uuid = &g_bleess_humidity_uuid.u,
        .access_cb = bleess_gatt_access,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      },

      {
        0, /* No more characteristics in this service. */
      }
    },
  },

  {
    0, /* No more services. */
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int bleess_gatt_access(uint16_t connection_handle,
                              uint16_t attribute_handle,
                              FAR struct ble_gatt_access_ctxt *context,
                              FAR void *arg)
{
  FAR const ble_uuid_t *uuid = context->chr->uuid;
  uint16_t encoded_temperature;
  uint8_t value[2];

  UNUSED(connection_handle);
  UNUSED(attribute_handle);
  UNUSED(arg);

  if (context->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
      return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    }

  if (ble_uuid_cmp(uuid, &g_bleess_temperature_uuid.u) == 0)
    {
      encoded_temperature = (uint16_t)g_bleess_temperature_centi;
      value[0] = (uint8_t)encoded_temperature;
      value[1] = (uint8_t)(encoded_temperature >> 8);
    }
  else if (ble_uuid_cmp(uuid, &g_bleess_humidity_uuid.u) == 0)
    {
      value[0] = (uint8_t)g_bleess_humidity_centi;
      value[1] = (uint8_t)(g_bleess_humidity_centi >> 8);
    }
  else
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

  if (os_mbuf_append(context->om, value, sizeof(value)) != 0)
    {
      return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bleess_gatt_register_cb(FAR struct ble_gatt_register_ctxt *context,
                             FAR void *arg)
{
  char uuid_string[BLE_UUID_STR_LEN];

  UNUSED(arg);

  if (context->op == BLE_GATT_REGISTER_OP_SVC)
    {
      printf("GATT service: uuid=%s handle=%u\n",
             ble_uuid_to_str(context->svc.svc_def->uuid, uuid_string),
             context->svc.handle);
    }
  else if (context->op == BLE_GATT_REGISTER_OP_CHR)
    {
      printf("GATT characteristic: uuid=%s value_handle=%u\n",
             ble_uuid_to_str(context->chr.chr_def->uuid, uuid_string),
             context->chr.val_handle);

      if (ble_uuid_cmp(context->chr.chr_def->uuid,
                       &g_bleess_temperature_uuid.u) == 0)
        {
          g_bleess_temperature_handle = context->chr.val_handle;
        }
      else if (ble_uuid_cmp(context->chr.chr_def->uuid,
                            &g_bleess_humidity_uuid.u) == 0)
        {
          g_bleess_humidity_handle = context->chr.val_handle;
        }
    }
}

int bleess_gatt_init(void)
{
  int ret;

  ble_svc_gatt_init();

  ret = ble_gatts_count_cfg(g_bleess_services);
  if (ret != 0)
    {
      return ret;
    }

  return ble_gatts_add_svcs(g_bleess_services);
}

void bleess_gatt_update(int16_t temperature_centi,
                        uint16_t humidity_centi)
{
  g_bleess_temperature_centi = temperature_centi;
  g_bleess_humidity_centi = humidity_centi;

  if (g_bleess_temperature_handle != 0)
    {
      ble_gatts_chr_updated(g_bleess_temperature_handle);
    }

  if (g_bleess_humidity_handle != 0)
    {
      ble_gatts_chr_updated(g_bleess_humidity_handle);
    }
}
