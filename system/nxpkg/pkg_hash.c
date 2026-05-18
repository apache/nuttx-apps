/****************************************************************************
 * apps/system/nxpkg/pkg_hash.c
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

#include <crypto/sha2.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "pkg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pkg_hex_encode(FAR const uint8_t *input, size_t length,
                           FAR char *output)
{
  static const char g_hex[] = "0123456789abcdef";
  size_t i;

  for (i = 0; i < length; i++)
    {
      output[i * 2] = g_hex[input[i] >> 4];
      output[i * 2 + 1] = g_hex[input[i] & 0x0f];
    }

  output[length * 2] = '\0';
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int pkg_hash_file_sha256(FAR const char *path,
                         FAR char digest[PKG_HASH_HEX_LEN + 1])
{
  SHA2_CTX ctx;
  FAR FILE *stream;
  uint8_t raw[SHA256_DIGEST_LENGTH];
  uint8_t buffer[512];
  size_t nread;

  stream = fopen(path, "rb");
  if (stream == NULL)
    {
      return -errno;
    }

  sha256init(&ctx);

  for (; ; )
    {
      nread = fread(buffer, 1, sizeof(buffer), stream);
      if (nread > 0)
        {
          sha256update(&ctx, buffer, nread);
        }

      if (nread < sizeof(buffer))
        {
          if (ferror(stream))
            {
              fclose(stream);
              return -EIO;
            }

          break;
        }
    }

  fclose(stream);
  sha256final(raw, &ctx);
  pkg_hex_encode(raw, sizeof(raw), digest);
  return 0;
}
