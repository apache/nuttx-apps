/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/digest.h
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

#ifndef KEYMASTER_DIGEST_H
#define KEYMASTER_DIGEST_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EVP_MAX_MD_SIZE 64  // SHA-512 is the longest so far.
#define EVP_MAX_MD_BLOCK_SIZE 128  // SHA-512 is the longest so far.

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

const EVP_MD *EVP_md4(void);
const EVP_MD *EVP_md5(void);
const EVP_MD *EVP_sha1(void);
const EVP_MD *EVP_sha224(void);
const EVP_MD *EVP_sha256(void);
const EVP_MD *EVP_sha384(void);
const EVP_MD *EVP_sha512(void);
const EVP_MD *EVP_sha512_256(void);
const EVP_MD *EVP_blake2b256(void);

const EVP_MD *EVP_md5_sha1(void);

const EVP_MD *EVP_get_digestbynid(int nid);

const EVP_MD *EVP_get_digestbyobj(const ASN1_OBJECT *obj);

void EVP_MD_CTX_init(EVP_MD_CTX *ctx);

EVP_MD_CTX *EVP_MD_CTX_new(void);

int EVP_MD_CTX_cleanup(EVP_MD_CTX *ctx);

void EVP_MD_CTX_cleanse(EVP_MD_CTX *ctx);

void EVP_MD_CTX_free(EVP_MD_CTX *ctx);

int EVP_MD_CTX_copy_ex(EVP_MD_CTX *out, const EVP_MD_CTX *in);

void EVP_MD_CTX_move(EVP_MD_CTX *out, EVP_MD_CTX *in);

int EVP_MD_CTX_reset(EVP_MD_CTX *ctx);

/* Digest operations. */

int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type,
                      ENGINE *engine);

int EVP_DigestInit(EVP_MD_CTX *ctx, const EVP_MD *type);

int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *data, size_t len);

int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, uint8_t *md_out,
                       unsigned int *out_size);

int EVP_DigestFinal(EVP_MD_CTX *ctx, uint8_t *md_out,
                    unsigned int *out_size);

int EVP_Digest(const void *data, size_t len,
               uint8_t *md_out, unsigned int *md_out_size,
               const EVP_MD *type, ENGINE *impl);

size_t EVP_MD_size(const EVP_MD *md);

#ifdef __cplusplus
}
#endif

#endif /* KEYMASTER_DIGEST_H */

