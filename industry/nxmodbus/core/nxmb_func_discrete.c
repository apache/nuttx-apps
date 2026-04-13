/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func_discrete.c
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

#define NXMB_FC02_ADDR_OFF     0
#define NXMB_FC02_QTY_OFF      2
#define NXMB_FC02_REQ_DATA_LEN 4

#define NXMB_DISCRETE_QTY_MAX  2000

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_fc02_read_discrete
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc02_read_discrete(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t addr;
  uint16_t qty;
  uint8_t  nbytes;
  int      ret;

  if (data_len != NXMB_FC02_REQ_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if (ctx->callbacks == NULL || ctx->callbacks->discrete_cb == NULL)
    {
      return NXMB_EX_ILLEGAL_FUNCTION;
    }

  addr = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC02_ADDR_OFF]);
  qty  = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_FC02_QTY_OFF]);

  if (qty < 1 || qty > NXMB_DISCRETE_QTY_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  if ((uint32_t)addr + qty > 0xffffu)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  nbytes = (uint8_t)((qty + 7) / 8);

  /* Response is byte_count + discrete data */

  if (1 + nbytes > NXMB_ADU_DATA_MAX)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  ctx->adu.fc      = NXMB_FC_READ_DISCRETE;
  ctx->adu.data[0] = nbytes;

  memset(&ctx->adu.data[1], 0, nbytes);

  ret = ctx->callbacks->discrete_cb(&ctx->adu.data[1], addr, qty,
                                    ctx->callbacks->priv);
  if (ret != 0)
    {
      return nxmb_cb_ret_to_exception(ret);
    }

  ctx->adu.length = (uint16_t)(2 + 1 + nbytes);
  return NXMB_EX_NONE;
}
