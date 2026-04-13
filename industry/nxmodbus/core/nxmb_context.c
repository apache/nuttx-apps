/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_context.c
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
#include <nuttx/debug.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxmb_context_s g_nxmb_pool[CONFIG_NXMODBUS_MAX_INSTANCES];
static pthread_mutex_t       g_pool_lock = PTHREAD_MUTEX_INITIALIZER;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_create
 *
 * Description:
 *   Create a new NxModbus instance.
 *
 * Input Parameters:
 *   handle - Pointer to receive the instance handle
 *   config - Instance configuration
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_create(FAR nxmb_handle_t *handle,
                FAR const struct nxmb_config_s *config)
{
  nxmb_handle_t ctx = NULL;
  int           ret = -ENOMEM;
  int           i;

  DEBUGASSERT(handle && config);

#ifndef CONFIG_NXMODBUS_CLIENT
  if (config->is_client)
    {
      /* Master not supported */

      return -EINVAL;
    }
#endif

  pthread_mutex_lock(&g_pool_lock);

  for (i = 0; i < CONFIG_NXMODBUS_MAX_INSTANCES; i++)
    {
      if (g_nxmb_pool[i].mode == NXMB_MODE_INVAL)
        {
          ctx = &g_nxmb_pool[i];
          break;
        }
    }

  if (ctx != NULL)
    {
      memset(ctx, 0, sizeof(struct nxmb_context_s));

      ret = pthread_mutex_init(&ctx->lock, NULL);
      if (ret == 0)
        {
          ctx->mode      = config->mode;
          ctx->unit_id   = config->unit_id;
          ctx->is_client = config->is_client;

          switch (config->mode)
            {
#ifdef CONFIG_NXMODBUS_RTU
              case NXMB_MODE_RTU:
                ctx->transport_cfg.serial = config->transport.serial;
                ctx->transport_ops        = &g_nxmb_serial_ops;
                break;
#endif

#ifdef CONFIG_NXMODBUS_ASCII
              case NXMB_MODE_ASCII:
                ctx->transport_cfg.serial = config->transport.serial;
                ctx->transport_ops        = &g_nxmb_ascii_ops;
                break;
#endif

#ifdef CONFIG_NXMODBUS_TCP
              case NXMB_MODE_TCP:
                ctx->transport_cfg.tcp = config->transport.tcp;
                ctx->transport_ops     = &g_nxmb_tcp_ops;
                break;
#endif

#ifdef CONFIG_NXMODBUS_RAW_ADU
              case NXMB_MODE_RAW:
                ctx->transport_cfg.raw = config->transport.raw;
                ctx->transport_ops     = &g_nxmb_raw_ops;
                break;
#endif

              default:
                pthread_mutex_destroy(&ctx->lock);
                memset(ctx, 0, sizeof(struct nxmb_context_s));
                ret = -EINVAL;
                goto out;
            }

          *handle = ctx;
          ret     = 0;
        }
      else
        {
          memset(ctx, 0, sizeof(struct nxmb_context_s));
          ret = -ret;
        }
    }

out:
  pthread_mutex_unlock(&g_pool_lock);
  return ret;
}

