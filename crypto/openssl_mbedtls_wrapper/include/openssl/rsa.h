/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/rsa.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_RSA_H
#define OPENSSL_MBEDTLS_WRAPPER_RSA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/engine.h>
#include <openssl/ex_data.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RSA_PKCS1_PADDING 1
#define RSA_FLAG_OPAQUE 1
#define RSA_NO_PADDING 3
#define RSA_PKCS1_OAEP_PADDING 4
#define RSA_PKCS1_PSS_PADDING 6

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct rsa_meth_st
{
  struct openssl_method_common_st common;

  void *app_data;

  int (*init)(RSA *rsa);
  int (*finish)(RSA *rsa);

  /* size returns the size of the RSA modulus in bytes. */

  size_t (*size)(const RSA *rsa);

  int (*sign)(int type, const uint8_t *m,
              unsigned int m_length, uint8_t *sigret,
              unsigned int *siglen, const RSA *rsa);

  /* These functions mirror the |RSA_*| functions of the same name. */

  int (*sign_raw)(RSA *rsa, size_t *out_len,
                  uint8_t *out, size_t max_out,
                  const uint8_t *in, size_t in_len,
                  int padding);
  int (*decrypt)(RSA *rsa, size_t *out_len,
                 uint8_t *out, size_t max_out,
                 const uint8_t *in, size_t in_len,
                 int padding);

  /* private_transform takes a big-endian integer from |in|, calculates the
   * d'th power of it, modulo the RSA modulus and writes the result as a
   * big-endian integer to |out|. Both |in| and |out| are |len| bytes long
   * and |len| is always equal to |RSA_size(rsa)|. If the result of
   * the transform can be represented in fewer than |len| bytes,
   * then |out| must be zero padded on the left.
   *
   * It returns one on success and zero otherwise.
   *
   * RSA decrypt and sign operations will call this,
   * thus an ENGINE might wish
   * to override it in order to avoid having to implement the padding
   * functionality demanded by those, higher level, operations.
   */

  int (*private_transform)(RSA *rsa, uint8_t *out,
                           const uint8_t *in, size_t len);

  int flags;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

RSA *RSA_new(void);

void RSA_free(RSA *a);

unsigned RSA_size(const RSA *rsa);

const BIGNUM *RSA_get0_e(const RSA *rsa);

const BIGNUM *RSA_get0_n(const RSA *rsa);

int RSA_generate_key_ex(RSA *rsa, int bits,
                        const BIGNUM *e, BN_GENCB *cb);

int RSA_get_ex_new_index(long argl, void *argp,
                         CRYPTO_EX_unused *unused,
                         CRYPTO_EX_dup *dup_unused,
                         CRYPTO_EX_free *free_func);

RSA *RSA_new_method(const ENGINE *engine);

int RSA_set_ex_data(RSA *rsa, int idx, void *arg);

void *RSA_get_ex_data(const RSA *rsa, int idx);

int RSA_set0_key(RSA *rsa, BIGNUM *n, BIGNUM *e, BIGNUM *d);

int RSA_private_encrypt(size_t flen, const uint8_t *from,
                        uint8_t *to, RSA *rsa, int padding);

int RSA_public_decrypt(size_t flen, const uint8_t *from,
                       uint8_t *to, RSA *rsa, int padding);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_RSA_H */

