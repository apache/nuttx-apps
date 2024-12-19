/****************************************************************************
 * apps/crypto/mbedtls/source/aes_alt.c
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

#include "mbedtls/aes.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ECB_BLOCK_SIZE    16
#define NONCE_LENGTH      4

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void mbedtls_aes_init(FAR mbedtls_aes_context *ctx)
{
  cryptodev_init(&ctx->dev);
}

void mbedtls_aes_free(FAR mbedtls_aes_context *ctx)
{
  cryptodev_free(&ctx->dev);
}

int mbedtls_aes_setkey_enc(FAR mbedtls_aes_context *ctx,
                           FAR const unsigned char *key,
                           unsigned int keybits)
{
  switch (keybits)
    {
      case 128:
        break;
      case 192:
        break;
      case 256:
        break;
      default:
        return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
    }

  memcpy(ctx->key, key, keybits / 8);
  ctx->dev.session.key = (caddr_t)ctx->key;
  ctx->dev.session.keylen = keybits / 8;
  return 0;
}

int mbedtls_aes_setkey_dec(FAR mbedtls_aes_context *ctx,
                           FAR const unsigned char *key,
                           unsigned int keybits)
{
  return mbedtls_aes_setkey_enc(ctx, key, keybits);
}

/* AES-ECB block encryption/decryption */

int mbedtls_aes_crypt_ecb(FAR mbedtls_aes_context *ctx,
                          int mode,
                          const unsigned char input[16],
                          unsigned char output[16])
{
  int ret;
  unsigned char iv[16];

  if (mode != MBEDTLS_AES_ENCRYPT && mode != MBEDTLS_AES_DECRYPT)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  ctx->dev.session.cipher = CRYPTO_AES_CBC;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  memset(iv, 0, 16);
  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = mode == MBEDTLS_AES_ENCRYPT ?
                              COP_ENCRYPT : COP_DECRYPT;
  ctx->dev.crypt.len = ECB_BLOCK_SIZE;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)iv;
  ret = cryptodev_crypt(&ctx->dev);
  cryptodev_free_session(&ctx->dev);
  return ret;
}

#if defined(MBEDTLS_CIPHER_MODE_CBC)

/* AES-CBC buffer encryption/decryption */

int mbedtls_aes_crypt_cbc(mbedtls_aes_context *ctx,
                          int mode,
                          size_t length,
                          unsigned char iv[16],
                          const unsigned char *input,
                          unsigned char *output)
{
  int ret;

  if (mode != MBEDTLS_AES_ENCRYPT && mode != MBEDTLS_AES_DECRYPT)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  if ((length % 16) != 0)
    {
      return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

  ctx->dev.session.cipher = CRYPTO_AES_CBC;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = mode == MBEDTLS_AES_ENCRYPT ?
                              COP_ENCRYPT : COP_DECRYPT;
  ctx->dev.crypt.len = length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)iv;
  ret = cryptodev_crypt(&ctx->dev);
  cryptodev_free_session(&ctx->dev);
  return ret;
}
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CTR)

/* AES-CTR buffer encryption/decryption */

int mbedtls_aes_crypt_ctr(FAR mbedtls_aes_context *ctx,
                          size_t length,
                          FAR size_t *nc_off,
                          unsigned char nonce_counter[16],
                          unsigned char stream_block[16],
                          FAR const unsigned char *input,
                          FAR unsigned char *output)
{
  int ret;

  if (*nc_off > 0x0f)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  ctx->dev.session.cipher = CRYPTO_AES_CTR;
  memcpy(ctx->key + ctx->dev.session.keylen,
         nonce_counter, NONCE_LENGTH);
  ctx->dev.session.keylen += NONCE_LENGTH;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = COP_ENCRYPT;
  ctx->dev.crypt.len = length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)nonce_counter + NONCE_LENGTH;
  ret = cryptodev_crypt(&ctx->dev);
  if (ret == 0)
    {
      *nc_off = length % ECB_BLOCK_SIZE;
    }

  cryptodev_free_session(&ctx->dev);
  return ret;
}
#endif /* MBEDTLS_CIPHER_MODE_CTR */

#if defined(MBEDTLS_CIPHER_MODE_XTS)
void mbedtls_aes_xts_init(FAR mbedtls_aes_xts_context *ctx)
{
  mbedtls_aes_init(ctx);
}

void mbedtls_aes_xts_free(FAR mbedtls_aes_xts_context *ctx)
{
  mbedtls_aes_free(ctx);
}

