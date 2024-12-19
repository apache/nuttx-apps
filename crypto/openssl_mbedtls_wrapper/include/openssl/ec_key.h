/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/ec_key.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_EC_KEY_H
#define OPENSSL_MBEDTLS_WRAPPER_EC_KEY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/engine.h>
#include <openssl/ex_data.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ECDSA_FLAG_OPAQUE 1

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ecdsa_method_st
{
  struct openssl_method_common_st common;

  void *app_data;

  int (*init)(EC_KEY *key);
  int (*finish)(EC_KEY *key);

  /* group_order_size returns the number of bytes needed to
   * represent the order of the group. This is used to
   * calculate the maximum size of an ECDSA signature
   * in |ECDSA_size|.
   */

  size_t (*group_order_size)(const EC_KEY *key);

  /* sign matches the arguments and behaviour of |ECDSA_sign|. */

  int (*sign)(const uint8_t *digest, size_t digest_len, uint8_t *sig,
              unsigned int *sig_len, EC_KEY *eckey);

  int flags;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

EC_KEY *EC_KEY_new(void);

const EC_GROUP *EC_KEY_get0_group(const EC_KEY *key);

const EC_POINT *EC_KEY_get0_public_key(const EC_KEY *key);

int EC_KEY_set_group(EC_KEY *key, const EC_GROUP *group);

int EC_KEY_generate_key(EC_KEY *key);

int EC_KEY_check_key(const EC_KEY *key);

int EC_KEY_get_ex_new_index(long argl, void *argp,
                            CRYPTO_EX_unused *unused,
                            CRYPTO_EX_dup *dup_unused,
                            CRYPTO_EX_free *free_func);

EC_KEY *EC_KEY_new_method(const ENGINE *engine);

int EC_KEY_set_ex_data(EC_KEY *r, int idx, void *arg);

void *EC_KEY_get_ex_data(const EC_KEY *r, int idx);

int EC_KEY_set_public_key(EC_KEY *key, const EC_POINT *pub);

EC_KEY *EC_KEY_new_by_curve_name(int nid);

int EC_KEY_set_private_key(EC_KEY *key, const BIGNUM *priv);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_EC_KEY_H */
