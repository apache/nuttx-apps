/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_func.c
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXMB_EXCEPTION_FLAG 0x80

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Function code handler signature */

typedef enum nxmb_exception_e (*nxmb_fc_handler_t)(nxmb_handle_t ctx);

/* Dispatch table entry */

struct nxmb_fc_entry_s
{
  uint8_t           fc;
  nxmb_fc_handler_t handler;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Standard function code dispatch table */

static const struct nxmb_fc_entry_s g_nxmb_fc_table[] =
{
#ifdef CONFIG_NXMODBUS_FUNC_READ_COILS
  { NXMB_FC_READ_COILS, nxmb_fc01_read_coils },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_READ_DISCRETE
  { NXMB_FC_READ_DISCRETE, nxmb_fc02_read_discrete },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_READ_HOLDING
  { NXMB_FC_READ_HOLDING, nxmb_fc03_read_holding },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_READ_INPUT
  { NXMB_FC_READ_INPUT, nxmb_fc04_read_input },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_WRITE_COIL
  { NXMB_FC_WRITE_COIL, nxmb_fc05_write_coil },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_WRITE_HOLDING
  { NXMB_FC_WRITE_HOLDING, nxmb_fc06_write_holding },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_DIAGNOSTICS
  { NXMB_FC_DIAGNOSTICS, nxmb_fc08_diagnostics },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_WRITE_COILS
  { NXMB_FC_WRITE_COILS, nxmb_fc15_write_coils },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_WRITE_HOLDINGS
  { NXMB_FC_WRITE_HOLDINGS, nxmb_fc16_write_holdings },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_REPORT_SERVER_ID
  { NXMB_FC_REPORT_SERVER_ID, nxmb_fc17_report_server_id },
#endif
#ifdef CONFIG_NXMODBUS_FUNC_READWRITE_HOLDINGS
  { NXMB_FC_READWRITE_HOLDINGS, nxmb_fc23_readwrite_holding }
#endif
};

#define NXMB_FC_TABLE_SIZE                                                     \
  (sizeof(g_nxmb_fc_table) / sizeof(g_nxmb_fc_table[0]))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_build_exception
 *
 * Description:
 *   Build a Modbus exception response in ctx->adu in place.
 *
 * Input Parameters:
 *   ctx       - Instance context
 *   fc        - Function code
 *   exception - NxModbus exception code
 *
 ****************************************************************************/

static void nxmb_build_exception(nxmb_handle_t ctx, uint8_t fc,
                                 enum nxmb_exception_e exception)
{
  ctx->adu.fc      = fc | NXMB_EXCEPTION_FLAG;
  ctx->adu.data[0] = (uint8_t)exception;

  /* unit_id + fc + 1 exception byte */

