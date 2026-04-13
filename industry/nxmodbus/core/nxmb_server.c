/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_server.c
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

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXMB_FUNC_ERROR 0x80

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_poll
 *
 * Description:
 *   Poll for Modbus events and process received frames.
 *
 * Input Parameters:
 *   handle - Instance handle
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_poll(nxmb_handle_t ctx)
{
  uint8_t rcv_address;
  int     ret;

  DEBUGASSERT(ctx && ctx->transport_ops && ctx->transport_ops->receive);

  pthread_mutex_lock(&ctx->lock);

  if (!ctx->enabled)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EAGAIN;
    }

  ret = ctx->transport_ops->receive(ctx);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (ret == 0 || ctx->adu.length == 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return 0;
    }

  rcv_address = ctx->adu.unit_id;

  if (rcv_address != NXMB_ADDRESS_BROADCAST && rcv_address != ctx->unit_id)
    {
      pthread_mutex_unlock(&ctx->lock);
      return 0;
    }

  /* Dispatch to function code handler. Handlers mutate ctx->adu in place,
   * turning the request into the response.
   */

  ret = nxmb_dispatch_function(ctx);

  if (rcv_address != NXMB_ADDRESS_BROADCAST)
    {
      if (ctx->transport_ops->send != NULL)
        {
          ret = ctx->transport_ops->send(ctx);
          if (ret < 0)
            {
              pthread_mutex_unlock(&ctx->lock);
              return ret;
            }
        }
    }

  pthread_mutex_unlock(&ctx->lock);
  return 0;
}
