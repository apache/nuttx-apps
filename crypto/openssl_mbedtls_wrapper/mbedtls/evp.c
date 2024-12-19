/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/evp.c
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

#include <openssl/bn.h>
#include <openssl/cipher.h>
#include <openssl/ssl_dbg.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/types.h>
#include "ssl_port.h"

#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

EVP_PKEY *__EVP_PKEY_new(EVP_PKEY *ipk)
{
  int ret;
  EVP_PKEY *pkey;

  pkey = ssl_mem_zalloc(sizeof(EVP_PKEY));
  if (!pkey)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "no enough memory > (pkey)");
      goto no_mem;
    }

  if (ipk)
    {
      pkey->method = ipk->method;
    }
  else
    {
      pkey->method = EVP_PKEY_method();
    }

  ret = EVP_PKEY_METHOD_CALL(new, pkey, ipk);
  if (ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL,
                "EVP_PKEY_METHOD_CALL(new) return %d", ret);
      goto failed;
    }

  return pkey;

failed:
  ssl_mem_free(pkey);
no_mem:
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int EVP_PKEY_id(const EVP_PKEY *pkey)
{
  const mbedtls_pk_context *mbedtls_pkey = (const mbedtls_pk_context *)pkey;
  return mbedtls_pk_get_type(mbedtls_pkey);
}

RSA *EVP_PKEY_get1_RSA(const EVP_PKEY *pkey)
{
  mbedtls_pk_context *mbedtls_pkey = (mbedtls_pk_context *)pkey;
  if (pkey == NULL || mbedtls_pk_get_type(mbedtls_pkey) != MBEDTLS_PK_RSA)
    {
      return NULL;
    }

  mbedtls_rsa_context *rsa = mbedtls_pk_rsa(* mbedtls_pkey);
  return (RSA *)rsa;
}

void EVP_PKEY_free(EVP_PKEY *pkey)
{
    SSL_ASSERT3(pkey);

    EVP_PKEY_METHOD_CALL(free, pkey);

    ssl_mem_free(pkey);
}

int EVP_PKEY_type(int nid)
{
  return 0;
}

EC_KEY *EVP_PKEY_get1_EC_KEY(const EVP_PKEY *pkey)
{
  return NULL;
}

EVP_PKEY *EVP_PKEY_new(void)
{
  return __EVP_PKEY_new(NULL);
}

int EVP_PKEY_set1_RSA(EVP_PKEY *pkey, RSA *key)
{
  return 0;
}

void EVP_MD_CTX_init(EVP_MD_CTX *ctx)
{
  mbedtls_md_init((mbedtls_md_context_t *)ctx);
}

EVP_PKEY *EVP_PKEY_new_raw_private_key(int type, ENGINE *unused,
                                       const uint8_t *in, size_t len)
{
  return NULL;
}

const EC_GROUP *EC_KEY_get0_group(const EC_KEY *key)
{
  return NULL;
}

EC_KEY *EC_KEY_new(void)
{
  return NULL;
}

void EC_GROUP_set_point_conversion_form(EC_GROUP *group,
                                        point_conversion_form_t form)
{
}

void EC_GROUP_set_asn1_flag(EC_GROUP *group, int flag)
{
}

int EC_KEY_set_group(EC_KEY *key, const EC_GROUP *group)
{
  return 0;
}

int EC_KEY_generate_key(EC_KEY *key)
{
  return 0;
}

BIGNUM *BN_new(void)
{
  return NULL;
}

void BN_free(BIGNUM *bn)
{
}

int BN_set_word(BIGNUM *bn, BN_ULONG value)
{
  return 0;
}

BN_ULONG BN_get_word(const BIGNUM *bn)
{
  return 0;
}

int EVP_DigestVerifyFinal(EVP_MD_CTX *ctx, const uint8_t *sig,
                          size_t sig_len)
{
  return 0;
}

int EVP_DigestVerifyUpdate(EVP_MD_CTX *ctx, const void *data, size_t len)
{
  return 0;
}

