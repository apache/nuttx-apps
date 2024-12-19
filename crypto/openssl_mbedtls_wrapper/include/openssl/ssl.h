/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/ssl.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_SSL_H
#define OPENSSL_MBEDTLS_WRAPPER_SSL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stddef.h>
#include <openssl/types.h>
#include <openssl/x509_vfy.h>
#include <openssl/tls1.h>

/* Used in SSL_set_shutdown()/SSL_get_shutdown(); */
#define SSL_SENT_SHUTDOWN       1
#define SSL_RECEIVED_SHUTDOWN   2

#define SSL_VERIFY_NONE                 0x00
#define SSL_VERIFY_PEER                 0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT 0x02
#define SSL_VERIFY_CLIENT_ONCE          0x04

/* The following 3 states are kept in ssl->rlayer.rstate when reads fail, you
 * should not need these
 */
#define SSL_ST_READ_HEADER              0xF0
#define SSL_ST_READ_BODY                0xF1
#define SSL_ST_READ_DONE                0xF2

#define SSL_NOTHING                     1
#define SSL_WRITING                     2
#define SSL_READING                     3
#define SSL_X509_LOOKUP                 4
#define SSL_ASYNC_PAUSED                5
#define SSL_ASYNC_NO_JOBS               6

#define SSL_ERROR_NONE                  0
#define SSL_ERROR_SSL                   1
#define SSL_ERROR_WANT_READ             2
#define SSL_ERROR_WANT_WRITE            3
#define SSL_ERROR_WANT_X509_LOOKUP      4
#define SSL_ERROR_SYSCALL               5/* look at error stack/return value/errno */
#define SSL_ERROR_ZERO_RETURN           6
#define SSL_ERROR_WANT_CONNECT          7
#define SSL_ERROR_WANT_ACCEPT           8
#define SSL_ERROR_WANT_ASYNC            9
#define SSL_ERROR_WANT_ASYNC_JOB       10

typedef enum
{
    TLS_ST_BEFORE,
    TLS_ST_OK,
    DTLS_ST_CR_HELLO_VERIFY_REQUEST,
    TLS_ST_CR_SRVR_HELLO,
    TLS_ST_CR_CERT,
    TLS_ST_CR_CERT_STATUS,
    TLS_ST_CR_KEY_EXCH,
    TLS_ST_CR_CERT_REQ,
    TLS_ST_CR_SRVR_DONE,
    TLS_ST_CR_SESSION_TICKET,
    TLS_ST_CR_CHANGE,
    TLS_ST_CR_FINISHED,
    TLS_ST_CW_CLNT_HELLO,
    TLS_ST_CW_CERT,
    TLS_ST_CW_KEY_EXCH,
    TLS_ST_CW_CERT_VRFY,
    TLS_ST_CW_CHANGE,
    TLS_ST_CW_NEXT_PROTO,
    TLS_ST_CW_FINISHED,
    TLS_ST_SW_HELLO_REQ,
    TLS_ST_SR_CLNT_HELLO,
    DTLS_ST_SW_HELLO_VERIFY_REQUEST,
    TLS_ST_SW_SRVR_HELLO,
    TLS_ST_SW_CERT,
    TLS_ST_SW_KEY_EXCH,
    TLS_ST_SW_CERT_REQ,
    TLS_ST_SW_SRVR_DONE,
    TLS_ST_SR_CERT,
    TLS_ST_SR_KEY_EXCH,
    TLS_ST_SR_CERT_VRFY,
    TLS_ST_SR_NEXT_PROTO,
    TLS_ST_SR_CHANGE,
    TLS_ST_SR_FINISHED,
    TLS_ST_SW_SESSION_TICKET,
    TLS_ST_SW_CERT_STATUS,
    TLS_ST_SW_CHANGE,
    TLS_ST_SW_FINISHED
}
OSSL_HANDSHAKE_STATE;

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

X509_VERIFY_PARAM *SSL_get0_param(SSL *ssl);

int X509_VERIFY_PARAM_set_hostflags(X509_VERIFY_PARAM *param,
                                    unsigned long flags);

int X509_VERIFY_PARAM_clear_hostflags(X509_VERIFY_PARAM *param,
                                      unsigned long flags);

int SSL_CTX_add_client_CA(SSL_CTX *ctx, X509 *x);

int SSL_CTX_add_client_CA_ASN1(SSL_CTX *ssl, int len,
                               const unsigned char *d);

int SSL_CTX_use_certificate(SSL_CTX *ctx, X509 *x);

int SSL_use_certificate(SSL *ssl, X509 *x);

X509 *SSL_get_certificate(const SSL *ssl);

int SSL_CTX_use_certificate_ASN1(SSL_CTX *ctx, int len,
                                 const unsigned char *d);

int SSL_use_certificate_ASN1(SSL *ssl, const unsigned char *d, int len);

int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type);

int SSL_use_certificate_file(SSL *ssl, const char *file, int type);

X509 *SSL_get_peer_certificate(const SSL *ssl);

int SSL_want(const SSL *ssl);

int SSL_want_nothing(const SSL *ssl);

int SSL_want_read(const SSL *ssl);

