/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/sha.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_SHA_H
#define OPENSSL_MBEDTLS_WRAPPER_SHA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SHA256_DIGEST_LENGTH is the length of a SHA-256 digest */

#define SHA256_DIGEST_LENGTH 32

/* SHA_CBLOCK is the block size of SHA-1. */

#define SHA_CBLOCK 64

/* SHA_DIGEST_LENGTH is the length of a SHA-1 digest. */

#define SHA_DIGEST_LENGTH 20

#define SHA256_CBLOCK 64

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct sha_state_st
{
  /* wpa_supplicant accesses |h0|..|h4| so we must support those names
   * for compatibility with it until it can be updated.
   */

  union
  {
    uint32_t h[5];
    struct
    {
      uint32_t h0;
      uint32_t h1;
      uint32_t h2;
      uint32_t h3;
      uint32_t h4;
    };
  };
  uint32_t Nl;
  uint32_t Nh;
  uint8_t data[SHA_CBLOCK];
  unsigned num;
};

struct sha256_state_st
{
    uint32_t h[8];
    uint32_t Nl;
    uint32_t Nh;
    uint8_t data[SHA256_CBLOCK];
    unsigned num;
    unsigned md_len;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* SHA1_Init initialises |sha| and returns one. */

int SHA1_Init(SHA_CTX *sha);

/* SHA1_Update adds |len| bytes from |data| to |sha| and returns one. */

int SHA1_Update(SHA_CTX *sha, const void *data, size_t len);

/* SHA1_Final adds the final padding to |sha| and writes the
 * resulting digest to |out|, which must have at least
 * |SHA_DIGEST_LENGTH| bytes of space. It returns one.
 */

int SHA1_Final(uint8_t out[SHA_DIGEST_LENGTH], SHA_CTX *sha);

/* SHA1 writes the digest of |len| bytes from |data| to |out| and returns
 * |out|. There must be at least |SHA_DIGEST_LENGTH| bytes of space in
 * |out|.
 */

uint8_t *SHA1(const uint8_t *data, size_t len,
              uint8_t out[SHA_DIGEST_LENGTH]);

void SHA1_Transform(SHA_CTX *sha, const uint8_t block[SHA_CBLOCK]);

int SHA256_Init(SHA256_CTX *sha);

int SHA256_Update(SHA256_CTX *sha, const void *data, size_t len);

int SHA256_Final(uint8_t out[SHA256_DIGEST_LENGTH], SHA256_CTX *sha);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_SHA_H */
