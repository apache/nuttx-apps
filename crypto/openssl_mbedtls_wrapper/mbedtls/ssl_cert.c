/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/ssl_cert.c
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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <openssl/ssl_dbg.h>
#include <openssl/evp.h>
#include <openssl/ssl_local.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include "ssl_port.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

CERT *__ssl_cert_new(CERT *ic)
{
  CERT *cert;

  X509 *ix;
  EVP_PKEY *ipk;

  cert = ssl_mem_zalloc(sizeof(CERT));
  if (!cert)
    {
      SSL_DEBUG(SSL_CERT_ERROR_LEVEL, "no enough memory > (cert)");
      goto no_mem;
    }

  if (ic)
    {
      ipk = ic->pkey;
      ix = ic->x509;
    }
  else
    {
      ipk = NULL;
      ix = NULL;
    }

  cert->pkey = __EVP_PKEY_new(ipk);
  if (!cert->pkey)
    {
      SSL_DEBUG(SSL_CERT_ERROR_LEVEL, "__EVP_PKEY_new() return NULL");
      goto pkey_err;
    }

  cert->x509 = __X509_new(ix);
  if (!cert->x509)
    {
      SSL_DEBUG(SSL_CERT_ERROR_LEVEL, "__X509_new() return NULL");
      goto x509_err;
    }

  return cert;

x509_err:
  EVP_PKEY_free(cert->pkey);
pkey_err:
  ssl_mem_free(cert);
no_mem:
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int SSL_CTX_add_client_CA(SSL_CTX *ctx, X509 *x)
{
  SSL_ASSERT1(ctx);
  SSL_ASSERT1(x);
  assert(ctx);
  if (ctx->client_CA == x)
    {
      return 1;
    }

  X509_free(ctx->client_CA);
  ctx->client_CA = x;
  return 1;
}

int SSL_CTX_add_client_CA_ASN1(SSL_CTX *ctx, int len,
                               const unsigned char *d)
{
  X509 *x;

  x = d2i_X509(NULL, &d, len);
  if (!x)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "d2i_X509() return NULL");
      return 0;
    }

  SSL_ASSERT1(ctx);
  X509_free(ctx->client_CA);
  ctx->client_CA = x;

  return 1;
}

CERT *ssl_cert_new(void)
{
  return __ssl_cert_new(NULL);
}

void ssl_cert_free(CERT *cert)
{
  SSL_ASSERT3(cert);

  X509_free(cert->x509);

  EVP_PKEY_free(cert->pkey);

  ssl_mem_free(cert);
}
