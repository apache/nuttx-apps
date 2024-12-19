/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/hmac.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_HMAC_H
#define OPENSSL_MBEDTLS_WRAPPER_HMAC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/digest.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* HMAC calculates the HMAC of |data_len| bytes of |data|, using the given
 * key and hash function, and writes the result to |out|. On entry, |out|
 * must contain at least |EVP_MD_size| bytes of space. The actual length
 * of the result is written to |*out_len|. An output size of
 * |EVP_MAX_MD_SIZE|  will always be large enough. It returns |out|
 * or NULL on error.
 */

uint8_t *HMAC(const EVP_MD *evp_md, const void *key,
              size_t key_len, const uint8_t *data, size_t data_len,
              uint8_t *out, unsigned int *out_len);

HMAC_CTX *HMAC_CTX_new(void);

void HMAC_CTX_init(HMAC_CTX *ctx);

int HMAC_Init_ex(HMAC_CTX *ctx, const void *key,
                 size_t key_len, const EVP_MD *md,
                 ENGINE *impl);

int HMAC_Update(HMAC_CTX *ctx, const uint8_t *data, size_t data_len);

int HMAC_Final(HMAC_CTX *ctx, uint8_t *out, unsigned int *out_len);

void HMAC_CTX_cleanup(HMAC_CTX *ctx);

void HMAC_CTX_free(HMAC_CTX *ctx);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_HMAC_H */

