/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/base.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_BASE_H
#define OPENSSL_MBEDTLS_WRAPPER_BASE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ASN1_BIT_STRING ASN1_STRING

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct EVP_CIPHER EVP_CIPHER;
typedef struct EVP_CIPHER_CTX EVP_CIPHER_CTX;
typedef struct ENGINE ENGINE;
typedef struct EVP_MD EVP_MD;
typedef struct EVP_MD_CTX EVP_MD_CTX;
typedef struct ASN1_BIT_STRING ASN1_BIT_STRING;
typedef struct ASN1_INTEGER ASN1_INTEGER;
typedef struct ASN1_OBJECT ASN1_OBJECT;
typedef struct ASN1_OCTET_STRING ASN1_OCTET_STRING;
typedef struct ASN1_TIME ASN1_TIME;
typedef struct BN_CTX BN_CTX;
typedef struct EC_GROUP EC_GROUP;
typedef struct EC_KEY EC_KEY;
typedef struct EC_POINT EC_POINT;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct EVP_PKEY_CTX EVP_PKEY_CTX;
typedef struct PKCS8_PRIV_KEY_INFO PKCS8_PRIV_KEY_INFO;
typedef struct X509_ALGOR X509_ALGOR;
typedef struct X509_EXTENSION X509_EXTENSION;
typedef struct X509_NAME X509_NAME;
typedef struct BIGNUM BIGNUM;
typedef struct HMAC_CTX HMAC_CTX;
typedef struct rsa_meth_st RSA_METHOD;
typedef struct ecdsa_method_st ECDSA_METHOD;
typedef struct BN_GENCB BN_GENCB;
typedef struct sha256_state_st SHA256_CTX;
typedef struct sha_state_st SHA_CTX;
typedef struct cbb_st CBB;
typedef struct ecdsa_sig_st ECDSA_SIG;
typedef void RSA;

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_BASE_H */
