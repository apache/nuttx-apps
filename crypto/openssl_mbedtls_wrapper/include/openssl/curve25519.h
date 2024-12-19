/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/curve25519.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_CURVE25519_H
#define OPENSSL_MBEDTLS_WRAPPER_CURVE25519_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define X25519_PRIVATE_KEY_LEN 32
#define X25519_PUBLIC_VALUE_LEN 32
#define X25519_SHARED_KEY_LEN 32
#define ED25519_PRIVATE_KEY_LEN 64
#define ED25519_PUBLIC_KEY_LEN 32
#define ED25519_SIGNATURE_LEN 64

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* ED25519_keypair sets |out_public_key| and |out_private_key| to a freshly
 * generated, public–private key pair.
 */

void ED25519_keypair(uint8_t out_public_key[32],
                     uint8_t out_private_key[64]);

void X25519_keypair(uint8_t out_public_value[32],
                    uint8_t out_private_key[32]);

int X25519(uint8_t out_shared_key[32], const uint8_t private_key[32],
           const uint8_t peer_public_value[32]);

int ED25519_sign(uint8_t out_sig[64], const uint8_t *message,
                 size_t message_len, const uint8_t private_key[64]);

int ED25519_verify(const uint8_t *message, size_t message_len,
                   const uint8_t signature[64],
                   const uint8_t public_key[32]);

void ED25519_keypair_from_seed(uint8_t out_public_key[32],
                               uint8_t out_private_key[64],
                               const uint8_t seed[32]);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_CURVE25519_H */
