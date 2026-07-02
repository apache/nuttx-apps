/****************************************************************************
 * apps/fsutils/passwd/passwd_base64.c
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
#include <stdint.h>

#include "passwd_base64.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RFC 4648 section 5 base64url alphabet (no padding). */

static const char g_base64url[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int passwd_base64url_val(char c)
{
  if (c >= 'A' && c <= 'Z')
    {
      return c - 'A';
    }

  if (c >= 'a' && c <= 'z')
    {
      return c - 'a' + 26;
    }

  if (c >= '0' && c <= '9')
    {
      return c - '0' + 52;
    }

  if (c == '-')
    {
      return 62;
    }

  if (c == '_')
    {
      return 63;
    }

  return -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_base64url_encode
 *
 * Description:
 *   Encode binary data as unpadded base64url (RFC 4648 section 5).
 *
 ****************************************************************************/

int passwd_base64url_encode(FAR const uint8_t *in, size_t inlen,
                            FAR char *out, size_t outlen)
{
  uint32_t acc = 0;
  size_t i;
  size_t o = 0;
  int bits = 0;

  for (i = 0; i < inlen; i++)
    {
      acc = (acc << 8) | in[i];
      bits += 8;

      while (bits >= 6)
        {
          if (o + 1 >= outlen)
            {
              return -E2BIG;
            }

          bits -= 6;
          out[o++] = g_base64url[(acc >> bits) & 0x3f];
        }
    }

  if (bits > 0)
    {
      if (o + 1 >= outlen)
        {
          return -E2BIG;
        }

      out[o++] = g_base64url[(acc << (6 - bits)) & 0x3f];
    }

  if (o >= outlen)
    {
      return -E2BIG;
    }

  out[o] = '\0';
  return OK;
}

/****************************************************************************
 * Name: passwd_base64url_decode
 *
 * Description:
 *   Decode unpadded base64url (RFC 4648 section 5).
 *
 ****************************************************************************/

int passwd_base64url_decode(FAR const char *in,
                            FAR uint8_t *out, size_t outmax,
                            FAR size_t *outlen)
{
  uint32_t acc = 0;
  size_t o = 0;
  int bits = 0;
  int v;

  *outlen = 0;

  while (*in != '\0')
    {
      if (*in == '$' || *in == ':')
        {
          break;
        }

      v = passwd_base64url_val(*in++);
      if (v < 0)
        {
          return -EINVAL;
        }

      acc = (acc << 6) | (uint32_t)v;
      bits += 6;

      if (bits >= 8)
        {
          bits -= 8;
          if (o >= outmax)
            {
              return -E2BIG;
            }

          out[o++] = (uint8_t)((acc >> bits) & 0xff);
        }
    }

  if (bits >= 6)
    {
      return -EINVAL;
    }

  *outlen = o;
  return OK;
}