/****************************************************************************
 * Name: nxmb_destroy
 *
 * Description:
 *   Destroy an NxModbus instance.
 *
 * Input Parameters:
 *   handle - Instance handle
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_destroy(nxmb_handle_t ctx)
{
#ifdef CONFIG_NXMODBUS_CUSTOM_FC
  FAR struct nxmb_custom_fc_s *entry;
  FAR struct nxmb_custom_fc_s *next;
#endif

  DEBUGASSERT(ctx);

  pthread_mutex_lock(&g_pool_lock);
  pthread_mutex_lock(&ctx->lock);

  if (ctx->enabled)
    {
      pthread_mutex_unlock(&ctx->lock);
      pthread_mutex_unlock(&g_pool_lock);
      return -EBUSY;
    }

  pthread_mutex_unlock(&ctx->lock);
  pthread_mutex_destroy(&ctx->lock);

#ifdef CONFIG_NXMODBUS_CUSTOM_FC
  entry = ctx->custom_fc_list;
  while (entry != NULL)
    {
      next = entry->next;
      free(entry);
      entry = next;
    }
#endif

  memset(ctx, 0, sizeof(struct nxmb_context_s));
  pthread_mutex_unlock(&g_pool_lock);

  return 0;
}

/****************************************************************************
 * Name: nxmb_set_callbacks
 *
 * Description:
 *   Set application callbacks for register access.
 *
 * Input Parameters:
 *   handle    - Instance handle
 *   callbacks - Pointer to callback structure
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_set_callbacks(nxmb_handle_t ctx,
                       FAR const struct nxmb_callbacks_s *callbacks)
{
  DEBUGASSERT(ctx);

  pthread_mutex_lock(&ctx->lock);
  ctx->callbacks = callbacks;
  pthread_mutex_unlock(&ctx->lock);

  return 0;
}

/****************************************************************************
 * Name: nxmb_enable
 *
 * Description:
 *   Enable an NxModbus instance and initialize transport.
 *
 * Input Parameters:
 *   handle - Instance handle
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_enable(nxmb_handle_t ctx)
{
  int ret;

  DEBUGASSERT(ctx && ctx->transport_ops && ctx->transport_ops->init);

  pthread_mutex_lock(&ctx->lock);

  if (ctx->enabled)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EBUSY;
    }

#ifdef CONFIG_NXMODBUS_CLIENT
  if (ctx->is_client)
    {
      ret = nxmb_client_init(ctx);
      if (ret < 0)
        {
          pthread_mutex_unlock(&ctx->lock);
          return ret;
        }
    }
#endif

  /* Initialize transport */

  ret = ctx->transport_ops->init(ctx);
  if (ret < 0)
    {
#ifdef CONFIG_NXMODBUS_CLIENT
      if (ctx->is_client)
        {
          nxmb_client_deinit(ctx);
        }
#endif

      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  ctx->enabled = true;
  pthread_mutex_unlock(&ctx->lock);

  return 0;
}

/****************************************************************************
 * Name: nxmb_set_server_id
 *
 * Description:
 *   Configure the data returned by FC17 (Report Server ID).
 *
 * Input Parameters:
 *   handle     - Instance handle
 *   id         - Server ID byte
 *   is_running - True if device is running
 *   additional - Optional additional data (may be NULL)
 *   addlen     - Length of additional data
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_set_server_id(nxmb_handle_t ctx, uint8_t id, bool is_running,
                       FAR const uint8_t *additional, uint16_t addlen)
{
  DEBUGASSERT(ctx);

  /* The first two bytes are server ID and run indicator.
   * The rest is optional additional data.
   */

  if (addlen + 2 > CONFIG_NXMODBUS_REP_SERVER_ID_BUF)
    {
      return -ENOMEM;
    }

  pthread_mutex_lock(&ctx->lock);

  ctx->server_id_len = 0;
  ctx->server_id_buf[ctx->server_id_len++] = id;
  ctx->server_id_buf[ctx->server_id_len++] =
    (uint8_t)(is_running ? 0xff : 0x00);

  if (addlen > 0 && additional != NULL)
    {
      memcpy(&ctx->server_id_buf[ctx->server_id_len], additional,
             (size_t)addlen);
      ctx->server_id_len += addlen;
    }

  pthread_mutex_unlock(&ctx->lock);

  return 0;
}

/****************************************************************************
 * Name: nxmb_disable
 *
 * Description:
 *   Disable an NxModbus instance and deinitialize transport.
 *
 * Input Parameters:
 *   handle - Instance handle
 *
 * Returned Value:
 *   0 on success, negative errno on failure
 *
 ****************************************************************************/

int nxmb_disable(nxmb_handle_t ctx)
{
  DEBUGASSERT(ctx && ctx->transport_ops && ctx->transport_ops->deinit);

  pthread_mutex_lock(&ctx->lock);

  if (!ctx->enabled)
    {
      pthread_mutex_unlock(&ctx->lock);
      return 0;
    }

  ctx->transport_ops->deinit(ctx);

#ifdef CONFIG_NXMODBUS_CLIENT
  if (ctx->is_client)
    {
      nxmb_client_deinit(ctx);
    }
#endif

  ctx->enabled = false;
  pthread_mutex_unlock(&ctx->lock);

  return 0;
}
