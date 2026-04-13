/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func_coils.c
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

#include <nuttx/compiler.h>

#include <stdint.h>
#include <string.h>

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Offsets are in adu.data[]; lengths refer to data only. */

/* FC01 Read Coils */

#define NXMB_FC01_ADDR_OFF      0
#define NXMB_FC01_QTY_OFF       2
#define NXMB_FC01_REQ_DATA_LEN  4

/* FC05 Write Single Coil */

#define NXMB_FC05_ADDR_OFF      0
#define NXMB_FC05_VALUE_OFF     2
#define NXMB_FC05_REQ_DATA_LEN  4

/* FC15 Write Multiple Coils */

#define NXMB_FC15_ADDR_OFF      0
#define NXMB_FC15_QTY_OFF       2
#define NXMB_FC15_BCNT_OFF      4
#define NXMB_FC15_DATA_OFF      5
#define NXMB_FC15_RESP_DATA_LEN 4

/* Maximum coil quantity per Modbus specification.
 * FC01/FC02 read max is 2000 (0x07D0).
 * FC15 write max is 1968 (0x07B0).
 */

#define NXMB_COIL_READ_QTY_MAX  2000
#define NXMB_COIL_WRITE_QTY_MAX 1968

/* Coil ON/OFF wire values for FC05 */

#define NXMB_COIL_ON            0xff00
#define NXMB_COIL_OFF           0x0000

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_fc01_read_coils
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc01_read_coils(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  uint16_t qty;
  uint8_t  nbytes;
  int      ret;

  if (data_len != NXMB_FC01_REQ_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->coil_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC01_ADDR_OFF]);
  qty  = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC01_QTY_OFF]);

  if (qty < 1 || qty > NXMB_COIL_READ_QTY_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if ((uint32_t)addr + qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  nbytes = (uint8_t)((qty + 7) / 8);

  /* Response is byte_count + coil data */

  if (1 + nbytes > NXMB_ADU_DATA_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* Build response: byte_count + coil data */

  ctx->adu.fc      = NXMB_FC_READ_COILS;
  ctx->adu.data[0] = nbytes;

  /* Zero data area so unused trailing bits are clear */

  memset(&ctx->adu.data[1], 0, nbytes);

  ret = ctx->callbacks->coil_cb(&ctx->adu.data[1], addr, qty, NXMB_REG_READ,
                                ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  ctx->adu.length = (uint16_t)(2 + 1 + nbytes);
  return NXMB_EX_NONE;
}

/****************************************************************************
 * Name: nxmb_fc05_write_coil
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc05_write_coil(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  uint16_t value;
  uint8_t  coil_buf[2];
  int      ret;

  if (data_len != NXMB_FC05_REQ_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->coil_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr  = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC05_ADDR_OFF]);
  value = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC05_VALUE_OFF]);

  if (addr == 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  if (value != NXMB_COIL_ON && value != NXMB_COIL_OFF)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* Convert wire value to single-bit buffer for callback */

  coil_buf[0] = (value == NXMB_COIL_ON) ? 1 : 0;
  coil_buf[1] = 0;

  ret = ctx->callbacks->coil_cb(coil_buf, addr, 1, NXMB_REG_WRITE,
                                ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  /* Response echoes the request — adu.length is already correct */

  return NXMB_EX_NONE;
}

/****************************************************************************
 * Name: nxmb_fc15_write_coils
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc15_write_coils(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  uint16_t qty;
  uint8_t  byte_count;
  uint8_t  byte_count_exp;
  int      ret;

  if (data_len < NXMB_FC15_DATA_OFF)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->coil_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr       = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC15_ADDR_OFF]);
  qty        = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC15_QTY_OFF]);
  byte_count = ctx->adu.data[NXMB_FC15_BCNT_OFF];

  if (qty < 1 || qty > NXMB_COIL_WRITE_QTY_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if ((uint32_t)addr + qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  byte_count_exp = (uint8_t)((qty + 7) / 8);

  if (byte_count != byte_count_exp)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* Verify total data length matches header + payload */

  if (data_len != (uint16_t)(NXMB_FC15_DATA_OFF + byte_count))
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  ret = ctx->callbacks->coil_cb(&ctx->adu.data[NXMB_FC15_DATA_OFF], addr,
                                qty, NXMB_REG_WRITE, ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  /* Response: addr(2) + qty(2) — already in data[0..3] from request */

  ctx->adu.length = (uint16_t)(2 + NXMB_FC15_RESP_DATA_LEN);
  return NXMB_EX_NONE;
}
