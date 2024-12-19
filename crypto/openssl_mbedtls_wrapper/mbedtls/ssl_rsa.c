/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/ssl_rsa.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include <openssl/ssl_dbg.h>
#include <openssl/evp.h>
#include <openssl/ssl_local.h>
#include <openssl/x509.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int SSL_CTX_use_certificate(SSL_CTX *ctx, X509 *x)
{
  SSL_ASSERT1(ctx);
  SSL_ASSERT1(x);

  if (ctx->cert->x509 == x)
    {
      return 1;
    }

  X509_free(ctx->cert->x509);
  ctx->cert->x509 = x;
  return 1;
}

int SSL_use_certificate(SSL *ssl, X509 *x)
{
  SSL_ASSERT1(ssl);
  SSL_ASSERT1(x);

  if (ssl->cert->x509 == x)
    {
      return 1;
    }

  X509_free(ssl->cert->x509);
  ssl->cert->x509 = x;
  return 1;
}

int SSL_CTX_use_certificate_ASN1(SSL_CTX *ctx, int len,
                                 const unsigned char *d)
{
  int ret;
  X509 *x;

  x = d2i_X509(NULL, &d, len);
  if (!x)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "d2i_X509() return NULL");
      goto failed1;
    }

  ret = SSL_CTX_use_certificate(ctx, x);
  if (!ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL,
                "SSL_CTX_use_certificate() return %d", ret);
      goto failed2;
    }

  return 1;

failed2:
  X509_free(x);
failed1:
  return 0;
}

int SSL_use_certificate_ASN1(SSL *ssl, const unsigned char *d, int len)
{
  int ret;
  X509 *x;

  x = d2i_X509(NULL, &d, len);
  if (!x)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "d2i_X509() return NULL");
      goto failed1;
    }

  ret = SSL_use_certificate(ssl, x);
  if (!ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL,
                "SSL_use_certificate() return %d", ret);
      goto failed2;
    }

  return 1;

failed2:
  X509_free(x);
failed1:
  return 0;
}

int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type)
{
  return 0;
}

int SSL_use_certificate_file(SSL *ssl, const char *file, int type)
{
  return 0;
}

int SSL_CTX_use_PrivateKey(SSL_CTX *ctx, EVP_PKEY *pkey)
{
  SSL_ASSERT1(ctx);
  SSL_ASSERT1(pkey);

  if (ctx->cert->pkey == pkey)
    {
      return 1;
    }

  if (ctx->cert->pkey)
    {
      EVP_PKEY_free(ctx->cert->pkey);
    }

  ctx->cert->pkey = pkey;

  return 1;
}

int SSL_use_PrivateKey(SSL *ssl, EVP_PKEY *pkey)
{
  SSL_ASSERT1(ssl);
  SSL_ASSERT1(pkey);

  if (ssl->cert->pkey == pkey)
    {
      return 1;
    }

  if (ssl->cert->pkey)
    {
      EVP_PKEY_free(ssl->cert->pkey);
    }

  ssl->cert->pkey = pkey;

  return 1;
}

int SSL_CTX_use_PrivateKey_ASN1(int type, SSL_CTX *ctx,
                                const unsigned char *d, long len)
{
  int ret;
  EVP_PKEY *pk;

  pk = d2i_PrivateKey(0, NULL, &d, len);
  if (!pk)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "d2i_PrivateKey() return NULL");
      goto failed1;
    }

  ret = SSL_CTX_use_PrivateKey(ctx, pk);
  if (!ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL,
                "SSL_CTX_use_PrivateKey() return %d", ret);
      goto failed2;
    }

  return 1;

failed2:
  EVP_PKEY_free(pk);
failed1:
  return 0;
}

int SSL_use_PrivateKey_ASN1(int type, SSL *ssl,
                            const unsigned char *d, long len)
{
  int ret;
  EVP_PKEY *pk;

  pk = d2i_PrivateKey(0, NULL, &d, len);
  if (!pk)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "d2i_PrivateKey() return NULL");
      goto failed1;
    }

  ret = SSL_use_PrivateKey(ssl, pk);
  if (!ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "SSL_use_PrivateKey() return %d", ret);
      goto failed2;
    }

  return 1;

failed2:
  EVP_PKEY_free(pk);
failed1:
  return 0;
}

int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type)
{
  return 0;
}

int SSL_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type)
{
  return 0;
}

int SSL_CTX_use_RSAPrivateKey_ASN1(SSL_CTX *ctx, const unsigned char *d,
                                   long len)
{
  return SSL_CTX_use_PrivateKey_ASN1(0, ctx, d, len);
}
