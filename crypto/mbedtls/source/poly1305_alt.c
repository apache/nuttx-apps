/****************************************************************************
 * apps/crypto/mbedtls/source/poly1305_alt.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "mbedtls/error.h"
#include "mbedtls/poly1305.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void mbedtls_poly1305_init(FAR mbedtls_poly1305_context *ctx)
{
  cryptodev_init(ctx);
}

void mbedtls_poly1305_free(FAR mbedtls_poly1305_context *ctx)
{
  cryptodev_free(ctx);
}

int mbedtls_poly1305_starts(FAR mbedtls_poly1305_context *ctx,
                            const unsigned char key[32])
{
  ctx->session.mac = CRYPTO_POLY1305;
  ctx->session.mackey = (caddr_t)key;
  ctx->session.mackeylen = 32;
  return cryptodev_get_session(ctx);
}

int mbedtls_poly1305_update(FAR mbedtls_poly1305_context *ctx,
                            FAR const unsigned char *input,
                            size_t ilen)
{
  ctx->crypt.op = COP_ENCRYPT;
  ctx->crypt.flags |= COP_FLAG_UPDATE;
  ctx->crypt.src = (caddr_t)input;
  ctx->crypt.len = ilen;
  return cryptodev_crypt(ctx);
}

int mbedtls_poly1305_finish(FAR mbedtls_poly1305_context *ctx,
                            unsigned char mac[16])
{
  int ret;

  ctx->crypt.op = COP_ENCRYPT;
  ctx->crypt.flags = 0;
  ctx->crypt.mac = (caddr_t)mac;
  ret = cryptodev_crypt(ctx);
  cryptodev_free_session(ctx);
  return ret;
}

int mbedtls_poly1305_mac(const unsigned char key[32],
                         FAR const unsigned char *input,
                         size_t ilen,
                         unsigned char mac[16])
{
  mbedtls_poly1305_context ctx;
  int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

  mbedtls_poly1305_init(&ctx);

  ret = mbedtls_poly1305_starts(&ctx, key);
  if (ret != 0)
    {
      goto cleanup;
    }

  ret = mbedtls_poly1305_update(&ctx, input, ilen);
  if (ret != 0)
    {
      goto cleanup;
    }

  ret = mbedtls_poly1305_finish(&ctx, mac);

cleanup:
  mbedtls_poly1305_free(&ctx);
  return ret;
}
