/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/x509.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_X509_H
#define OPENSSL_MBEDTLS_WRAPPER_X509_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include <openssl/base.h>
#include <openssl/bytestring.h>
#include <openssl/ecdsa.h>
#include <openssl/obj.h>
#include <openssl/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct x509_st
{
/* X509 certification platform private point */

  void *x509_pm;
  const X509_METHOD *method;
};

struct x509_method_st
{
  int (*x509_new)(X509 *x, X509 *m_x);
  void (*x509_free)(X509 *x);
  int (*x509_load)(X509 *x, const unsigned char *buf, int len);
  int (*x509_show_info)(X509 *x);
};

struct cert_st
{
  int sec_level;
  X509 *x509;
  EVP_PKEY *pkey;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void PKCS8_PRIV_KEY_INFO_free(PKCS8_PRIV_KEY_INFO *key);

X509 *X509_new(void);

void X509_free(X509 *a);

void X509_ALGOR_free(X509_ALGOR *a);

void X509_EXTENSION_free(X509_EXTENSION *a);

void X509_NAME_free(X509_NAME *a);

EVP_PKEY *EVP_PKCS82PKEY(const PKCS8_PRIV_KEY_INFO *p8);

X509_ALGOR *X509_ALGOR_new(void);

int X509_ALGOR_set0(X509_ALGOR *alg, ASN1_OBJECT *obj,
                    int param_type, void *param_value);

int X509_set1_signature_algo(X509 *x509, const X509_ALGOR *algo);

int X509_set1_signature_value(X509 *x509, const uint8_t *sig,
                              size_t sig_len);

X509_NAME *X509_NAME_new(void);

int X509_NAME_get_text_by_NID(const X509_NAME *name, int nid,
                              char *buf, int len);

int X509_set_issuer_name(X509 *x509, X509_NAME *name);

int X509_NAME_add_entry_by_txt(X509_NAME *name, const char *field,
                               int type, const uint8_t *bytes,
                               int len, int loc, int set);

X509_NAME *d2i_X509_NAME(X509_NAME **out, const uint8_t **inp, long len);

X509_EXTENSION *X509_EXTENSION_create_by_NID(X509_EXTENSION **ex,
                                             int nid, int crit,
                                             const ASN1_OCTET_STRING *data);

int X509_set_version(X509 *x509, long version);

int X509_set_serialNumber(X509 *x509, const ASN1_INTEGER *serial);

int X509_set_subject_name(X509 *x509, X509_NAME *name);

int X509_set_notBefore(X509 *x509, const ASN1_TIME *tm);

int X509_set_notAfter(X509 *x509, const ASN1_TIME *tm);

int X509_set_pubkey(X509 *x509, EVP_PKEY *pkey);

int X509_add_ext(X509 *x, const X509_EXTENSION *ex, int loc);

int X509_sign(X509 *x509, EVP_PKEY *pkey, const EVP_MD *md);

int i2d_X509(X509 *x509, uint8_t **outp);

PKCS8_PRIV_KEY_INFO *d2i_PKCS8_PRIV_KEY_INFO(PKCS8_PRIV_KEY_INFO *info,
                                             const uint8_t **key_data,
                                             size_t key_length);

X509 *d2i_X509(X509 **out, const uint8_t **inp, long len);

const char *X509_verify_cert_error_string(long n);

X509 *__X509_new(X509 *ix);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_X509_H */
