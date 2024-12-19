/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/types.h
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2015-2016 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#ifndef OPENSSL_MBEDTLS_WRAPPER_TYPES_H
#define OPENSSL_MBEDTLS_WRAPPER_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef void SSL_CIPHER;
typedef void X509_STORE;

typedef void RSA;
typedef void BIO;

typedef int (*OPENSSL_sk_compfunc)(const void *, const void *);

typedef struct stack_st OPENSSL_STACK;
typedef struct ssl_method_st SSL_METHOD;
typedef struct ssl_method_func_st SSL_METHOD_FUNC;
typedef struct record_layer_st RECORD_LAYER;
typedef struct ossl_statem_st OSSL_STATEM;
typedef struct ssl_session_st SSL_SESSION;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;
typedef struct cert_st CERT;
typedef struct x509_st X509;
typedef struct X509_VERIFY_PARAM_st X509_VERIFY_PARAM;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct x509_method_st X509_METHOD;
typedef struct pkey_method_st PKEY_METHOD;

#define ossl_inline inline
#define OPENSSL_NPN_NEGOTIATED 1

#define SSL_METHOD_CALL(f, s, ...)        s->method->func->ssl_##f(s, ##__VA_ARGS__)
#define X509_METHOD_CALL(f, x, ...)       x->method->x509_##f(x, ##__VA_ARGS__)
#define EVP_PKEY_METHOD_CALL(f, k, ...)   k->method->pkey_##f(k, ##__VA_ARGS__)

typedef int (*next_proto_cb)(SSL *ssl, unsigned char **out,
                             unsigned char *outlen, const unsigned char *in,
                             unsigned int inlen, void *arg);

struct pkey_method_st
{
  int (*pkey_new)(EVP_PKEY *pkey, EVP_PKEY *m_pkey);
  void (*pkey_free)(EVP_PKEY *pkey);
  int (*pkey_load)(EVP_PKEY *pkey, const unsigned char *buf, int len);
};

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_TYPES_H */