  ctx->adu.length  = 3;
}

/****************************************************************************
 * Name: nxmb_lookup_standard_fc
 *
 * Description:
 *   Look up a standard function code handler in the dispatch table.
 *
 * Input Parameters:
 *   fc - Function code to look up
 *
 * Returned Value:
 *   Handler function pointer, or NULL if not found
 *
 ****************************************************************************/

static nxmb_fc_handler_t nxmb_lookup_standard_fc(uint8_t fc)
{
  int i;

  for (i = 0; i < NXMB_FC_TABLE_SIZE; i++)
    {
      if (g_nxmb_fc_table[i].fc == fc)
        {
          return g_nxmb_fc_table[i].handler;
        }
    }

  return NULL;
}

#ifdef CONFIG_NXMODBUS_CUSTOM_FC

/****************************************************************************
 * Name: nxmb_lookup_custom_fc
 *
 * Description:
 *   Look up a custom function code handler in the instance list.
 *
 * Input Parameters:
 *   ctx - Instance context
 *   fc  - Function code to look up
 *
 * Returned Value:
 *   Handler function pointer, or NULL if not found
 *
 ****************************************************************************/

static nxmb_custom_fc_handler_t nxmb_lookup_custom_fc(
  nxmb_handle_t ctx, uint8_t fc)
{
  FAR struct nxmb_custom_fc_s *entry;

  for (entry = ctx->custom_fc_list; entry != NULL; entry = entry->next)
    {
      if (entry->fc == fc)
        {
          return entry->handler;
        }
    }

  return NULL;
}

#endif /* CONFIG_NXMODBUS_CUSTOM_FC */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_dispatch_function
 *
 * Description:
 *   Dispatch a Modbus function code to the appropriate handler. Operates in
 *   place on ctx->adu. Checks standard handlers first, then custom handlers
 *   if enabled. Builds an exception response if no handler is found or the
 *   handler fails.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_dispatch_function(nxmb_handle_t ctx)
{
  nxmb_fc_handler_t        handler;
  enum nxmb_exception_e    exception;
  uint8_t                  fc;
#ifdef CONFIG_NXMODBUS_CUSTOM_FC
  nxmb_custom_fc_handler_t custom_handler;
  int                      ret;
#endif

  DEBUGASSERT(ctx);

  if (ctx->adu.length < 2)
    {
      return -EINVAL;
    }

  fc = ctx->adu.fc;

  /* Look up standard handler first */

  handler = nxmb_lookup_standard_fc(fc);

#ifdef CONFIG_NXMODBUS_CUSTOM_FC

  /* If not found, check custom handlers */

  if (handler == NULL)
    {
      custom_handler = nxmb_lookup_custom_fc(ctx, fc);
      if (custom_handler != NULL)
        {
          ret = custom_handler(ctx);
          if (ret < 0)
            {
              nxmb_build_exception(ctx, fc, NXMB_EX_DEVICE_FAILURE);
            }

          return 0;
        }
    }
#endif

  /* If no handler found, return illegal function exception */

  if (handler == NULL)
    {
      nxmb_build_exception(ctx, fc, NXMB_EX_ILLEGAL_FUNCTION);
      return 0;
    }

  /* Call the handler */

  exception = handler(ctx);

  /* If handler returned exception, build exception response */

  if (exception != NXMB_EX_NONE)
    {
      nxmb_build_exception(ctx, fc, exception);
    }

  return 0;
}

#ifdef CONFIG_NXMODBUS_CUSTOM_FC
/****************************************************************************
 * Name: nxmb_register_custom_fc
 *
 * Description:
 *   Register a custom Modbus function code handler for an instance.
 *
 *   Custom handlers are checked after standard handlers, so standard
 *   function codes (FC01-FC16) cannot be overridden.
 *
 * Input Parameters:
 *   h       - The NxModbus instance handle
 *   fc      - The custom function code to register
 *   handler - The handler function
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_register_custom_fc(nxmb_handle_t ctx, uint8_t fc,
                            nxmb_custom_fc_handler_t handler)
{
  FAR struct nxmb_custom_fc_s *entry;
  FAR struct nxmb_custom_fc_s *new_entry;

  DEBUGASSERT(ctx && handler);

  /* Check if FC is already a standard handler */

  if (nxmb_lookup_standard_fc(fc) != NULL)
    {
      return -EEXIST;
    }

  pthread_mutex_lock(&ctx->lock);

  /* Check if FC is already registered as custom */

  for (entry = ctx->custom_fc_list; entry != NULL; entry = entry->next)
    {
      if (entry->fc == fc)
        {
          pthread_mutex_unlock(&ctx->lock);
          return -EEXIST;
        }
    }

  /* Allocate new entry */

  new_entry = malloc(sizeof(struct nxmb_custom_fc_s));
  if (new_entry == NULL)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -ENOMEM;
    }

  /* Initialize and prepend to list */

  new_entry->fc       = fc;
  new_entry->handler  = handler;
  new_entry->next     = ctx->custom_fc_list;
  ctx->custom_fc_list = new_entry;

  pthread_mutex_unlock(&ctx->lock);
  return 0;
}

#endif /* CONFIG_NXMODBUS_CUSTOM_FC */
