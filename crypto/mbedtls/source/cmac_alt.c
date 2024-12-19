/****************************************************************************
 * apps/crypto/mbedtls/source/cmac_alt.c
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

#include "mbedtls/cmac.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"

#include <string.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_cipher_cmac_starts(FAR mbedtls_cipher_context_t *ctx,
                               FAR const unsigned char *key,
                               size_t keybits)
{
  FAR mbedtls_cmac_context_t *cmac_ctx;
  uint32_t cipher_type;
  uint32_t mac_type;
  int retval;

  if (ctx == NULL || ctx->cipher_info == NULL || key == NULL)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  switch (ctx->cipher_info->type)
    {
      case MBEDTLS_CIPHER_AES_128_ECB:
          cipher_type = CRYPTO_AES_CMAC;
          mac_type = CRYPTO_AES_128_CMAC;
          break;
      default:
          return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  cmac_ctx = mbedtls_calloc(1, sizeof(mbedtls_cmac_context_t));
  if (cmac_ctx == NULL)
    {
      return MBEDTLS_ERR_CIPHER_ALLOC_FAILED;
    }

  retval = cryptodev_init(&cmac_ctx->dev);
  if (retval != 0)
    {
      mbedtls_free(cmac_ctx);
      return MBEDTLS_ERR_CIPHER_ALLOC_FAILED;
    }

  cmac_ctx->cipher_type = cipher_type;
  cmac_ctx->mac_type = mac_type;
  cmac_ctx->keybits = keybits;
  memcpy(cmac_ctx->key, key, keybits / 8);
  cmac_ctx->dev.session.cipher = cipher_type;
  cmac_ctx->dev.session.key = (caddr_t)cmac_ctx->key;
  cmac_ctx->dev.session.keylen = keybits / 8;
  cmac_ctx->dev.session.mac = mac_type;
  cmac_ctx->dev.session.mackey = (caddr_t)cmac_ctx->key;
  cmac_ctx->dev.session.mackeylen = keybits / 8;

  retval = cryptodev_get_session(&cmac_ctx->dev);
  if (retval != 0)
    {
      cryptodev_free(&cmac_ctx->dev);
      mbedtls_free(cmac_ctx);
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  ctx->cmac_ctx = cmac_ctx;
  return retval;
}

int mbedtls_cipher_cmac_update(FAR mbedtls_cipher_context_t *ctx,
                               FAR const unsigned char *input,
                               size_t ilen)
{
  if (ctx == NULL || ctx->cmac_ctx == NULL || input == NULL || ilen < 0)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  ctx->cmac_ctx->dev.crypt.op = COP_ENCRYPT;
  ctx->cmac_ctx->dev.crypt.flags |= COP_FLAG_UPDATE;
  ctx->cmac_ctx->dev.crypt.src = (caddr_t)input;
  ctx->cmac_ctx->dev.crypt.len = ilen;
  return cryptodev_crypt(&ctx->cmac_ctx->dev);
}

int mbedtls_cipher_cmac_finish(FAR mbedtls_cipher_context_t *ctx,
                               FAR unsigned char *output)
{
  int ret;

  if (ctx == NULL || ctx->cmac_ctx == NULL || output == NULL)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  ctx->cmac_ctx->dev.crypt.flags = 0;
  ctx->cmac_ctx->dev.crypt.mac = (caddr_t)output;
  ret = cryptodev_crypt(&ctx->cmac_ctx->dev);
  cryptodev_free_session(&ctx->cmac_ctx->dev);
  cryptodev_free(&ctx->cmac_ctx->dev);
  return ret;
}

int mbedtls_cipher_cmac_reset(FAR mbedtls_cipher_context_t *ctx)
{
  FAR mbedtls_cmac_context_t *cmac_ctx;
  int ret;

  if (ctx == NULL || ctx->cmac_ctx == NULL)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  cmac_ctx = ctx->cmac_ctx;
  ret = cryptodev_init(&cmac_ctx->dev);
  if (ret != 0)
    {
      return MBEDTLS_ERR_CIPHER_ALLOC_FAILED;
    }

  cmac_ctx->dev.session.cipher = cmac_ctx->cipher_type;
  cmac_ctx->dev.session.key = (caddr_t)cmac_ctx->key;
  cmac_ctx->dev.session.keylen = cmac_ctx->keybits / 8;
  cmac_ctx->dev.session.mac = cmac_ctx->mac_type;
  cmac_ctx->dev.session.mackey = (caddr_t)cmac_ctx->key;
  cmac_ctx->dev.session.mackeylen = cmac_ctx->keybits / 8;

  ret = cryptodev_get_session(&cmac_ctx->dev);
  if (ret != 0)
    {
      cryptodev_free(&cmac_ctx->dev);
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  return ret;
}

int mbedtls_cipher_cmac(FAR const mbedtls_cipher_info_t *cipher_info,
                        FAR const unsigned char *key, size_t keylen,
                        FAR const unsigned char *input, size_t ilen,
                        FAR unsigned char *output)
{
  FAR mbedtls_cipher_context_t ctx;
  int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

  if (cipher_info == NULL || key == NULL || input == NULL || output == NULL)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  mbedtls_cipher_init(&ctx);

  if ((ret = mbedtls_cipher_setup(&ctx, cipher_info)) != 0)
    {
      goto exit;
    }

  ret = mbedtls_cipher_cmac_starts(&ctx, key, keylen);
  if (ret != 0)
    {
      goto exit;
    }

  ret = mbedtls_cipher_cmac_update(&ctx, input, ilen);
  if (ret != 0)
    {
      goto exit;
    }

  ret = mbedtls_cipher_cmac_finish(&ctx, output);

exit:
  mbedtls_cipher_free(&ctx);
  return ret;
}

#if defined(MBEDTLS_AES_C)
int mbedtls_aes_cmac_prf_128(FAR const unsigned char *key, size_t key_length,
                             FAR const unsigned char *input, size_t in_len,
                             unsigned char output[16])
{
  int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
  FAR const mbedtls_cipher_info_t *cipher_info;
  unsigned char zero_key[MBEDTLS_AES_BLOCK_SIZE];
  unsigned char int_key[MBEDTLS_AES_BLOCK_SIZE];

  if (key == NULL || input == NULL || output == NULL)
    {
      return MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA;
    }

  cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
  if (cipher_info == NULL)
    {
      ret = MBEDTLS_ERR_CIPHER_FEATURE_UNAVAILABLE;
      goto exit;
    }

  if (key_length == MBEDTLS_AES_BLOCK_SIZE)
    {
      memcpy(int_key, key, MBEDTLS_AES_BLOCK_SIZE);
    }
  else
    {
      memset(zero_key, 0, MBEDTLS_AES_BLOCK_SIZE);

      ret = mbedtls_cipher_cmac(cipher_info, zero_key, 128, key,
                                key_length, int_key);
      if (ret != 0)
        {
          goto exit;
        }
    }

  ret = mbedtls_cipher_cmac(cipher_info, int_key, 128, input, in_len,
                            output);

exit:
  mbedtls_platform_zeroize(int_key, sizeof(int_key));
  return ret;
}
#endif /* MBEDTLS_AES_C */
