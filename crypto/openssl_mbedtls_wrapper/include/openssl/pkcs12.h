/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/pkcs12.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_PKCS12_H
#define OPENSSL_MBEDTLS_WRAPPER_PKCS12_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/stack.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct PKCS12_st PKCS12;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

PKCS12 *d2i_PKCS12(PKCS12 **a, const unsigned char **in, long len);

PKCS12 *d2i_PKCS12_fp(FILE *fp, PKCS12 **a);

int PKCS12_parse(PKCS12 *p12, const char *pass,
                 EVP_PKEY **pkey, X509 **cert,
                 STACK_OF(X509) **ca);

void PKCS12_free(PKCS12 *p12);

int i2d_PKCS12(const PKCS12 *p12, unsigned char **out);

PKCS12 *PKCS12_create(const char *pass, const char *name,
                      EVP_PKEY *pkey, X509 *cert,
                      STACK_OF(X509) *ca,
                      int nid_key, int nid_cert,
                      int iter, int mac_iter, int keytype);

int PKCS12_newpass(PKCS12 *p12, const char *oldpass, const char *newpass);

PKCS12 *d2i_PKCS12_bio(BIO *bp, PKCS12 **p12);

int i2d_PKCS12_bio(BIO *bp, const PKCS12 *p12);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_PKCS12_H */
