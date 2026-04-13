/****************************************************************************
 * apps/industry/nxmodbus/transport/nxmb_serial.c
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
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#ifdef CONFIG_SERIAL_TERMIOS
#  include <termios.h>
#endif

#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXMB_RTU_MIN_FRAME_SIZE 5

/* Wire frame: unit_id + fc + data + 2 CRC bytes */

#define NXMB_RTU_FRAME_MAX      (NXMB_ADU_DATA_MAX + 4)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxmb_serial_state_s
{
  int            fd;
  uint32_t       t35_usec;
  bool           rx_frame_now;
  uint16_t       rx_pos;
  uint8_t        rx_buf[NXMB_RTU_FRAME_MAX];
#ifdef CONFIG_SERIAL_TERMIOS
  struct termios old_tio;
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static uint32_t nxmb_serial_calculate_t35(uint32_t baudrate);
static int nxmb_serial_init(nxmb_handle_t ctx);
static int nxmb_serial_deinit(nxmb_handle_t ctx);
static int nxmb_serial_send(nxmb_handle_t ctx);
static int nxmb_serial_receive(nxmb_handle_t ctx);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nxmb_transport_ops_s g_nxmb_serial_ops =
{
  .init    = nxmb_serial_init,
  .deinit  = nxmb_serial_deinit,
  .send    = nxmb_serial_send,
  .receive = nxmb_serial_receive
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_serial_calculate_t35
 *
 * Description:
 *   Calculate T3.5 character timeout in microseconds based on baud rate.
 *
 * Input Parameters:
 *   baudrate - Serial baud rate
 *
 * Returned Value:
 *   T3.5 timeout in microseconds
 *
 ****************************************************************************/

static uint32_t nxmb_serial_calculate_t35(uint32_t baudrate)
{
  if (baudrate <= 19200)
    {
      /* T3.5 = 3.5 * 11 bits/char * 1e6 us/s / baudrate
       * = 38500000 / baudrate (in microseconds).
       * Use integer arithmetic to avoid floating-point, which may
       * require FPU context save/restore on embedded targets.
       */

      return 38500000u / baudrate;
    }
  else
    {
      /* Per Modbus spec, fixed 1750 us for baud rates above 19200 */

      return 1750;
    }
}

/****************************************************************************
 * Name: nxmb_serial_init
 *
 * Description:
 *   Initialize serial RTU transport.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_serial_init(nxmb_handle_t ctx)
{
  FAR struct nxmb_serial_state_s        *state;
  FAR const struct nxmb_serial_config_s *cfg;
  int                                    ret;

  DEBUGASSERT(ctx);

  cfg = &ctx->transport_cfg.serial;

  if (cfg->devpath == NULL)
    {
      return -EINVAL;
    }

  state = calloc(1, sizeof(struct nxmb_serial_state_s));
  if (state == NULL)
    {
      return -ENOMEM;
    }

  state->fd = open(cfg->devpath, O_RDWR | O_NOCTTY);
  if (state->fd < 0)
    {
      ret = -errno;
      free(state);
      return ret;
    }

#ifdef CONFIG_SERIAL_TERMIOS
  if (tcgetattr(state->fd, &state->old_tio) < 0)
    {
      ret = -errno;
      close(state->fd);
      free(state);
      return ret;
    }

  ret = nxmb_serial_configure(state->fd, cfg->baudrate, cfg->parity);

  if (ret < 0)
    {
      close(state->fd);
      free(state);
      return ret;
    }

  tcflush(state->fd, TCIOFLUSH);
#endif

  state->t35_usec     = nxmb_serial_calculate_t35(cfg->baudrate);
  state->rx_frame_now = false;

  ctx->transport_state = state;

  return 0;
}

/****************************************************************************
 * Name: nxmb_serial_deinit
 *
 * Description:
 *   Deinitialize serial RTU transport.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_serial_deinit(nxmb_handle_t ctx)
{
  FAR struct nxmb_serial_state_s *state;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

#ifdef CONFIG_SERIAL_TERMIOS
  tcsetattr(state->fd, TCSANOW, &state->old_tio);
#endif
  close(state->fd);
  free(state);

  ctx->transport_state = NULL;

  return 0;
}

/****************************************************************************
 * Name: nxmb_serial_send
 *
 * Description:
 *   Send RTU frame over serial port.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_serial_send(nxmb_handle_t ctx)
{
  FAR struct nxmb_serial_state_s *state;
  uint8_t                         frame[NXMB_RTU_FRAME_MAX];
  uint16_t                        data_len;
  uint16_t                        total;
  uint16_t                        crc;
  ssize_t                         written;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  if (ctx->adu.length < 2)
    {
      return -EINVAL;
    }

  data_len = ctx->adu.length - 2;

  /* Linearize the ADU and append CRC: unit_id, fc, data..., crc_lo, crc_hi */

  frame[0] = ctx->adu.unit_id;
  frame[1] = ctx->adu.fc;
  memcpy(&frame[2], ctx->adu.data, data_len);

  crc = nxmb_crc16(frame, ctx->adu.length);

  frame[ctx->adu.length]     = (crc & 0xff);
  frame[ctx->adu.length + 1] = (crc >> 8);

  ctx->adu.crc = crc;

  total   = ctx->adu.length + 2;
  written = write(state->fd, frame, total);

  if (written < 0)
    {
      return -errno;
    }

  if (written != total)
    {
      return -EIO;
    }

  return 0;
}

/****************************************************************************
 * Name: nxmb_serial_receive
 *
 * Description:
 *   Receive RTU frame from serial port with dynamic timeout.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_serial_receive(nxmb_handle_t ctx)
{
  FAR struct nxmb_serial_state_s *state;
  struct timeval                  timeout;
  fd_set                          readfds;
  ssize_t                         nread;
  uint16_t                        crc_calc;
  uint16_t                        crc_recv;
  uint16_t                        data_len;
  int                             ret;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  FD_ZERO(&readfds);
  FD_SET(state->fd, &readfds);

  if (state->rx_frame_now)
    {
      timeout.tv_sec  = 0;
      timeout.tv_usec = state->t35_usec;
    }
  else
    {
      timeout.tv_sec  = 0;
      timeout.tv_usec = CONFIG_NXMODBUS_RTU_IDLE_TIMEOUT_MS * 1000;
    }

  ret = select(state->fd + 1, &readfds, NULL, NULL, &timeout);

  if (ret < 0)
    {
      return -errno;
    }

  if (ret == 0)
    {
      if (state->rx_frame_now)
        {
          state->rx_frame_now = false;

          if (state->rx_pos < NXMB_RTU_MIN_FRAME_SIZE)
            {
              state->rx_pos = 0;
              return 0;
            }

          crc_calc = nxmb_crc16(state->rx_buf, state->rx_pos - 2);
          crc_recv = state->rx_buf[state->rx_pos - 2] |
                     (state->rx_buf[state->rx_pos - 1] << 8);

          if (crc_calc != crc_recv)
            {
              state->rx_pos = 0;
              return 0;
            }

          /* Unpack frame into ctx->adu (excluding the trailing CRC). */

          data_len         = (state->rx_pos - 4);
          ctx->adu.unit_id = state->rx_buf[0];
          ctx->adu.fc      = state->rx_buf[1];
          memcpy(ctx->adu.data, &state->rx_buf[2], data_len);
          ctx->adu.crc     = crc_recv;
          ctx->adu.length  = (state->rx_pos - 2);

          state->rx_pos = 0;
          return 1;
        }

      return 0;
    }

  if (!FD_ISSET(state->fd, &readfds))
    {
      return 0;
    }

  if (!state->rx_frame_now)
    {
      state->rx_pos       = 0;
      state->rx_frame_now = true;
    }

  nread = read(state->fd,
               &state->rx_buf[state->rx_pos],
               sizeof(state->rx_buf) - state->rx_pos);
  if (nread < 0)
    {
      return -errno;
    }

  if (nread == 0)
    {
      return 0;
    }

  state->rx_pos += nread;

  if (state->rx_pos >= sizeof(state->rx_buf))
    {
      state->rx_pos       = 0;
      state->rx_frame_now = false;
      return 0;
    }

  return 0;
}