int SSL_want_write(const SSL *ssl);

int SSL_want_x509_lookup(const SSL *ssl);

void _ssl_set_alpn_list(const SSL *ssl);

int SSL_get_error(const SSL *ssl, int ret_code);

OSSL_HANDSHAKE_STATE SSL_get_state(const SSL *ssl);

SSL_CTX *SSL_CTX_new(const SSL_METHOD *method, void *rngctx);

void SSL_CTX_free(SSL_CTX *ctx);

int SSL_CTX_set_ssl_version(SSL_CTX *ctx, const SSL_METHOD *meth);

const SSL_METHOD *SSL_CTX_get_ssl_method(SSL_CTX *ctx);

SSL *SSL_new(SSL_CTX *ctx);

void SSL_free(SSL *ssl);

int SSL_do_handshake(SSL *ssl);

int SSL_connect(SSL *ssl);

int SSL_accept(SSL *ssl);

int SSL_shutdown(SSL *ssl);

int SSL_clear(SSL *ssl);

int SSL_read(SSL *ssl, void *buffer, int len);

int SSL_write(SSL *ssl, const void *buffer, int len);

SSL_CTX *SSL_get_SSL_CTX(const SSL *ssl);

const SSL_METHOD *SSL_get_ssl_method(SSL *ssl);

int SSL_set_ssl_method(SSL *ssl, const SSL_METHOD *method);

int SSL_get_shutdown(const SSL *ssl);

void SSL_set_shutdown(SSL *ssl, int mode);

int SSL_pending(const SSL *ssl);

int SSL_has_pending(const SSL *ssl);

unsigned long SSL_CTX_clear_options(SSL_CTX *ctx, unsigned long op);

unsigned long SSL_CTX_get_options(SSL_CTX *ctx);

unsigned long SSL_clear_options(SSL *ssl, unsigned long op);

unsigned long SSL_get_options(SSL *ssl);

unsigned long SSL_set_options(SSL *ssl, unsigned long op);

int SSL_get_fd(const SSL *ssl);

int SSL_get_rfd(const SSL *ssl);

int SSL_get_wfd(const SSL *ssl);

int SSL_set_fd(SSL *ssl, int fd);

int SSL_set_rfd(SSL *ssl, int fd);

int SSL_set_wfd(SSL *ssl, int fd);

int SSL_version(const SSL *ssl);

const char *SSL_alert_type_string(int value);

void SSL_CTX_set_default_read_buffer_len(SSL_CTX *ctx, size_t len);

void SSL_set_default_read_buffer_len(SSL *ssl, size_t len);

void SSL_set_info_callback(SSL *ssl,
                           void (*cb) (const SSL *ssl, int type, int val));

int SSL_CTX_up_ref(SSL_CTX *ctx);

void SSL_set_security_level(SSL *ssl, int level);

int SSL_get_security_level(const SSL *ssl);

int SSL_CTX_get_verify_mode(const SSL_CTX *ctx);

long SSL_CTX_set_timeout(SSL_CTX *ctx, long t);

long SSL_CTX_get_timeout(const SSL_CTX *ctx);

void SSL_set_read_ahead(SSL *ssl, int yes);

void SSL_CTX_set_read_ahead(SSL_CTX *ctx, int yes);

int SSL_get_read_ahead(const SSL *ssl);

long SSL_CTX_get_read_ahead(SSL_CTX *ctx);

long SSL_CTX_get_default_read_ahead(SSL_CTX *ctx);

long SSL_set_time(SSL *ssl, long t);

long SSL_set_timeout(SSL *ssl, long t);

long SSL_get_verify_result(const SSL *ssl);

int SSL_CTX_get_verify_depth(const SSL_CTX *ctx);

void SSL_CTX_set_verify_depth(SSL_CTX *ctx, int depth);

int SSL_get_verify_depth(const SSL *ssl);

void SSL_set_verify_depth(SSL *ssl, int depth);

void SSL_CTX_set_verify(SSL_CTX *ctx, int mode,
                        int (*verify_callback)(int, X509_STORE_CTX *));

void SSL_set_verify(SSL *ssl, int mode,
                    int (*verify_callback)(int, X509_STORE_CTX *));

void *SSL_CTX_get_ex_data(const SSL_CTX *ctx, int idx);

void SSL_CTX_set_alpn_select_cb(SSL_CTX *ctx, next_proto_cb cb, void *arg);

void SSL_set_alpn_select_cb(SSL *ssl, void *arg);

int SSL_CTX_use_PrivateKey(SSL_CTX *ctx, EVP_PKEY *pkey);

int SSL_use_PrivateKey(SSL *ssl, EVP_PKEY *pkey);

int SSL_CTX_use_PrivateKey_ASN1(int type, SSL_CTX *ctx,
                                const unsigned char *d, long len);

int SSL_use_PrivateKey_ASN1(int type, SSL *ssl,
                            const unsigned char *d, long len);

int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type);

int SSL_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type);

int SSL_CTX_use_RSAPrivateKey_ASN1(SSL_CTX *ctx, const unsigned char *d,
                                   long len);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_SSL_H */
