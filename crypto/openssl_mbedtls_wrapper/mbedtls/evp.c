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
#include <mbedtls/platform.h>
#include <mbedtls/rsa.h>

#include "ssl_pm.h"

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
  struct pkey_pm *pm;
  mbedtls_pk_context *pk;
  mbedtls_ecp_keypair *ecp;
  unsigned char buf[MBEDTLS_ECP_MAX_PT_LEN];
  size_t olen;
  int ret;

  if (pkey == NULL || out_len == NULL)
    {
      return 0;
    }

  pm = (struct pkey_pm *)pkey->pkey_pm;
  if (pm == NULL || pm->pkey == NULL)
    {
      return 0;
    }

  pk = pm->pkey;
  if (!mbedtls_pk_can_do(pk, MBEDTLS_PK_ECKEY))
    {
      return 0;
    }

  /* Raw public key is the X9.62 uncompressed point, not the SPKI DER. */

  ecp = mbedtls_pk_ec(*pk);
  ret = mbedtls_ecp_point_write_binary(&ecp->MBEDTLS_PRIVATE(grp),
                                       &ecp->MBEDTLS_PRIVATE(Q),
                                       MBEDTLS_ECP_PF_UNCOMPRESSED,
                                       &olen, buf, sizeof(buf));
  if (ret != 0)
    {
      return 0;
    }

  if (out == NULL)
    {
      *out_len = olen;
      return 1;
    }

  if (*out_len < olen)
    {
      *out_len = olen;
      return 0;
    }

  memcpy(out, buf, olen);
  *out_len = olen;
  return 1;
}

EVP_PKEY *d2i_PUBKEY(EVP_PKEY **out, const uint8_t **inp, long len)
{
  return NULL;
}

int EVP_PKEY_get_raw_private_key(const EVP_PKEY *pkey,
                                 uint8_t *out, size_t *out_len)
{
  struct pkey_pm *pm;
  mbedtls_pk_context *pk;
  mbedtls_ecp_keypair *ecp;
  size_t need;
  int ret;

  if (pkey == NULL || out_len == NULL)
    {
      return 0;
    }

  pm = (struct pkey_pm *)pkey->pkey_pm;
  if (pm == NULL || pm->pkey == NULL)
    {
      return 0;
    }

  pk = pm->pkey;
  if (!mbedtls_pk_can_do(pk, MBEDTLS_PK_ECKEY))
    {
      return 0;
    }

  /* Raw private key is the fixed-length big-endian scalar. */

  ecp = mbedtls_pk_ec(*pk);
  need = (ecp->MBEDTLS_PRIVATE(grp).pbits + 7) / 8;

  if (out == NULL)
    {
      *out_len = need;
      return 1;
    }

  if (*out_len < need)
    {
      *out_len = need;
      return 0;
    }

  ret = mbedtls_mpi_write_binary(&ecp->MBEDTLS_PRIVATE(d), out, need);
  if (ret != 0)
    {
      return 0;
    }

  *out_len = need;
  return 1;
}

int EVP_PKEY_eq(const EVP_PKEY *a, const EVP_PKEY *b)
{
  unsigned char *buf_a = NULL;
  unsigned char *buf_b = NULL;
  struct pkey_pm *pm_a;
  struct pkey_pm *pm_b;
  size_t buf_size;
  int len_a;
  int len_b;
  int result;

  if (a == NULL || b == NULL)
    {
      return -2;
    }

  pm_a = (struct pkey_pm *)a->pkey_pm;
  pm_b = (struct pkey_pm *)b->pkey_pm;

  if (pm_a == NULL || pm_a->pkey == NULL ||
      pm_b == NULL || pm_b->pkey == NULL)
    {
      return -1;
    }

  /* Size the DER buffer from the key length so large keys (e.g. RSA-4096)
   * fit while small keys do not over-allocate.  The public key DER adds
   * only a small SubjectPublicKeyInfo overhead on top of the key material.
   */

  buf_size = mbedtls_pk_get_len(pm_a->pkey);
  if (mbedtls_pk_get_len(pm_b->pkey) > buf_size)
    {
      buf_size = mbedtls_pk_get_len(pm_b->pkey);
    }

  buf_size = buf_size * 2 + 128;

  buf_a = mbedtls_calloc(1, buf_size);
  buf_b = mbedtls_calloc(1, buf_size);
  if (buf_a == NULL || buf_b == NULL)
    {
      result = -1;
      goto out;
    }

  len_a = mbedtls_pk_write_pubkey_der(pm_a->pkey, buf_a, buf_size);
  len_b = mbedtls_pk_write_pubkey_der(pm_b->pkey, buf_b, buf_size);
  if (len_a < 0 || len_b < 0)
    {
      result = -1;
      goto out;
    }

  if (len_a != len_b)
    {
      result = 0;
      goto out;
    }

  /* mbedtls writes DER from the end of the buffer */

  result = memcmp(buf_a + buf_size - len_a,
                  buf_b + buf_size - len_b, len_a) == 0 ? 1 : 0;

out:
  mbedtls_free(buf_a);
  mbedtls_free(buf_b);
  return result;
}

int OpenSSL_add_all_algorithms(void)
{
  return 0;
}

void EVP_cleanup(void)
{
}
