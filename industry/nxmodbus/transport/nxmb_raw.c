/****************************************************************************
 * apps/industry/nxmodbus/transport/nxmb_raw.c
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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nxmodbus/nxmb_raw.h>
#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MBAP_PROTO_ID 0x0000

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Raw ADU transport state */

struct nxmb_raw_state_s
{
  FAR void         *user_data;
  nxmb_raw_tx_cb_t  tx_callback;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxmb_raw_init(nxmb_handle_t ctx);
static int nxmb_raw_deinit(nxmb_handle_t ctx);
static int nxmb_raw_send(nxmb_handle_t ctx);
static int nxmb_raw_receive(nxmb_handle_t ctx);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nxmb_transport_ops_s g_nxmb_raw_ops =
{
  .init    = nxmb_raw_init,
  .deinit  = nxmb_raw_deinit,
  .send    = nxmb_raw_send,
  .receive = nxmb_raw_receive,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_raw_init
 *
 * Description:
 *   Initialize raw ADU transport.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_raw_init(nxmb_handle_t ctx)
{
  FAR const struct nxmb_raw_config_s *cfg;
  FAR struct nxmb_raw_state_s        *state;

  DEBUGASSERT(ctx);

  cfg = &ctx->transport_cfg.raw;

  if (cfg->tx_cb == NULL)
    {
      return -EINVAL;
    }

  state = malloc(sizeof(struct nxmb_raw_state_s));
  if (state == NULL)
    {
      return -ENOMEM;
    }

  state->tx_callback = cfg->tx_cb;
  state->user_data   = cfg->user_data;

  ctx->transport_state = state;

  return OK;
}

/****************************************************************************
 * Name: nxmb_raw_deinit
 *
 * Description:
 *   Deinitialize raw ADU transport.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_raw_deinit(nxmb_handle_t ctx)
{
  DEBUGASSERT(ctx && ctx->transport_state);

  free(ctx->transport_state);
  ctx->transport_state = NULL;

  return OK;
}

/****************************************************************************
 * Name: nxmb_raw_send
 *
 * Description:
 *   Send raw ADU via user callback.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_raw_send(nxmb_handle_t ctx)
{
  FAR struct nxmb_raw_state_s *state;
  int                          ret;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  if (state->tx_callback == NULL)
    {
      return -EINVAL;
    }

  ctx->adu.proto_id = MBAP_PROTO_ID;

  ret = state->tx_callback(&ctx->adu, state->user_data);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: nxmb_raw_receive
 *
 * Description:
 *   Receive raw ADU (not used in raw mode, frames are submitted via
 *   nxmb_raw_submit_rx).
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   -ENOTSUP (raw mode uses submit pattern, not polling)
 *
 ****************************************************************************/

static int nxmb_raw_receive(nxmb_handle_t ctx)
{
  return -ENOTSUP;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_raw_submit_rx
 *
 * Description:
 *   Submit a received raw ADU into the NxModbus protocol core.
 *
 * Input Parameters:
 *   h   - The NxModbus instance receiving the frame
 *   adu - The received raw ADU container
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_raw_submit_rx(nxmb_handle_t ctx, FAR const struct nxmb_adu_s *adu)
{
  DEBUGASSERT(ctx && adu);

  if (adu->proto_id != MBAP_PROTO_ID)
    {
      return -EPROTO;
    }

  if (adu->length < 2)
    {
      return -EINVAL;
    }

  if ((adu->length - 2) > NXMB_ADU_DATA_MAX)
    {
      return -EMSGSIZE;
    }

  pthread_mutex_lock(&ctx->lock);
  ctx->adu = *adu;
  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_raw_put_header
 *
 * Description:
 *   Serialize the 8-byte MBAP+FC header for an ADU.
 *
 * Input Parameters:
 *   adu - The ADU providing the header fields
 *   hdr - The destination buffer for the serialized header
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxmb_raw_put_header(FAR const struct nxmb_adu_s *adu, FAR uint8_t *hdr)
{
  DEBUGASSERT(adu && hdr);

  nxmb_util_put_u16_be(&hdr[0], adu->trans_id);
  nxmb_util_put_u16_be(&hdr[2], adu->proto_id);
  nxmb_util_put_u16_be(&hdr[4], adu->length);
  hdr[6] = adu->unit_id;
  hdr[7] = adu->fc;
}

/****************************************************************************
 * Name: nxmb_raw_get_header
 *
 * Description:
 *   Parse the 8-byte MBAP+FC header into an ADU structure.
 *
 * Input Parameters:
 *   adu - The destination ADU structure
 *   hdr - The source buffer containing the serialized header
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxmb_raw_get_header(FAR struct nxmb_adu_s *adu, FAR const uint8_t *hdr)
{
  DEBUGASSERT(adu && hdr);

  adu->trans_id = nxmb_util_get_u16_be(&hdr[0]);
  adu->proto_id = nxmb_util_get_u16_be(&hdr[2]);
  adu->length   = nxmb_util_get_u16_be(&hdr[4]);
  adu->unit_id  = hdr[6];
  adu->fc       = hdr[7];
}
