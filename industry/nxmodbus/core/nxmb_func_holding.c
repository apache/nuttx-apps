/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func_holding.c
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

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* PDU data offsets are relative to ctx->adu.data[] (after unit_id + fc).
 * Lengths refer to the data payload only — adu.length = 2 + data_len.
 */

/* FC03 Read Holding Registers */

#define NXMB_FC03_ADDR_OFF            0
#define NXMB_FC03_QTY_OFF             2
#define NXMB_FC03_REQ_DATA_LEN        4

/* FC06 Write Single Holding Register */

#define NXMB_FC06_ADDR_OFF            0
#define NXMB_FC06_VALUE_OFF           2
#define NXMB_FC06_REQ_DATA_LEN        4

/* FC16 Write Multiple Holding Registers */

#define NXMB_FC16_ADDR_OFF            0
#define NXMB_FC16_QTY_OFF             2
#define NXMB_FC16_BCNT_OFF            4
#define NXMB_FC16_DATA_OFF            5
#define NXMB_FC16_RESP_DATA_LEN       4

/* FC23 Read/Write Multiple Holding Registers */

#define NXMB_FC23_RD_ADDR_OFF         0
#define NXMB_FC23_RD_QTY_OFF          2
#define NXMB_FC23_WR_ADDR_OFF         4
#define NXMB_FC23_WR_QTY_OFF          6
#define NXMB_FC23_WR_BCNT_OFF         8
#define NXMB_FC23_WR_DATA_OFF         9
#define NXMB_FC23_REQ_MIN_DATA_LEN    9

/* Quantity limits per Modbus Application Protocol Specification */

#define NXMB_REG_READ_QTY_MAX         125 /* 0x007D */
#define NXMB_REG_WRITE_MUL_QTY_MAX    123 /* 0x007B */
#define NXMB_REG_READWRITE_RD_QTY_MAX 125 /* 0x007D */
#define NXMB_REG_READWRITE_WR_QTY_MAX 121 /* 0x0079 */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_fc03_read_holding
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc03_read_holding(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  uint16_t qty;
  uint8_t  nbytes;
  int      ret;

  if (data_len != NXMB_FC03_REQ_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->holding_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC03_ADDR_OFF]);
  qty  = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC03_QTY_OFF]);

  if (qty < 1 || qty > NXMB_REG_READ_QTY_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if ((uint32_t)addr + qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  nbytes = (uint8_t)(qty * 2);

  /* Response is byte_count + register data */

  if (1 + nbytes > NXMB_ADU_DATA_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* Build response: byte_count + register data (big-endian) */

  ctx->adu.fc      = NXMB_FC_READ_HOLDING;
  ctx->adu.data[0] = nbytes;

  ret = ctx->callbacks->holding_cb(&ctx->adu.data[1], addr, qty,
                                   NXMB_REG_READ, ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  ctx->adu.length = (uint16_t)(2 + 1 + nbytes);
  return NXMB_EX_NONE;
}

/****************************************************************************
 * Name: nxmb_fc06_write_holding
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc06_write_holding(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  int      ret;

  if (data_len != NXMB_FC06_REQ_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->holding_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC06_ADDR_OFF]);

  if (addr == 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  ret = ctx->callbacks->holding_cb(&ctx->adu.data[NXMB_FC06_VALUE_OFF], addr,
                                   1, NXMB_REG_WRITE, ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  /* Response echoes the request — adu.length is already correct */

  return NXMB_EX_NONE;
}

/****************************************************************************
 * Name: nxmb_fc16_write_holdings
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc16_write_holdings(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  uint16_t qty;
  uint8_t  byte_count;
  int      ret;

  if (data_len < NXMB_FC16_DATA_OFF)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->holding_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr       = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC16_ADDR_OFF]);
  qty        = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC16_QTY_OFF]);
  byte_count = ctx->adu.data[NXMB_FC16_BCNT_OFF];

  if (qty < 1 || qty > NXMB_REG_WRITE_MUL_QTY_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if ((uint32_t)addr + qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  if (byte_count != (uint8_t)(qty * 2))
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* Verify total data length matches header + payload */

  if (data_len != (uint16_t)(NXMB_FC16_DATA_OFF + byte_count))
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  ret = ctx->callbacks->holding_cb(&ctx->adu.data[NXMB_FC16_DATA_OFF], addr,
                                   qty, NXMB_REG_WRITE,
                                   ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  /* Response: addr(2) + qty(2) — already in data[0..3] from request */

  ctx->adu.length = (uint16_t)(2 + NXMB_FC16_RESP_DATA_LEN);
  return NXMB_EX_NONE;
}

/****************************************************************************
 * Name: nxmb_fc23_readwrite_holding
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc23_readwrite_holding(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t rd_addr;
  uint16_t rd_qty;
  uint16_t wr_addr;
  uint16_t wr_qty;
  uint8_t  wr_bcnt;
  uint16_t rd_bcnt;
  int      ret;

  if (data_len < NXMB_FC23_REQ_MIN_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->holding_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  rd_addr = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC23_RD_ADDR_OFF]);
  rd_qty  = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC23_RD_QTY_OFF]);
  wr_addr = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC23_WR_ADDR_OFF]);
  wr_qty  = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC23_WR_QTY_OFF]);
  wr_bcnt = ctx->adu.data[NXMB_FC23_WR_BCNT_OFF];

  if (rd_qty < 1 || rd_qty > NXMB_REG_READWRITE_RD_QTY_MAX || wr_qty < 1 ||
      wr_qty > NXMB_REG_READWRITE_WR_QTY_MAX || wr_bcnt != (wr_qty * 2))
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if ((uint32_t)rd_addr + rd_qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  if ((uint32_t)wr_addr + wr_qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  /* Verify total request data length matches header + write payload */

  if (data_len != (uint16_t)(NXMB_FC23_WR_DATA_OFF + wr_bcnt))
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* Response is byte_count + read register data — must fit in adu.data[] */

  if (1 + rd_qty * 2 > NXMB_ADU_DATA_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  ret = ctx->callbacks->holding_cb(&ctx->adu.data[NXMB_FC23_WR_DATA_OFF],
                                   wr_addr, wr_qty, NXMB_REG_WRITE,
                                   ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  /* Build response: byte_count + read register data, overwriting request */

  ctx->adu.fc      = NXMB_FC_READWRITE_HOLDINGS;
  rd_bcnt          = rd_qty * 2;
  ctx->adu.data[0] = (uint8_t)rd_bcnt;

  ret = ctx->callbacks->holding_cb(&ctx->adu.data[1], rd_addr, rd_qty,
                                   NXMB_REG_READ, ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  ctx->adu.length = (uint16_t)(2 + 1 + rd_bcnt);
  return NXMB_EX_NONE;
}