int EVP_DigestVerifyInit(EVP_MD_CTX *ctx, EVP_PKEY_CTX **pctx,
                         const EVP_MD *type, ENGINE *e, EVP_PKEY *pkey)
{
  return 0;
}

int EVP_DigestSignFinal(EVP_MD_CTX *ctx, uint8_t *out_sig,
                        size_t *out_sig_len)
{
  return 0;
}

int i2d_PUBKEY(const EVP_PKEY *pkey, uint8_t **outp)
{
  return 0;
}

EVP_PKEY *d2i_PrivateKey(int type, EVP_PKEY **out,
                         const uint8_t **inp, long len)
{
  return NULL;
}

int EVP_PKEY_set1_EC_KEY(EVP_PKEY *pkey, EC_KEY *key)
{
  return 0;
}

int EVP_PKEY_bits(const EVP_PKEY *pkey)
{
  return 0;
}

int EVP_DigestSignInit(EVP_MD_CTX *ctx, EVP_PKEY_CTX **pctx,
                       const EVP_MD *type, ENGINE *e, EVP_PKEY *pkey)
{
  return 0;
}

int EVP_DigestSignUpdate(EVP_MD_CTX *ctx, const void *data, size_t len)
{
  return 0;
}

int EVP_marshal_private_key(CBB *cbb, const EVP_PKEY *key)
{
  return 0;
}

int EVP_DigestSign(EVP_MD_CTX *ctx, uint8_t *out_sig, size_t *out_sig_len,
                   const uint8_t *data, size_t data_len)
{
  return 0;
}

int EVP_PKEY_size(const EVP_PKEY *pkey)
{
  return 0;
}

int EVP_PKEY_CTX_set_rsa_oaep_md(EVP_PKEY_CTX *ctx, const EVP_MD *md)
{
  return 0;
}

int EVP_PKEY_CTX_set_rsa_padding(EVP_PKEY_CTX *ctx, int padding)
{
  return 0;
}

int EVP_PKEY_CTX_set_rsa_pss_saltlen(EVP_PKEY_CTX *ctx, int salt_len)
{
  return 0;
}

int EVP_PKEY_CTX_set_rsa_mgf1_md(EVP_PKEY_CTX *ctx, const EVP_MD *md)
{
  return 0;
}

EVP_PKEY_CTX *EVP_PKEY_CTX_new(EVP_PKEY *pkey, ENGINE *e)
{
  return NULL;
}

int EVP_PKEY_encrypt_init(EVP_PKEY_CTX *ctx)
{
  return 0;
}

int EVP_PKEY_encrypt(EVP_PKEY_CTX *ctx, uint8_t *out, size_t *out_len,
                     const uint8_t *in, size_t in_len)
{
  return 0;
}

int EVP_PKEY_decrypt_init(EVP_PKEY_CTX *ctx)
{
  return 0;
}

int EVP_PKEY_decrypt(EVP_PKEY_CTX *ctx, uint8_t *out, size_t *out_len,
                     const uint8_t *in, size_t in_len)
{
  return 0;
}

void EVP_PKEY_CTX_free(EVP_PKEY_CTX *ctx)
{
}

int i2d_PrivateKey(const EVP_PKEY *key, uint8_t **outp)
{
  return 0;
}

int EVP_PKEY_derive_init(EVP_PKEY_CTX *ctx)
{
  return 0;
}

int EVP_PKEY_derive_set_peer(EVP_PKEY_CTX *ctx, EVP_PKEY *peer)
{
  return 0;
}

int EVP_PKEY_derive(EVP_PKEY_CTX *ctx, uint8_t *key, size_t *out_key_len)
{
  return 0;
}

int EVP_PKEY_get_raw_public_key(const EVP_PKEY *pkey,
                                uint8_t *out, size_t *out_len)
{
  return 0;
}

EVP_PKEY *d2i_PUBKEY(EVP_PKEY **out, const uint8_t **inp, long len)
{
  return NULL;
}

int EVP_PKEY_get_raw_private_key(const EVP_PKEY *pkey,
                                 uint8_t *out, size_t *out_len)
{
  return 0;
}
