/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/ecdsa.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_ECDSA_H
#define OPENSSL_MBEDTLS_WRAPPER_ECDSA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/ec_key.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ecdsa_sig_st
{
    BIGNUM *r;
    BIGNUM *s;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

size_t ECDSA_size(const EC_KEY *key);

int ECDSA_sign(int type, const uint8_t *digest, size_t digest_len,
               uint8_t *sig, unsigned int *sig_len,
               const EC_KEY *key);

int ECDSA_verify(int type, const uint8_t *digest,
                 size_t digest_len, const uint8_t *sig,
                 size_t sig_len, const EC_KEY *key);

int i2d_ECDSA_SIG(const ECDSA_SIG *sig, uint8_t **outp);

ECDSA_SIG *d2i_ECDSA_SIG(ECDSA_SIG **out, const uint8_t **inp, long len);

const BIGNUM *ECDSA_SIG_get0_r(const ECDSA_SIG *sig);

const BIGNUM *ECDSA_SIG_get0_s(const ECDSA_SIG *sig);

ECDSA_SIG *ECDSA_SIG_new(void);

void ECDSA_SIG_free(ECDSA_SIG *sig);

ECDSA_SIG *ECDSA_do_sign(const uint8_t *digest, size_t digest_len,
                         const EC_KEY *key);

int ECDSA_do_verify(const uint8_t *digest, size_t digest_len,
                    const ECDSA_SIG *sig, const EC_KEY *key);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_ECDSA_H */