int mbedtls_aes_xts_setkey_enc(FAR mbedtls_aes_xts_context *ctx,
                               FAR const unsigned char *key,
                               unsigned int keybits)
{
  if (keybits != 256 && keybits != 512)
    {
      return MBEDTLS_ERR_AES_INVALID_KEY_LENGTH;
    }

  memcpy(ctx->key, key, keybits / 8);
  ctx->dev.session.key = (caddr_t)ctx->key;
  ctx->dev.session.keylen = keybits / 8;
  return 0;
}

int mbedtls_aes_xts_setkey_dec(FAR mbedtls_aes_xts_context *ctx,
                               FAR const unsigned char *key,
                               unsigned int keybits)
{
  return mbedtls_aes_xts_setkey_enc(ctx, key, keybits);
}

int mbedtls_aes_crypt_xts(FAR mbedtls_aes_xts_context *ctx,
                          int mode,
                          size_t length,
                          const unsigned char data_unit[16],
                          FAR const unsigned char *input,
                          FAR unsigned char *output)
{
  int ret;
  unsigned char iv[16];

  if (mode != MBEDTLS_AES_ENCRYPT && mode != MBEDTLS_AES_DECRYPT)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  /* Data units must be at least 16 bytes long. */

  if (length < 16)
    {
      return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

  /* NIST SP 800-38E disallows data units larger than 2**20 blocks. */

  if (length > (1 << 20) * 16)
    {
      return MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH;
    }

  ctx->dev.session.cipher = CRYPTO_AES_XTS;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  memcpy(iv, data_unit, 16);
  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = mode == MBEDTLS_AES_ENCRYPT ?
                              COP_ENCRYPT : COP_DECRYPT;
  ctx->dev.crypt.len = length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)iv;
  ret = cryptodev_crypt(&ctx->dev);
  cryptodev_free_session(&ctx->dev);
  return ret;
}
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#if defined(MBEDTLS_CIPHER_MODE_CFB)

/* AES-CFB128 buffer encryption/decryption */

int mbedtls_aes_crypt_cfb128(FAR mbedtls_aes_context *ctx,
                             int mode,
                             size_t length,
                             size_t *iv_off,
                             unsigned char iv[16],
                             const unsigned char *input,
                             unsigned char *output)
{
  int ret;

  if (mode != MBEDTLS_AES_ENCRYPT && mode != MBEDTLS_AES_DECRYPT)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  if (*iv_off > 15)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  ctx->dev.session.cipher = CRYPTO_AES_CFB_128;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = mode == MBEDTLS_AES_ENCRYPT ?
                              COP_ENCRYPT : COP_DECRYPT;
  ctx->dev.crypt.len = length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)iv;
  ret = cryptodev_crypt(&ctx->dev);
  if (ret == 0)
    {
      *iv_off = length % ECB_BLOCK_SIZE;
    }

  cryptodev_free_session(&ctx->dev);
  return ret;
}

/* AES-CFB8 buffer encryption/decryption */

int mbedtls_aes_crypt_cfb8(FAR mbedtls_aes_context *ctx,
                           int mode,
                           size_t length,
                           unsigned char iv[16],
                           const unsigned char *input,
                           unsigned char *output)
{
  int ret;

  if (mode != MBEDTLS_AES_ENCRYPT && mode != MBEDTLS_AES_DECRYPT)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  ctx->dev.session.cipher = CRYPTO_AES_CFB_8;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = mode == MBEDTLS_AES_ENCRYPT ?
                              COP_ENCRYPT : COP_DECRYPT;
  ctx->dev.crypt.len = length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)iv;
  ret = cryptodev_crypt(&ctx->dev);
  cryptodev_free_session(&ctx->dev);
  return ret;
}
#endif /* MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_OFB)

/* AES-OFB (Output Feedback Mode) buffer encryption/decryption */

int mbedtls_aes_crypt_ofb(FAR mbedtls_aes_context *ctx,
                          size_t length,
                          size_t *iv_off,
                          unsigned char iv[16],
                          const unsigned char *input,
                          unsigned char *output)
{
  int ret;

  if (*iv_off > 15)
    {
      return MBEDTLS_ERR_AES_BAD_INPUT_DATA;
    }

  ctx->dev.session.cipher = CRYPTO_AES_OFB;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return ret;
    }

  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = COP_ENCRYPT;
  ctx->dev.crypt.len = length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.iv = (caddr_t)iv;
  ret = cryptodev_crypt(&ctx->dev);
  if (ret == 0)
    {
      *iv_off = length % ECB_BLOCK_SIZE;
    }

  cryptodev_free_session(&ctx->dev);
  return ret;
}
#endif /* MBEDTLS_CIPHER_MODE_OFB */
