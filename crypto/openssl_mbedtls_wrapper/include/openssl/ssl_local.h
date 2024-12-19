/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/ssl_local.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_SSL_LOCAL_H
#define OPENSSL_MBEDTLS_WRAPPER_SSL_LOCAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/ssl.h>
#include <openssl/statem.h>
#include <openssl/x509.h>
#include <openssl/x509_local.h>
#include <openssl/x509_vfy.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct record_layer_st
{
  int rstate;
  int read_ahead;
};

struct ssl_ctx_st
{
  int version;
  int references;
  unsigned long options;
  const SSL_METHOD *method;
  CERT *cert;
  X509 *client_CA;
  const char **alpn_protos;
  next_proto_cb alpn_cb;
  int verify_mode;
  int (*default_verify_callback) (int ok, X509_STORE_CTX *ctx);
  long session_timeout;
  int read_ahead;
  int read_buffer_len;
  X509_VERIFY_PARAM param;
};

struct ssl_method_func_st
{
  int (*ssl_new)(SSL *ssl);
  void (*ssl_free)(SSL *ssl);
  int (*ssl_handshake)(SSL *ssl);
  int (*ssl_shutdown)(SSL *ssl);
  int (*ssl_clear)(SSL *ssl);
  int (*ssl_read)(SSL *ssl, void *buffer, int len);
  int (*ssl_send)(SSL *ssl, const void *buffer, int len);
  int (*ssl_pending)(const SSL *ssl);
  void (*ssl_set_fd)(SSL *ssl, int fd, int mode);
  int (*ssl_get_fd)(const SSL *ssl, int mode);
  void (*ssl_set_bufflen)(SSL *ssl, int len);
  long (*ssl_get_verify_result)(const SSL *ssl);
  OSSL_HANDSHAKE_STATE (*ssl_get_state)(const SSL *ssl);
};

struct ssl_method_st
{
  /* protocol version(one of SSL3.0, TLS1.0, etc.) */

  int version;

  /* SSL mode(client(0) , server(1), not known(-1)) */

  int endpoint;
  const SSL_METHOD_FUNC *func;
};

struct ssl_session_st
{
  long timeout;
  long time;
  X509 *peer;
};

struct ssl_st
{
/* protocol version(one of SSL3.0, TLS1.0, etc.) */

  int version;
  unsigned long options;

/* shut things down(0x01 : sent, 0x02 : received) */

  int shutdown;
  CERT *cert;
  X509 *client_CA;
  SSL_CTX  *ctx;
  const SSL_METHOD *method;
  const char **alpn_protos;
  RECORD_LAYER rlayer;

/* where we are */

  OSSL_STATEM statem;
  SSL_SESSION *session;
  int verify_mode;
  int (*verify_callback) (int ok, X509_STORE_CTX *ctx);
  int rwstate;
  int interrupted_remaining_write;
  long verify_result;
  X509_VERIFY_PARAM param;
  int err;
  void (*info_callback) (const SSL *ssl, int type, int val);

/* SSL low-level system arch point */

  void *ssl_pm;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

CERT *__ssl_cert_new(CERT *ic);
CERT *ssl_cert_new(void);
void ssl_cert_free(CERT *cert);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_SSL_LOCAL_H */
