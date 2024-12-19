/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/hkdf.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_HKDF_H
#define OPENSSL_MBEDTLS_WRAPPER_HKDF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int HKDF(uint8_t *out_key, size_t out_len,
         const EVP_MD *digest, const uint8_t *secret,
         size_t secret_len, const uint8_t *salt,
         size_t salt_len, const uint8_t *info,
         size_t info_len);

/* HKDF_extract computes a HKDF PRK (as specified by RFC 5869) from initial
 * keying material |secret| and salt |salt| using |digest|, and outputs
 * |out_len| bytes to |out_key|. The maximum output size is
 * |EVP_MAX_MD_SIZE|. It returns one on success and zero on error.
 *
 * WARNING: This function orders the inputs differently from RFC 5869
 * specification. Double-check which parameter is the secret/IKM and which is
 * the salt when using.
 */

int HKDF_extract(uint8_t *out_key, size_t *out_len,
                 const EVP_MD *digest, const uint8_t *secret,
                 size_t secret_len, const uint8_t *salt,
                 size_t salt_len);

/* HKDF_expand computes a HKDF OKM (as specified by RFC 5869) of length
 * |out_len| from the PRK |prk| and info |info| using |digest|, and outputs
 * the result to |out_key|. It returns one on success and zero on error.
 */

int HKDF_expand(uint8_t *out_key, size_t out_len,
                const EVP_MD *digest, const uint8_t *prk,
                size_t prk_len, const uint8_t *info,
                size_t info_len);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_HKDF_H */
