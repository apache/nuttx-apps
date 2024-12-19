/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/x509.c
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
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/ssl_dbg.h>
#include <openssl/x509.h>
#include "ssl_port.h"
#include "ssl_methods.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void X509_free(X509 *a)
{
  SSL_ASSERT3(a);

  X509_METHOD_CALL(free, a);

  ssl_mem_free(a);
}

void X509_EXTENSION_free(X509_EXTENSION *a)
{
}

int X509_set_notAfter(X509 *x509, const ASN1_TIME *tm)
{
  return 0;
}

void X509_NAME_free(X509_NAME *a)
{
}

void X509_ALGOR_free(X509_ALGOR *a)
{
}

int X509_sign(X509 *x509, EVP_PKEY *pkey, const EVP_MD *md)
{
  return 0;
}

int X509_add_ext(X509 *x, const X509_EXTENSION *ex, int loc)
{
  return 0;
}

int X509_set_pubkey(X509 *x509, EVP_PKEY *pkey)
{
  return 0;
}

X509_EXTENSION *X509_EXTENSION_create_by_NID(X509_EXTENSION **ex,
                                             int nid, int crit,
                                             const ASN1_OCTET_STRING *data)
{
  return NULL;
}

X509 *X509_new(void)
{
  return __X509_new(NULL);
}

int X509_set_version(X509 *x509, long version)
{
  return 0;
}

int X509_set_serialNumber(X509 *x509, const ASN1_INTEGER *serial)
{
  return 0;
}

int X509_set_subject_name(X509 *x509, X509_NAME *name)
{
  return 0;
}

int X509_set_issuer_name(X509 *x509, X509_NAME *name)
{
  return 0;
}

int X509_set_notBefore(X509 *x509, const ASN1_TIME *tm)
{
  return 0;
}

int X509_ALGOR_set0(X509_ALGOR *alg, ASN1_OBJECT *obj,
                    int param_type, void *param_value)
{
  return 0;
}

int X509_set1_signature_algo(X509 *x509, const X509_ALGOR *algo)
{
  return 0;
}

int X509_set1_signature_value(X509 *x509,
                              const uint8_t *sig,
                              size_t sig_len)
{
  return 0;
}

X509_NAME *X509_NAME_new(void)
{
  return NULL;
}

int X509_NAME_add_entry_by_txt(X509_NAME *name, const char *field,
                               int type, const uint8_t *bytes,
                               int len, int loc, int set)
{
  return 0;
}

X509_NAME *d2i_X509_NAME(X509_NAME **out, const uint8_t **inp, long len)
{
  return NULL;
}

X509_ALGOR *X509_ALGOR_new(void)
{
  return NULL;
}

int i2d_X509(X509 *x509, uint8_t **outp)
{
  return 0;
}

PKCS8_PRIV_KEY_INFO *d2i_PKCS8_PRIV_KEY_INFO(PKCS8_PRIV_KEY_INFO *info,
                                             const uint8_t **key_data,
                                             size_t key_length)
{
  return NULL;
}

void PKCS8_PRIV_KEY_INFO_free(PKCS8_PRIV_KEY_INFO *key)
{
}

EVP_PKEY *EVP_PKCS82PKEY(const PKCS8_PRIV_KEY_INFO *p8)
{
  return NULL;
}

X509 *d2i_X509(X509 **out, const unsigned char **inp, long len)
{
  int m = 0;
  int ret;
  X509 *x;

  SSL_ASSERT2(inp);
  SSL_ASSERT2(len);

  if (out && *out)
    {
      x = *out;
    }
  else
    {
      x = X509_new();
      if (!x)
        {
          SSL_DEBUG(SSL_PKEY_ERROR_LEVEL, "X509_new() return NULL");
          goto failed1;
        }

      m = 1;
    }

  ret = X509_METHOD_CALL(load, x, *inp, (int)len);
  if (ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL,
                "X509_METHOD_CALL(load) return %d", ret);
      goto failed2;
    }

  return x;

failed2:
  if (m)
    {
      X509_free(x);
    }

failed1:
  return NULL;
}

X509 *__X509_new(X509 *ix)
{
  int ret;
  X509 *x;

  x = ssl_mem_zalloc(sizeof(X509));
  if (!x)
    {
      SSL_DEBUG(SSL_X509_ERROR_LEVEL, "no enough memory > (x)");
      goto no_mem;
    }

  if (ix)
    {
      x->method = ix->method;
    }
  else
    {
      x->method = X509_method();
    }

  ret = X509_METHOD_CALL(new, x, ix);
  if (ret)
    {
      SSL_DEBUG(SSL_PKEY_ERROR_LEVEL,
                "X509_METHOD_CALL(new) return %d", ret);
      goto failed;
    }

  return x;

failed:
  ssl_mem_free(x);
no_mem:
  return NULL;
}
