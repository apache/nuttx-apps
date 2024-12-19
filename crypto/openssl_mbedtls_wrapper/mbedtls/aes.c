/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/aes.c
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

#include <errno.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <mbedtls/cipher.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void EVP_CIPHER_CTX_init(EVP_CIPHER_CTX *ctx)
{
  mbedtls_cipher_init((mbedtls_cipher_context_t *)ctx);
}

int EVP_CIPHER_CTX_cleanup(EVP_CIPHER_CTX *ctx)
{
  mbedtls_cipher_free((mbedtls_cipher_context_t *)ctx);
  return 1;
}

const EVP_CIPHER *EVP_aes_128_ecb(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_128_ECB);
}

const EVP_CIPHER *EVP_aes_192_ecb(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_192_ECB);
}

const EVP_CIPHER *EVP_aes_256_ecb(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_256_ECB);
}

const EVP_CIPHER *EVP_aes_128_cbc(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_128_CBC);
}

const EVP_CIPHER *EVP_aes_192_cbc(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_192_CBC);
}

const EVP_CIPHER *EVP_aes_256_cbc(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_256_CBC);
}

const EVP_CIPHER *EVP_aes_128_ctr(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_128_CTR);
}

const EVP_CIPHER *EVP_aes_192_ctr(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_192_CTR);
}

const EVP_CIPHER *EVP_aes_256_ctr(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_256_CTR);
}

const EVP_CIPHER *EVP_aes_128_gcm(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_128_GCM);
}

const EVP_CIPHER *EVP_aes_192_gcm(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_192_GCM);
}

const EVP_CIPHER *EVP_aes_256_gcm(void)
{
  return (const EVP_CIPHER *)mbedtls_cipher_info_from_type(
    MBEDTLS_CIPHER_AES_256_GCM);
}

int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX *c, int pad)
{
  mbedtls_cipher_padding_t padding = MBEDTLS_PADDING_PKCS7;
  if (pad == 0)
    {
      padding = MBEDTLS_PADDING_NONE;
    }

  mbedtls_cipher_set_padding_mode((mbedtls_cipher_context_t *)c, padding);
  return 1;
}

int EVP_CipherInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                      ENGINE *impl, const unsigned char *key,
                      const unsigned char *iv, int enc)
{
  mbedtls_cipher_context_t *ctx_ = (mbedtls_cipher_context_t *)ctx;
  mbedtls_cipher_set_padding_mode(ctx_, MBEDTLS_PADDING_PKCS7);
  const mbedtls_operation_t operation = enc
    ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT;
  int ret = mbedtls_cipher_setup(ctx_,
                                 (mbedtls_cipher_info_t *)cipher);
  if (ret != 0)
    {
      goto error;
    }

  ret = mbedtls_cipher_setkey(ctx_, key,
                              mbedtls_cipher_get_key_bitlen(ctx_),
                              operation);
  if (ret != 0)
    {
      goto error;
    }

  ret = mbedtls_cipher_set_iv(ctx_, iv,
                              mbedtls_cipher_get_iv_size(ctx_));
  if (ret != 0)
    {
      goto error;
    }

  return 1;
error:
  errno = ret;
  return 0;
}

int EVP_CipherUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                     const unsigned char *in, int inl)
{
  size_t out_len = 0;
  int ret = mbedtls_cipher_update((mbedtls_cipher_context_t *)ctx,
                                   in, inl, out, &out_len);
  if (ret != 0)
    {
      errno = ret;
      return 0;
    }

  *outl = out_len;
  return 1;
}

int EVP_CipherFinal_ex(EVP_CIPHER_CTX *ctx,
                       unsigned char *outm, int *outl)
{
  size_t out_len = 0;
  int ret = mbedtls_cipher_finish((mbedtls_cipher_context_t *)ctx,
                                   outm, &out_len);
  if (ret != 0)
    {
      errno = ret;
      return 0;
    }

  *outl = out_len;
  return 1;
}

EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void)
{
  return (EVP_CIPHER_CTX *)(malloc(sizeof(mbedtls_cipher_context_t)));
}

void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx)
{
  if (ctx == NULL)
    return;

  free(ctx);
}
