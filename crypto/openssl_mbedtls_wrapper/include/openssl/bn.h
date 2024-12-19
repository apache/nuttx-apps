/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/bn.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_BN_H
#define OPENSSL_MBEDTLS_WRAPPER_BN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/asn1.h>
#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BN_ULONG uint32_t
#define BN_BITS2 32

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef uint32_t BN_ULONG;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

BN_CTX *BN_CTX_new(void);
void BN_CTX_free(BN_CTX *a);
void BN_free(BIGNUM *a);

BN_ULONG BN_get_word(const BIGNUM *bn);

unsigned BN_num_bits(const BIGNUM *bn);

BIGNUM *BN_new(void);

int BN_bn2binpad(const BIGNUM *in, uint8_t *out, int len);

int BN_set_word(BIGNUM *bn, BN_ULONG value);

BIGNUM *BN_dup(const BIGNUM *src);

BIGNUM *BN_bin2bn(const uint8_t *in, size_t len, BIGNUM *ret);

int BN_one(BIGNUM *bn);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_BN_H */
