/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func_diag.c
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

#define NXMB_DIAG_SUBFUNC_OFF       0
#define NXMB_DIAG_DATA_OFF          2
#define NXMB_DIAG_REQ_MIN_DATA_LEN  4

/* Sub-function codes */

#define NXMB_DIAG_RETURN_QUERY      0x0000

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_fc08_diagnostics
 ****************************************************************************/

enum nxmb_exception_e nxmb_fc08_diagnostics(nxmb_handle_t ctx)
{
  uint16_t data_len = ctx->adu.length - 2;
  uint16_t subfunc;

  if (data_len < NXMB_DIAG_REQ_MIN_DATA_LEN)
    {
      return NXMB_EX_ILLEGAL_DATA_VALUE;
    }

  subfunc = nxmb_util_get_u16_be(&ctx->adu.data[NXMB_DIAG_SUBFUNC_OFF]);

  switch (subfunc)
    {
      case NXMB_DIAG_RETURN_QUERY:
        {
          /* Response echoes the request — adu is already correct */

          return NXMB_EX_NONE;
        }

      default:
        {
          return NXMB_EX_ILLEGAL_FUNCTION;
        }
    }
}
