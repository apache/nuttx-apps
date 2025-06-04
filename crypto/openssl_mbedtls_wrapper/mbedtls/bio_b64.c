/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/bio_b64.c
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

#include <stdio.h>
#include <sys/param.h>

#include <mbedtls/base64.h>
#include <openssl/bio.h>
#include <openssl/types.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

#define BIO_B64_BUFSIZE ((BIO_B64_ENC_LEN / 3) * 4 + 1)
#define BIO_B64_ENC_LEN 192

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int b64_write(BIO *h, const char *buf, int num);
static int b64_read(BIO *h, char *buf, int size);
static int b64_new(BIO *h);
static int b64_free(BIO *data);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const BIO_METHOD g_methods_b64 =
{
  BIO_TYPE_BASE64,
  "base64 encoding",
  b64_write,
  b64_read,
  b64_new,
  b64_free,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int b64_write(BIO *b, const char *in, int inl)
{
  BIO *next;
  int len;
  int offset = 0;
  int ret = 0;
  size_t n;
  unsigned char out[BIO_B64_BUFSIZE];

  next = BIO_next(b);
  if (next == NULL)
    {
      return 0;
    }

  while (inl > 0)
    {
      len = MIN(inl, BIO_B64_ENC_LEN);
      ret = mbedtls_base64_encode(out, BIO_B64_BUFSIZE, &n,
                                  (const unsigned char *)in + offset, len);
      if (ret < 0)
        {
          break;
        }

      ret = BIO_write(next, out, n);
      inl -= len;
      offset += len;
    }

  return ret;
}

static int b64_read(BIO *b, char *out, int outl)
{
  BIO *next;
  int ret;
  size_t n;

  if (out == NULL)
    {
      return 0;
    }

  next = BIO_next(b);
  if (next == NULL)
    {
      return 0;
    }

  ret = BIO_read(next, out, outl);
  if (ret > 0)
    {
      ret = mbedtls_base64_decode((unsigned char *)out, outl, &n,
                                  (const unsigned char *)out, ret);
      if (ret == 0)
        {
          ret = n;
        }
    }

  return ret;
}

static int b64_new(BIO *bi)
{
  return 1;
}

static int b64_free(BIO *a)
{
  return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

const BIO_METHOD *BIO_f_base64(void)
{
  return &g_methods_b64;
}
