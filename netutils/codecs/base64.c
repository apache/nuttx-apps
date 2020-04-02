/****************************************************************************
 * apps/netutils/codecs/base64.c
 *
 * This file is part of the NuttX RTOS:
 *
 *   Copyright (C) 2012, 2018 Gregory Nutt. All rights reserved.
 *   Author: Darcy Gong
 *
 * Reference:
 *
 *   Base64 encoding/decoding (RFC1341)
 *   Copyright (c) 2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   Alternatively, this software may be distributed under the terms of BSD
 *   license.
 *
 *   See README and COPYING for more details.
 *
 * And is re-released under the NuttX modified BSD license:
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the Institute nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS''
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS
 *   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "netutils/base64.h"

#ifdef CONFIG_CODECS_BASE64

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: base64_tab
 ****************************************************************************/

static FAR const char *base64_tab(bool websafe)
{
  static FAR const char *_tab =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  static FAR const char *_tab_w =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789__";

  return websafe ? _tab_w : _tab;
}

/****************************************************************************
 * Name: _base64_encode
 *
 * Description:
 *   Base64 encode
 *
 *   Caller is responsible for freeing the returned buffer. Returned buffer
 *   is NUL terminated to make it easier to use as a C string. The NUL
 *   terminator is not included in out_len.
 *
 * Input Parameters:
 *   src:     Data to be encoded
 *   len:     Length of the data to be encoded
 *   out_len: Pointer to output length variable, or NULL if not used
 *
 * Returned Value:
 *   Returns: Allocated buffer of out_len bytes of encoded data,
 *   or NULL on failure
 *
 ****************************************************************************/

static FAR unsigned char *_base64_encode(FAR const unsigned char *src,
                                         size_t len,
                                         FAR unsigned char *dst,
                                         FAR size_t *out_len,
                                         bool websafe)
{
  FAR unsigned char *out;
  FAR unsigned char *pos;
  FAR const unsigned char *end;
  FAR const unsigned char *in;
  FAR const char *base64_table;
  char ch = '=';
  size_t olen;

  if (websafe)
    {
      ch = '.';
    }

  base64_table = base64_tab(websafe);
  olen = (len + 2) / 3 * 4 + 1; /* 3-byte blocks to 4-byte */

  end = src + len;
  in = src;

  if (dst)
    {
      pos = out = dst;
    }
  else
    {
      pos = out = malloc(olen);
      if (out == NULL)
        {
          return NULL;
        }
    }

  while (end - in >= 3)
    {
      *pos++ = base64_table[in[0] >> 2];
      *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
      *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
      *pos++ = base64_table[in[2] & 0x3f];
      in += 3;
    }

  if (end - in != 0)
    {
      *pos++ = base64_table[in[0] >> 2];
      if (end - in == 1)
        {
          *pos++ = base64_table[(in[0] & 0x03) << 4];
          *pos++ = ch;          /* *pos++ = '='; */
        }
      else
        {
          *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
          *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }

      *pos++ = ch;              /* *pos++ = '='; */
    }

  *pos = '\0';
  if (out_len)
    {
      *out_len = pos - out;
    }

  return out;
}

/****************************************************************************
 * Name: _base64_decode
 *
 * Description:
 *   Base64 decode
 *
 * Caller is responsible for freeing the returned buffer.
 *
 * Input Parameters:
 *   src:     Data to be decoded
 *   len:     Length of the data to be decoded
 *   out_len: Pointer to output length variable
 *
 * Returned Value:
 *   Returns: Allocated buffer of out_len bytes of decoded data,
 *   or NULL on failure
 *
 ****************************************************************************/

static unsigned char *_base64_decode(FAR const unsigned char *src,
                                     size_t len, FAR unsigned char *dst,
                                     FAR size_t *out_len, bool websafe)
{
  FAR unsigned char *out;
  FAR unsigned char *pos;
  FAR unsigned char block[4];
  FAR const char *base64_table;
  char ch = '=';
  FAR char *tmp;
  size_t count;
  size_t i;

  if (websafe)
    {
      ch = '.';
    }

  base64_table = base64_tab(websafe);

  if (dst)
    {
      pos = out = dst;
    }
  else
    {
      pos = out = malloc(len / 4 * 3);
      if (out == NULL)
        {
          return NULL;
        }
    }

  count = 0;
  for (i = 0; i < len; i++)
    {
      tmp = strchr(base64_table, src[i]);
      block[count] = tmp ? tmp - base64_table : 0;
      count++;

      if (count == 4)
        {
          *pos++ = (block[0] << 2) | (block[1] >> 4);
          if (src[i - 1] == ch)
            {
              break;
            }

          *pos++ = (block[1] << 4) | (block[2] >> 2);
          if (src[i] == ch)
            {
              break;
            }

          *pos++ = (block[2] << 6) | block[3];
          count = 0;
        }
    }

  *out_len = pos - out;
  return out;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: base64_encode_length
 ****************************************************************************/

size_t base64_encode_length(size_t len)
{
  return (len + 2) / 3 * 4;
}

/****************************************************************************
 * Name: base64_encode
 ****************************************************************************/

FAR void *base64_encode(FAR const void *src, size_t len, FAR void *dst,
                       FAR size_t *out_len)
{
  return _base64_encode(src, len, dst, out_len, false);
}

/****************************************************************************
 * Name: base64_decode_length
 ****************************************************************************/

size_t base64_decode_length(size_t len)
{
  return len * 3 / 4;
}

/****************************************************************************
 * Name: base64_decode
 ****************************************************************************/

FAR void *base64_decode(FAR const void *src, size_t len, FAR void *dst,
                        FAR size_t *out_len)
{
  return _base64_decode(src, len, dst, out_len, false);
}

/****************************************************************************
 * Name: base64w_encode
 ****************************************************************************/

FAR void *base64w_encode(FAR const void *src, size_t len, FAR void *dst,
                         FAR size_t *out_len)
{
  return _base64_encode(src, len, dst, out_len, true);
}

/****************************************************************************
 * Name: base64w_decode
 ****************************************************************************/

FAR void *base64w_decode(FAR const void *src, size_t len, FAR void *dst,
                         FAR size_t *out_len)
{
  return _base64_decode(src, len, dst, out_len, true);
}

#endif /* CONFIG_CODECS_BASE64 */
