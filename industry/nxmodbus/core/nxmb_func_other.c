/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func_other.c
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

#define NXMB_REPORT_ID_REQ_DATA_LEN 0

/* Run indicator status */

#define NXMB_RUN_STATUS_ON          0xff
#define NXMB_RUN_STATUS_OFF         0x00

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_fc17_report_server_id
 *
 * Description:
 *   Handle FC17 (0x11) Report Server ID request. Returns the configured
 *   unit ID and a run indicator byte (0xFF = ON).
 *
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc17_report_server_id(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;

  if (data_len != NXMB_REPORT_ID_REQ_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  /* If the application configured server ID data via nxmb_set_server_id(),
   * use that; otherwise fall back to unit_id + run status ON.
   */

  ctx->adu.fc = NXMB_FC_REPORT_SERVER_ID;

  if (ctx->server_id_len > 0)
    {
      /* Response is byte_count + server_id_buf */

      if (1 + ctx->server_id_len > NXMB_ADU_DATA_MAX)
        {
          return NXMB_EX_DEVICE_FAILURE;
        }

      ctx->adu.data[0] = (uint8_t)ctx->server_id_len;
      memcpy(&ctx->adu.data[1], ctx->server_id_buf, ctx->server_id_len);
      ctx->adu.length = (uint16_t)(2 + 1 + ctx->server_id_len);
    }
  else
    {
      ctx->adu.data[0] = 2;                  /* byte count */
      ctx->adu.data[1] = ctx->unit_id;       /* server ID */
      ctx->adu.data[2] = NXMB_RUN_STATUS_ON; /* run indicator */
      ctx->adu.length  = (uint16_t)(2 + 3);
    }

  return NXMB_EX_NONE;
}
