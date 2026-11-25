/****************************************************************************
 * apps/crypto/mbedtls/source/gcm_alt.c
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

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <crypto/cryptodev.h>

#include "mbedtls/error.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/gcm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* NIST SP 800-38D 5.2.1.1 limits in bytes: len(A) < 2^64 bits and
 * len(P) <= 2^39 - 256 bits.
 */

#define GCM_AAD_MAX   ((UINT64_C(1) << 61) - 1)
#define GCM_INPUT_MAX ((UINT64_C(1) << 36) - 32)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int gcm_select_mac(unsigned int keybits)
{
  switch (keybits)
    {
      case 128:
        return CRYPTO_AES_128_GMAC;
      case 192:
        return CRYPTO_AES_192_GMAC;
      case 256:
        return CRYPTO_AES_256_GMAC;
    }

  return -EINVAL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void mbedtls_gcm_init(FAR mbedtls_gcm_context *ctx)
{
  if (ctx == NULL)
    {
      return;
    }

  memset(ctx, 0, sizeof(mbedtls_gcm_context));
  cryptodev_init(&ctx->dev);
}

void mbedtls_gcm_free(FAR mbedtls_gcm_context *ctx)
{
  if (ctx == NULL)
    {
      return;
    }

  cryptodev_free(&ctx->dev);
  mbedtls_platform_zeroize(ctx, sizeof(mbedtls_gcm_context));
}

int mbedtls_gcm_setkey(FAR mbedtls_gcm_context *ctx,
                       mbedtls_cipher_id_t cipher,
                       FAR const unsigned char *key,
                       unsigned int keybits)
{
  if (ctx == NULL || key == NULL)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (cipher != MBEDTLS_CIPHER_ID_AES)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (keybits != 128 && keybits != 192 && keybits != 256)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  memcpy(ctx->key, key, keybits / 8);
  ctx->keybits = keybits;
  return 0;
}

int mbedtls_gcm_starts(FAR mbedtls_gcm_context *ctx, int mode,
                       FAR const unsigned char *iv, size_t iv_len)
{
  int ret;

  if (ctx == NULL || iv == NULL)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (mode != MBEDTLS_GCM_ENCRYPT && mode != MBEDTLS_GCM_DECRYPT)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (iv_len == 0 || iv_len > GCM_IV_MAX_SIZE)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  memset(&ctx->dev.session, 0, sizeof(ctx->dev.session));
  ctx->dev.session.cipher = CRYPTO_AES_GCM_16;
  ctx->dev.session.mac = gcm_select_mac(ctx->keybits);
  ctx->dev.session.key = (caddr_t)ctx->key;
  ctx->dev.session.keylen = ctx->keybits / 8;
  ctx->dev.session.mackey = (caddr_t)ctx->key;
  ctx->dev.session.mackeylen = ctx->keybits / 8;
  ret = cryptodev_get_session(&ctx->dev);
  if (ret != 0)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  memset(&ctx->dev.crypt, 0, sizeof(ctx->dev.crypt));
  ctx->dev.crypt.ses = ctx->dev.session.ses;
  ctx->dev.crypt.op = mode == MBEDTLS_GCM_ENCRYPT ?
                      COP_ENCRYPT : COP_DECRYPT;

  /* Copy IV into the context to match mbedTLS API expectations. */

  memcpy(ctx->iv, iv, iv_len);
  ctx->ivlen = iv_len;
  ctx->aadlen = 0;
  ctx->len = 0;
  ctx->dev.crypt.iv = (caddr_t)ctx->iv;
  ctx->dev.crypt.ivlen = ctx->ivlen;
  return OK;
}

int mbedtls_gcm_update_ad(FAR mbedtls_gcm_context *ctx,
                          FAR const unsigned char *add,
                          size_t add_len)
{
  int ret;

  if (ctx == NULL || (add_len > 0 && add == NULL))
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (add_len > GCM_AAD_MAX - ctx->aadlen)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  ctx->aadlen += add_len;

  ctx->dev.crypt.flags = COP_FLAG_UPDATE_AAD;
  ctx->dev.crypt.aad = (caddr_t)add;
  ctx->dev.crypt.aadlen = add_len;

  /* AAD-only phase */

  ctx->dev.crypt.len = 0;
  ctx->dev.crypt.src = NULL;
  ctx->dev.crypt.dst = NULL;
  ret = cryptodev_crypt(&ctx->dev);
  if (ret != 0)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  /* Clear IV to prevent re-initialization in subsequent calls */

  ctx->dev.crypt.iv = NULL;
  ctx->dev.crypt.ivlen = 0;
  return OK;
}

int mbedtls_gcm_update(FAR mbedtls_gcm_context *ctx,
                       FAR const unsigned char *input, size_t input_length,
                       FAR unsigned char *output, size_t output_size,
                       FAR size_t *output_length)
{
  int ret;

  if (input_length > 0 && (input == NULL || output == NULL))
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (output_size < input_length)
    {
      return MBEDTLS_ERR_GCM_BUFFER_TOO_SMALL;
    }

  if (input_length > GCM_INPUT_MAX - ctx->len)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  ctx->len += input_length;

  if (output_length)
    {
      *output_length = input_length;
    }

  if (input_length == 0)
    {
      return 0;
    }

  ctx->dev.crypt.flags = COP_FLAG_UPDATE;
  ctx->dev.crypt.aad = NULL;
  ctx->dev.crypt.aadlen = 0;
  ctx->dev.crypt.len = input_length;
  ctx->dev.crypt.olen = input_length;
  ctx->dev.crypt.src = (caddr_t)input;
  ctx->dev.crypt.dst = (caddr_t)output;
  ret = cryptodev_crypt(&ctx->dev);
  if (ret != 0)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  /* Clear IV to prevent re-initialization in subsequent calls */

  ctx->dev.crypt.iv = NULL;
  ctx->dev.crypt.ivlen = 0;
  return OK;
}

int mbedtls_gcm_finish(FAR mbedtls_gcm_context *ctx,
                       FAR unsigned char *output, size_t output_size,
                       FAR size_t *output_length,
                       FAR unsigned char *tag, size_t tag_len)
{
  int ret;

  if (ctx == NULL || tag == NULL)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (tag_len > 16)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  if (output_length)
    {
      *output_length = 0;
    }

  /* Finalize: no payload processing, just emit the tag. */

  ctx->dev.crypt.flags = 0;
  ctx->dev.crypt.aad = NULL;
  ctx->dev.crypt.aadlen = 0;
  ctx->dev.crypt.len = 0;
  ctx->dev.crypt.src = NULL;
  ctx->dev.crypt.dst = (caddr_t)output;
  ctx->dev.crypt.mac = (caddr_t)tag;
  ctx->dev.crypt.olen = tag_len;
  ret = cryptodev_crypt(&ctx->dev);
  cryptodev_free_session(&ctx->dev);
  if (ret != 0)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  return 0;
}

int mbedtls_gcm_crypt_and_tag(FAR mbedtls_gcm_context *ctx,
                              int mode,
                              size_t length,
                              FAR const unsigned char *iv,
                              size_t iv_len,
                              FAR const unsigned char *add,
                              size_t add_len,
                              FAR const unsigned char *input,
                              FAR unsigned char *output,
                              size_t tag_len,
                              FAR unsigned char *tag)
{
  int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
  size_t olen;

  if ((ret = mbedtls_gcm_starts(ctx, mode, iv, iv_len)) != 0)
    {
      return ret;
    }

  if ((ret = mbedtls_gcm_update_ad(ctx, add, add_len)) != 0)
    {
      return ret;
    }

  if ((ret = mbedtls_gcm_update(ctx, input, length,
                                output, length, &olen)) != 0)
    {
      return ret;
    }

  if ((ret = mbedtls_gcm_finish(ctx, NULL, 0, &olen, tag, tag_len)) != 0)
    {
      return ret;
    }

  return 0;
}

int mbedtls_gcm_auth_decrypt(FAR mbedtls_gcm_context *ctx,
                             size_t length,
                             FAR const unsigned char *iv,
                             size_t iv_len,
                             FAR const unsigned char *add,
                             size_t add_len,
                             FAR const unsigned char *tag,
                             size_t tag_len,
                             FAR const unsigned char *input,
                             FAR unsigned char *output)
{
  unsigned char check_tag[16];
  int diff = 0;
  int ret;
  size_t i;

  if (tag == NULL)
    {
      return MBEDTLS_ERR_GCM_BAD_INPUT;
    }

  ret = mbedtls_gcm_crypt_and_tag(ctx, MBEDTLS_GCM_DECRYPT, length,
                                 iv, iv_len, add, add_len,
                                 input, output, tag_len, check_tag);
  if (ret != 0)
    {
      return ret;
    }

  for (i = 0; i < tag_len; i++)
    {
      diff |= tag[i] ^ check_tag[i];
    }

  if (diff != 0)
    {
      if (length > 0)
        {
          mbedtls_platform_zeroize(output, length);
        }

      return MBEDTLS_ERR_GCM_AUTH_FAILED;
    }

  return 0;
}
