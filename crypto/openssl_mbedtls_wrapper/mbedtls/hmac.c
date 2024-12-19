/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/hmac.c
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
#include <openssl/hmac.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/md.h>
#include <stdlib.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uint8_t *HMAC(const EVP_MD *evp_md, const void *key,
              size_t key_len, const uint8_t *data,
              size_t data_len, uint8_t *out,
              unsigned int *out_len)
{
  if (mbedtls_md_hmac((const mbedtls_md_info_t *)evp_md,
                      (const unsigned char *)key, key_len, data, data_len,
                      out) != 0)
    {
      return NULL;
    }

  if (out_len != NULL)
    {
      *out_len = mbedtls_md_get_size((const mbedtls_md_info_t *)evp_md);
    }

  return out;
}

HMAC_CTX *HMAC_CTX_new(void)
{
  mbedtls_md_context_t *ctx =
      (mbedtls_md_context_t *)malloc(sizeof(mbedtls_md_context_t));
  if (ctx != NULL)
    {
      mbedtls_md_init(ctx);
    }

  return (HMAC_CTX *)ctx;
}

void HMAC_CTX_init(HMAC_CTX *ctx)
{
  mbedtls_md_init((mbedtls_md_context_t *)ctx);
}

int HMAC_Init_ex(HMAC_CTX *ctx, const void *key, size_t key_len,
                 const EVP_MD *md, ENGINE *impl)
{
  int ret = mbedtls_md_setup((mbedtls_md_context_t *)ctx,
                             (const mbedtls_md_info_t *)md, 1);
  if (ret != 0)
    {
      goto error;
    }

  ret = mbedtls_md_hmac_starts((mbedtls_md_context_t *)ctx,
                               (const unsigned char *)key, key_len);
  if (ret != 0)
    {
      goto error;
    }

  return 1;
error:
  errno = ret;
  return 0;
}

int HMAC_Update(HMAC_CTX *ctx, const uint8_t *data, size_t data_len)
{
  errno = mbedtls_md_hmac_update((mbedtls_md_context_t *)ctx,
                                 data, data_len);
  return !errno;
}

int HMAC_Final(HMAC_CTX *ctx, uint8_t *out, unsigned int *out_len)
{
  size_t md_size;

  const mbedtls_md_info_t *md_info =
    ((mbedtls_md_context_t *)(ctx))->md_info;
  if (md_info == NULL)
    {
      return 0;
    }

  md_size = mbedtls_md_get_size(md_info);
  *out_len = md_size;
  errno = mbedtls_md_hmac_finish((mbedtls_md_context_t *)ctx, out);
  return !errno;
}

void HMAC_CTX_cleanup(HMAC_CTX *ctx)
{
  mbedtls_md_free((mbedtls_md_context_t *)ctx);
}

void HMAC_CTX_free(HMAC_CTX *ctx)
{
  mbedtls_md_free((mbedtls_md_context_t *)ctx);
  free(ctx);
}

size_t EVP_MD_size(const EVP_MD *md)
{
  return mbedtls_md_get_size((const mbedtls_md_info_t *)md);
}

const EVP_MD *EVP_sha1(void)
{
  const mbedtls_md_info_t *md_info =
    mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  return (const EVP_MD *)md_info;
}

const EVP_MD *EVP_sha224(void)
{
  const mbedtls_md_info_t *md_info =
    mbedtls_md_info_from_type(MBEDTLS_MD_SHA224);
  return (const EVP_MD *)md_info;
}

const EVP_MD *EVP_sha256(void)
{
  const mbedtls_md_info_t *md_info =
    mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  return (const EVP_MD *)md_info;
}

const EVP_MD *EVP_sha384(void)
{
  const mbedtls_md_info_t *md_info =
    mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
  return (const EVP_MD *)md_info;
}

const EVP_MD *EVP_sha512(void)
{
  const mbedtls_md_info_t *md_info =
    mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
  return (const EVP_MD *)md_info;
}
