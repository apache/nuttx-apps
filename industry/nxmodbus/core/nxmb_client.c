/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_client.c
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

#include <nxmodbus/nxmb_client.h>
#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxmb_client_state_s
{
  uint32_t timeout_ms;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxmb_client_tx_wait_rx(nxmb_handle_t ctx,
                                  uint8_t expected_uid, uint8_t expected_fc);
static int nxmb_client_validate_response(nxmb_handle_t ctx,
                                         uint8_t expected_uid,
                                         uint8_t expected_fc);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_exception_to_errno
 ****************************************************************************/

static int nxmb_exception_to_errno(uint8_t exception)
{
  switch (exception)
    {
      case NXMB_EX_ILLEGAL_FUNCTION:
        return -ENXIO;

      case NXMB_EX_ILLEGAL_DATA_ADDRESS:
        return -EFAULT;

      case NXMB_EX_ILLEGAL_DATA_VALUE:
        return -EINVAL;

      case NXMB_EX_DEVICE_FAILURE:
        return -EIO;

      case NXMB_EX_ACKNOWLEDGE:
        return -EBUSY;

      case NXMB_EX_DEVICE_BUSY:
        return -EBUSY;

      default:
        return -EPROTO;
    }
}

/****************************************************************************
 * Name: nxmb_client_validate_response
 ****************************************************************************/

static int nxmb_client_validate_response(nxmb_handle_t ctx,
                                         uint8_t expected_uid,
                                         uint8_t expected_fc)
{
  uint8_t rx_uid;
  uint8_t rx_fc;

  if (ctx->adu.length < 2)
    {
      return -EPROTO;
    }

  rx_uid = ctx->adu.unit_id;
  rx_fc  = ctx->adu.fc;

  if (rx_uid != expected_uid)
    {
      return -EPROTO;
    }

  if (rx_fc == (expected_fc | 0x80))
    {
      if (ctx->adu.length < 3)
        {
          return -EPROTO;
        }

      return nxmb_exception_to_errno(ctx->adu.data[0]);
    }

  if (rx_fc != expected_fc)
    {
      return -EPROTO;
    }

  return OK;
}

/****************************************************************************
 * Name: nxmb_client_tx_wait_rx
 ****************************************************************************/

static int nxmb_client_tx_wait_rx(nxmb_handle_t ctx, uint8_t expected_uid,
                                  uint8_t expected_fc)
{
  FAR struct nxmb_client_state_s *state;
  uint64_t                        deadline;
  int                             ret;

  if (ctx == NULL || ctx->client_state == NULL)
    {
      return -EINVAL;
    }

  state = (FAR struct nxmb_client_state_s *)ctx->client_state;

  ret = ctx->transport_ops->send(ctx);
  if (ret < 0)
    {
      return ret;
    }

  /* Broadcast frames do not receive a response */

  if (expected_uid == NXMB_ADDRESS_BROADCAST)
    {
      return OK;
    }

  deadline = nxmb_util_clock_ms() + state->timeout_ms;

  for (; ; )
    {
      if (nxmb_util_clock_ms() >= deadline)
        {
          return -ETIMEDOUT;
        }

      ret = ctx->transport_ops->receive(ctx);
      if (ret > 0)
        {
          return nxmb_client_validate_response(ctx, expected_uid,
                                               expected_fc);
        }
      else if (ret < 0 && ret != -EAGAIN)
        {
          return ret;
        }
    }
}

/****************************************************************************
 * Name: nxmb_client_read_bits
 *
 * Description:
 *   Common helper for FC01 (Read Coils) and FC02 (Read Discrete Inputs).
 *
 ****************************************************************************/

static int nxmb_client_read_bits(nxmb_handle_t ctx, uint8_t uid, uint8_t fc,
                                 uint16_t addr, uint16_t count,
                                 FAR uint8_t *buf)
{
  uint16_t nbytes;
  int      ret;

  nbytes = (count + 7) / 8;

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = fc;
  nxmb_util_put_u16_be(&ctx->adu.data[0], addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], count);
  ctx->adu.length = 6;

  ret = nxmb_client_tx_wait_rx(ctx, uid, fc);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (ctx->adu.length < (3 + nbytes) || ctx->adu.data[0] != nbytes)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  memcpy(buf, &ctx->adu.data[1], nbytes);

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_client_read_regs
 *
 * Description:
 *   Common helper for FC03 (Read Holding) and FC04 (Read Input) registers.
 *
 ****************************************************************************/

static int nxmb_client_read_regs(nxmb_handle_t ctx, uint8_t uid, uint8_t fc,
                                 uint16_t addr, uint16_t count,
                                 FAR uint16_t *buf)
{
  uint16_t nbytes;
  int      ret;
  int      i;

  nbytes = count * 2;

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = fc;
  nxmb_util_put_u16_be(&ctx->adu.data[0], addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], count);
  ctx->adu.length = 6;

  ret = nxmb_client_tx_wait_rx(ctx, uid, fc);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (ctx->adu.length < (3 + nbytes) || ctx->adu.data[0] != nbytes)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  for (i = 0; i < count; i++)
    {
      buf[i] = nxmb_util_get_u16_be(&ctx->adu.data[1 + i * 2]);
    }

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_read_coils
 ****************************************************************************/

int nxmb_read_coils(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                    uint16_t count, FAR uint8_t *buf)
{
  DEBUGASSERT(ctx && buf);

  if (uid == NXMB_ADDRESS_BROADCAST || count == 0 || count > 2000)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  return nxmb_client_read_bits(ctx, uid, NXMB_FC_READ_COILS, addr,
                               count, buf);
}

/****************************************************************************
 * Name: nxmb_read_discrete
 ****************************************************************************/

int nxmb_read_discrete(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                       uint16_t count, FAR uint8_t *buf)
{
  DEBUGASSERT(ctx && buf);

  if (uid == NXMB_ADDRESS_BROADCAST || count == 0 || count > 2000)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  return nxmb_client_read_bits(ctx, uid, NXMB_FC_READ_DISCRETE, addr,
                               count, buf);
}

/****************************************************************************
 * Name: nxmb_read_input
 ****************************************************************************/

int nxmb_read_input(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                    uint16_t count, FAR uint16_t *buf)
{
  DEBUGASSERT(ctx && buf);

  if (uid == NXMB_ADDRESS_BROADCAST || count == 0 || count > 125)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  return nxmb_client_read_regs(ctx, uid, NXMB_FC_READ_INPUT, addr,
                               count, buf);
}

/****************************************************************************
 * Name: nxmb_read_holding
 ****************************************************************************/

int nxmb_read_holding(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                      uint16_t count, FAR uint16_t *buf)
{
  DEBUGASSERT(ctx && buf);

  if (uid == NXMB_ADDRESS_BROADCAST || count == 0 || count > 125)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  return nxmb_client_read_regs(ctx, uid, NXMB_FC_READ_HOLDING, addr,
                               count, buf);
}

/****************************************************************************
 * Name: nxmb_write_coil
 ****************************************************************************/

int nxmb_write_coil(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                    bool value)
{
  uint16_t coil_value;
  int      ret;

  DEBUGASSERT(ctx);

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  coil_value = value ? 0xff00 : 0x0000;

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = NXMB_FC_WRITE_COIL;
  nxmb_util_put_u16_be(&ctx->adu.data[0], addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], coil_value);
  ctx->adu.length = 6;

  ret = nxmb_client_tx_wait_rx(ctx, uid, NXMB_FC_WRITE_COIL);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (uid != NXMB_ADDRESS_BROADCAST && ctx->adu.length < 6)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_write_holding
 ****************************************************************************/

int nxmb_write_holding(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                       uint16_t value)
{
  int ret;

  DEBUGASSERT(ctx);

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = NXMB_FC_WRITE_HOLDING;
  nxmb_util_put_u16_be(&ctx->adu.data[0], addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], value);
  ctx->adu.length = 6;

  ret = nxmb_client_tx_wait_rx(ctx, uid, NXMB_FC_WRITE_HOLDING);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (uid != NXMB_ADDRESS_BROADCAST && ctx->adu.length < 6)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_write_coils
 ****************************************************************************/

int nxmb_write_coils(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                     uint16_t count, FAR const uint8_t *buf)
{
  uint16_t nbytes;
  int      ret;

  DEBUGASSERT(ctx && buf);

  if (count == 0 || count > 1968)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  nbytes = (count + 7) / 8;

  /* Request layout in adu.data[]: addr(2) + count(2) + bcnt(1) + nbytes */

  if (5 + nbytes > NXMB_ADU_DATA_MAX)
    {
      return -EMSGSIZE;
    }

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = NXMB_FC_WRITE_COILS;
  nxmb_util_put_u16_be(&ctx->adu.data[0], addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], count);
  ctx->adu.data[4] = nbytes;
  memcpy(&ctx->adu.data[5], buf, nbytes);
  ctx->adu.length = 7 + nbytes;

  ret = nxmb_client_tx_wait_rx(ctx, uid, NXMB_FC_WRITE_COILS);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (uid != NXMB_ADDRESS_BROADCAST && ctx->adu.length < 6)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_write_holdings
 ****************************************************************************/

int nxmb_write_holdings(nxmb_handle_t ctx, uint8_t uid, uint16_t addr,
                        uint16_t count, FAR const uint16_t *buf)
{
  uint16_t nbytes;
  int      ret;
  int      i;

  DEBUGASSERT(ctx && buf);

  if (count == 0 || count > 123)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  nbytes = count * 2;

  /* Request layout in adu.data[]: addr(2) + count(2) + bcnt(1) + nbytes */

  if (5 + nbytes > NXMB_ADU_DATA_MAX)
    {
      return -EMSGSIZE;
    }

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = NXMB_FC_WRITE_HOLDINGS;
  nxmb_util_put_u16_be(&ctx->adu.data[0], addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], count);
  ctx->adu.data[4] = nbytes;

  for (i = 0; i < count; i++)
    {
      nxmb_util_put_u16_be(&ctx->adu.data[5 + i * 2], buf[i]);
    }

  ctx->adu.length = 7 + nbytes;

  ret = nxmb_client_tx_wait_rx(ctx, uid, NXMB_FC_WRITE_HOLDINGS);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (uid != NXMB_ADDRESS_BROADCAST && ctx->adu.length < 6)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_readwrite_holdings
 ****************************************************************************/

int nxmb_readwrite_holdings(nxmb_handle_t ctx, uint8_t uid, uint16_t rd_addr,
                            uint16_t rd_count, FAR uint16_t *rd_buf,
                            uint16_t wr_addr, uint16_t wr_count,
                            FAR const uint16_t *wr_buf)
{
  uint16_t rd_nbytes;
  uint16_t wr_nbytes;
  int      ret;
  int      i;

  DEBUGASSERT(ctx && rd_buf && wr_buf);

  if (uid == NXMB_ADDRESS_BROADCAST || rd_count == 0 || rd_count > 125 ||
      wr_count == 0 || wr_count > 121)
    {
      return -EINVAL;
    }

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  rd_nbytes = rd_count * 2;
  wr_nbytes = wr_count * 2;

  /* Request layout: rd_addr(2) + rd_qty(2) + wr_addr(2) + wr_qty(2) +
   * wr_bcnt(1) + wr_nbytes
   */

  if (9 + wr_nbytes > NXMB_ADU_DATA_MAX)
    {
      return -EMSGSIZE;
    }

  pthread_mutex_lock(&ctx->lock);

  ctx->adu.unit_id = uid;
  ctx->adu.fc = NXMB_FC_READWRITE_HOLDINGS;
  nxmb_util_put_u16_be(&ctx->adu.data[0], rd_addr);
  nxmb_util_put_u16_be(&ctx->adu.data[2], rd_count);
  nxmb_util_put_u16_be(&ctx->adu.data[4], wr_addr);
  nxmb_util_put_u16_be(&ctx->adu.data[6], wr_count);
  ctx->adu.data[8] = (uint8_t)wr_nbytes;

  for (i = 0; i < wr_count; i++)
    {
      nxmb_util_put_u16_be(&ctx->adu.data[9 + i * 2], wr_buf[i]);
    }

  ctx->adu.length = 11 + wr_nbytes;

  ret = nxmb_client_tx_wait_rx(ctx, uid, NXMB_FC_READWRITE_HOLDINGS);
  if (ret < 0)
    {
      pthread_mutex_unlock(&ctx->lock);
      return ret;
    }

  if (ctx->adu.length < (3 + rd_nbytes))
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  if (ctx->adu.data[0] != rd_nbytes)
    {
      pthread_mutex_unlock(&ctx->lock);
      return -EPROTO;
    }

  for (i = 0; i < rd_count; i++)
    {
      rd_buf[i] = nxmb_util_get_u16_be(&ctx->adu.data[1 + i * 2]);
    }

  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_set_timeout
 ****************************************************************************/

int nxmb_set_timeout(nxmb_handle_t ctx, uint32_t timeout_ms)
{
  FAR struct nxmb_client_state_s *state;

  DEBUGASSERT(ctx && ctx->client_state);

  if (!ctx->is_client)
    {
      return -ENOTSUP;
    }

  state = (FAR struct nxmb_client_state_s *)ctx->client_state;

  pthread_mutex_lock(&ctx->lock);
  state->timeout_ms = timeout_ms;
  pthread_mutex_unlock(&ctx->lock);

  return OK;
}

/****************************************************************************
 * Name: nxmb_client_init
 ****************************************************************************/

int nxmb_client_init(nxmb_handle_t ctx)
{
  FAR struct nxmb_client_state_s *state;

  DEBUGASSERT(ctx && ctx->is_client);

  state = calloc(1, sizeof(struct nxmb_client_state_s));
  if (state == NULL)
    {
      return -ENOMEM;
    }

  state->timeout_ms = CONFIG_NXMODBUS_CLIENT_TIMEOUT_MS;

  ctx->client_state = state;

  return OK;
}

/****************************************************************************
 * Name: nxmb_client_deinit
 ****************************************************************************/

int nxmb_client_deinit(nxmb_handle_t ctx)
{
  FAR struct nxmb_client_state_s *state;

  DEBUGASSERT(ctx && ctx->client_state);

  state = (FAR struct nxmb_client_state_s *)ctx->client_state;

  free(state);

  ctx->client_state = NULL;

  return OK;
}
