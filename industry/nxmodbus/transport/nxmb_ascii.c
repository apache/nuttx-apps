/****************************************************************************
 * apps/industry/nxmodbus/transport/nxmb_ascii.c
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

#include <ctype.h>
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

#define NXMB_ASCII_START_CHAR ':'
#define NXMB_ASCII_CR         '\r'
#define NXMB_ASCII_LF         '\n'

/* Serialized binary frame: unit_id(1) + fc(1) + data + LRC(1) */

#define NXMB_ASCII_BIN_MAX    (NXMB_ADU_DATA_MAX + 3)

/* Hex-encoded ASCII frame: ':' + 2*bin + CR + LF */

#define NXMB_ASCII_BUFLEN     (NXMB_ASCII_BIN_MAX * 2 + 4)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxmb_ascii_state_s
{
  int            fd;
  uint8_t        rx_buf[NXMB_ASCII_BUFLEN];
  uint16_t       rx_pos;
  bool           in_frame;
#ifdef CONFIG_SERIAL_TERMIOS
  struct termios old_tio;
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxmb_ascii_char_to_hex(char c);
static char nxmb_ascii_hex_to_char(uint8_t nibble);
static uint16_t nxmb_ascii_encode(FAR const uint8_t *bin, uint16_t bin_len,
                                    FAR char *ascii);
static int nxmb_ascii_decode(FAR const char *ascii, uint16_t ascii_len,
                             FAR uint8_t *bin);
static int nxmb_ascii_init(nxmb_handle_t ctx);
static int nxmb_ascii_deinit(nxmb_handle_t ctx);
static int nxmb_ascii_send(nxmb_handle_t ctx);
static int nxmb_ascii_receive(nxmb_handle_t ctx);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nxmb_transport_ops_s g_nxmb_ascii_ops =
{
  .init    = nxmb_ascii_init,
  .deinit  = nxmb_ascii_deinit,
  .send    = nxmb_ascii_send,
  .receive = nxmb_ascii_receive
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_ascii_char_to_hex
 *
 * Description:
 *   Convert ASCII hex character to binary nibble.
 *
 * Input Parameters:
 *   c - ASCII hex character
 *
 * Returned Value:
 *   Binary value 0-15, or -1 on error
 *
 ****************************************************************************/

static int nxmb_ascii_char_to_hex(char c)
{
  if (c >= '0' && c <= '9')
    {
      return c - '0';
    }
  else if (c >= 'A' && c <= 'F')
    {
      return c - 'A' + 10;
    }
  else if (c >= 'a' && c <= 'f')
    {
      return c - 'a' + 10;
    }

  return -1;
}

/****************************************************************************
 * Name: nxmb_ascii_hex_to_char
 *
 * Description:
 *   Convert binary nibble to ASCII hex character.
 *
 * Input Parameters:
 *   nibble - Binary value 0-15
 *
 * Returned Value:
 *   ASCII hex character
 *
 ****************************************************************************/

static char nxmb_ascii_hex_to_char(uint8_t nibble)
{
  if (nibble < 10)
    {
      return '0' + nibble;
    }
  else
    {
      return 'A' + (nibble - 10);
    }
}

/****************************************************************************
 * Name: nxmb_ascii_encode
 *
 * Description:
 *   Encode binary data to ASCII hex string.
 *
 * Input Parameters:
 *   bin     - Binary data
 *   bin_len - Binary data length
 *   ascii   - Output ASCII buffer
 *
 * Returned Value:
 *   ASCII string length
 *
 ****************************************************************************/

static uint16_t nxmb_ascii_encode(FAR const uint8_t *bin, uint16_t bin_len,
                                  FAR char *ascii)
{
  uint16_t pos = 0;
  uint16_t i;

  for (i = 0; i < bin_len; i++)
    {
      ascii[pos++] = nxmb_ascii_hex_to_char(bin[i] >> 4);
      ascii[pos++] = nxmb_ascii_hex_to_char(bin[i] & 0x0f);
    }

  return pos;
}

/****************************************************************************
 * Name: nxmb_ascii_decode
 *
 * Description:
 *   Decode ASCII hex string to binary data.
 *
 * Input Parameters:
 *   ascii     - ASCII hex string
 *   ascii_len - ASCII string length (must be even)
 *   bin       - Output binary buffer
 *
 * Returned Value:
 *   Binary data length, or negative errno on error
 *
 ****************************************************************************/

static int nxmb_ascii_decode(FAR const char *ascii, uint16_t ascii_len,
                             FAR uint8_t *bin)
{
  uint16_t i;
  int      high;
  int      low;

  if (ascii_len % 2 != 0)
    {
      return -EINVAL;
    }

  for (i = 0; i < ascii_len; i += 2)
    {
      high = nxmb_ascii_char_to_hex(ascii[i]);
      low  = nxmb_ascii_char_to_hex(ascii[i + 1]);

      if (high < 0 || low < 0)
        {
          return -EINVAL;
        }

      bin[i / 2] = (high << 4) | low;
    }

  return ascii_len / 2;
}

/****************************************************************************
 * Name: nxmb_ascii_init
 *
 * Description:
 *   Initialize serial ASCII transport.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_ascii_init(nxmb_handle_t ctx)
{
  FAR struct nxmb_ascii_state_s         *state;
  FAR const struct nxmb_serial_config_s *cfg;
  int                                    ret;

  DEBUGASSERT(ctx);

  cfg = &ctx->transport_cfg.serial;

  if (cfg->devpath == NULL)
    {
      return -EINVAL;
    }

  state = calloc(1, sizeof(struct nxmb_ascii_state_s));
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
#endif

  state->rx_pos   = 0;
  state->in_frame = false;

  ctx->transport_state = state;

  return 0;
}

/****************************************************************************
 * Name: nxmb_ascii_deinit
 *
 * Description:
 *   Deinitialize serial ASCII transport.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_ascii_deinit(nxmb_handle_t ctx)
{
  FAR struct nxmb_ascii_state_s *state;

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
 * Name: nxmb_ascii_send
 *
 * Description:
 *   Send ASCII frame over serial port.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_ascii_send(nxmb_handle_t ctx)
{
  FAR struct nxmb_ascii_state_s *state;
  uint8_t                        bin[NXMB_ASCII_BIN_MAX];
  char                           tx_buf[NXMB_ASCII_BUFLEN];
  uint16_t                       data_len;
  uint16_t                       pos = 0;
  uint8_t                        lrc;
  ssize_t                        written;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  if (ctx->adu.length < 2)
    {
      return -EINVAL;
    }

  data_len = ctx->adu.length - 2;

  /* Linearize the ADU into the wire byte order: unit_id, fc, data... */

  bin[0] = ctx->adu.unit_id;
  bin[1] = ctx->adu.fc;
  memcpy(&bin[2], ctx->adu.data, data_len);

  lrc          = nxmb_lrc(bin, ctx->adu.length);
  ctx->adu.crc = lrc;

  tx_buf[pos++] = NXMB_ASCII_START_CHAR;
  pos += nxmb_ascii_encode(bin, ctx->adu.length, &tx_buf[pos]);
  tx_buf[pos++] = nxmb_ascii_hex_to_char(lrc >> 4);
  tx_buf[pos++] = nxmb_ascii_hex_to_char(lrc & 0x0f);
  tx_buf[pos++] = NXMB_ASCII_CR;
  tx_buf[pos++] = NXMB_ASCII_LF;

  written = write(state->fd, tx_buf, pos);

  if (written < 0)
    {
      return -errno;
    }

  if (written != pos)
    {
      return -EIO;
    }

  return 0;
}

/****************************************************************************
 * Name: nxmb_ascii_receive
 *
 * Description:
 *   Receive ASCII frame from serial port.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

static int nxmb_ascii_receive(nxmb_handle_t ctx)
{
  FAR struct nxmb_ascii_state_s *state;
  uint8_t                        bin[NXMB_ASCII_BIN_MAX];
  struct timeval                 timeout;
  fd_set                         readfds;
  ssize_t                        nread;
  uint8_t                        c;
  uint8_t                        lrc_calc;
  uint8_t                        lrc_recv;
  int                            ret;
  int                            bin_len;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  FD_ZERO(&readfds);
  FD_SET(state->fd, &readfds);

  timeout.tv_sec  = CONFIG_NXMODBUS_ASCII_TIMEOUT_SEC;
  timeout.tv_usec = 0;

  ret = select(state->fd + 1, &readfds, NULL, NULL, &timeout);

  if (ret < 0)
    {
      return -errno;
    }

  if (ret == 0)
    {
      return 0;
    }

  if (!FD_ISSET(state->fd, &readfds))
    {
      return 0;
    }

  nread = read(state->fd, &c, 1);
  if (nread < 0)
    {
      return -errno;
    }

  if (nread == 0)
    {
      return 0;
    }

  if (c == NXMB_ASCII_START_CHAR)
    {
      state->rx_pos   = 0;
      state->in_frame = true;
      return 0;
    }

  if (!state->in_frame)
    {
      return 0;
    }

  if (c == NXMB_ASCII_LF)
    {
      state->in_frame = false;

      /* Minimum valid frame: addr(2) + func(2) + lrc(2) + CR = 7 chars.
       * Maximum valid frame: NXMB_ASCII_BIN_MAX hex pairs + CR. Reject
       * anything larger so the decoded length fits bin[].
       */

      if (state->rx_pos < 7 ||
          state->rx_pos > NXMB_ASCII_BIN_MAX * 2 + 1 ||
          state->rx_buf[state->rx_pos - 1] != NXMB_ASCII_CR)
        {
          state->rx_pos = 0;
          return 0;
        }

      /* Decode all hex chars (exclude CR) into a temp binary buffer.
       * Layout: unit_id + fc + data... + LRC.
       */

      bin_len = nxmb_ascii_decode((FAR const char *)state->rx_buf,
                                  state->rx_pos - 1, bin);

      if (bin_len < 3)
        {
          state->rx_pos = 0;
          return 0;
        }

      lrc_calc = nxmb_lrc(bin, bin_len - 1);
      lrc_recv = bin[bin_len - 1];

      if (lrc_calc != lrc_recv)
        {
          state->rx_pos = 0;
          return 0;
        }

      /* Unpack into ctx->adu. Length excludes LRC. */

      ctx->adu.unit_id = bin[0];
      ctx->adu.fc      = bin[1];
      memcpy(ctx->adu.data, &bin[2], (bin_len - 3));
      ctx->adu.crc     = lrc_recv;
      ctx->adu.length  = (bin_len - 1);

      state->rx_pos = 0;

      return 1;
    }

  if (state->rx_pos < sizeof(state->rx_buf))
    {
      state->rx_buf[state->rx_pos++] = c;
    }
  else
    {
      state->in_frame = false;
      state->rx_pos   = 0;
    }

  return 0;
}
