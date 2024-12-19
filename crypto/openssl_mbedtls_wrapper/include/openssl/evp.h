/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/evp.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_EVP_H
#define OPENSSL_MBEDTLS_WRAPPER_EVP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/cipher.h>
#include <openssl/digest.h>
#include <openssl/ec_key.h>
#include <openssl/mem.h>
#include <openssl/nid.h>
#include <openssl/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EVP_PKEY_RSA NID_rsaEncryption
#define EVP_PKEY_EC NID_X9_62_id_ecPublicKey
#define EVP_PKEY_ED25519 NID_ED25519

#define EVP_PKEY_X25519 NID_X25519

struct evp_pkey_st
{
  void *pkey_pm;
  const PKEY_METHOD *method;
};

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int EVP_PKEY_id(const EVP_PKEY *pkey);

RSA *EVP_PKEY_get1_RSA(const EVP_PKEY *pkey);

int EVP_PKEY_set1_RSA(EVP_PKEY *pkey, RSA *key);

EVP_PKEY *__EVP_PKEY_new(EVP_PKEY *ipk);

EVP_PKEY *EVP_PKEY_new(void);

void EVP_PKEY_free(EVP_PKEY *pkey);

void EVP_PKEY_CTX_free(EVP_PKEY_CTX *ctx);

EC_KEY *EVP_PKEY_get0_EC_KEY(const EVP_PKEY *pkey);

EC_KEY *EVP_PKEY_get1_EC_KEY(const EVP_PKEY *pkey);

int EVP_PKEY_type(int nid);

EVP_PKEY *d2i_PrivateKey(int type, EVP_PKEY **out,
                         const uint8_t **inp, long len);

int i2d_PUBKEY(const EVP_PKEY *pkey, uint8_t **outp);

EVP_PKEY *EVP_PKEY_new_raw_private_key(int type, ENGINE *unused,
                                       const uint8_t *in, size_t len);

int EVP_PKEY_set1_EC_KEY(EVP_PKEY *pkey, EC_KEY *key);

int EVP_PKEY_bits(const EVP_PKEY *pkey);

int EVP_DigestSignInit(EVP_MD_CTX *ctx, EVP_PKEY_CTX **pctx,
                       const EVP_MD *type, ENGINE *e,
                       EVP_PKEY *pkey);

int EVP_DigestSignUpdate(EVP_MD_CTX *ctx, const void *data, size_t len);

int EVP_DigestSignFinal(EVP_MD_CTX *ctx, uint8_t *out_sig,
                        size_t *out_sig_len);

int EVP_DigestSign(EVP_MD_CTX *ctx, uint8_t *out_sig,
                   size_t *out_sig_len, const uint8_t *data,
                   size_t data_len);

int EVP_DigestVerifyInit(EVP_MD_CTX *ctx, EVP_PKEY_CTX **pctx,
                         const EVP_MD *type, ENGINE *e,
                         EVP_PKEY *pkey);

int EVP_DigestVerifyUpdate(EVP_MD_CTX *ctx, const void *data,
                           size_t len);

int EVP_DigestVerifyFinal(EVP_MD_CTX *ctx, const uint8_t *sig,
                          size_t sig_len);

EVP_PKEY *d2i_PUBKEY(EVP_PKEY **out, const uint8_t **inp,
                     long len);

int EVP_PKEY_size(const EVP_PKEY *pkey);

int EVP_PKEY_CTX_set_rsa_padding(EVP_PKEY_CTX *ctx, int padding);

int EVP_PKEY_CTX_set_rsa_pss_saltlen(EVP_PKEY_CTX *ctx, int salt_len);

int EVP_PKEY_CTX_set_rsa_oaep_md(EVP_PKEY_CTX *ctx, const EVP_MD *md);

int EVP_PKEY_CTX_set_rsa_mgf1_md(EVP_PKEY_CTX *ctx, const EVP_MD *md);

EVP_PKEY_CTX *EVP_PKEY_CTX_new(EVP_PKEY *pkey, ENGINE *e);

int EVP_PKEY_encrypt_init(EVP_PKEY_CTX *ctx);

int EVP_PKEY_encrypt(EVP_PKEY_CTX *ctx, uint8_t *out,
                     size_t *out_len, const uint8_t *in,
                     size_t in_len);

int EVP_PKEY_decrypt_init(EVP_PKEY_CTX *ctx);

int EVP_PKEY_decrypt(EVP_PKEY_CTX *ctx, uint8_t *out,
                     size_t *out_len, const uint8_t *in,
                     size_t in_len);

int i2d_PrivateKey(const EVP_PKEY *key, uint8_t **outp);

EVP_PKEY *X509_get_pubkey(X509 *x509);

int EVP_marshal_private_key(CBB *cbb, const EVP_PKEY *key);

int EVP_PKEY_derive_init(EVP_PKEY_CTX *ctx);

int EVP_PKEY_derive_set_peer(EVP_PKEY_CTX *ctx, EVP_PKEY *peer);

int EVP_PKEY_get_raw_private_key(const EVP_PKEY *pkey, uint8_t *out,
                                 size_t *out_len);

int EVP_PKEY_get_raw_public_key(const EVP_PKEY *pkey, uint8_t *out,
                                size_t *out_len);

int EVP_PKEY_derive(EVP_PKEY_CTX *ctx, uint8_t *key, size_t *out_key_len);

int PKCS5_PBKDF2_HMAC(const char *password, size_t password_len,
                      const uint8_t *salt, size_t salt_len,
                      unsigned iterations, const EVP_MD *digest,
                      size_t key_len, uint8_t *out_key);

const PKEY_METHOD *EVP_PKEY_method(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_EVP_H */
