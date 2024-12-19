/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/aes.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_AES_H
#define OPENSSL_MBEDTLS_WRAPPER_AES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AES_ENCRYPT 1
#define AES_DECRYPT 0

#define AES_BLOCK_SIZE 16
#define AES_MAXNR 14

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct aes_key_st
{
  uint32_t rd_key[4 * (AES_MAXNR + 1)];
  unsigned rounds;
};

typedef struct aes_key_st AES_KEY;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int AES_set_encrypt_key(const uint8_t *key, unsigned bits,
                        AES_KEY *aeskey);

void AES_encrypt(const uint8_t *in, uint8_t *out,
                 const AES_KEY *key);

int AES_set_decrypt_key(const uint8_t *key, unsigned bits,
                        AES_KEY *aeskey);

void AES_decrypt(const uint8_t *in, uint8_t *out,
                 const AES_KEY *key);

void AES_cbc_encrypt(const uint8_t *in, uint8_t *out,
                     size_t len, const AES_KEY *key,
                     uint8_t *ivec, const int enc);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_AES_H */
