/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/bss_mem.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include <openssl/bio.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

/* LIMIT_BEFORE_EXPANSION is the maximum n such that (n+3)/3*4 < 2**31. That
 * function is applied in several functions in this file and this limit
 * ensures that the result fits in an int.
 */

#define LIMIT_BEFORE_EXPANSION 0x5ffffffc

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int mem_write(BIO *h, const char *buf, int num);
static int mem_read(BIO *h, char *buf, int size);
static int mem_new(BIO *h);
static int mem_free(BIO *data);

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct bio_buf_mem_st
{
  unsigned long flags;
  char *data;
  size_t length;              /* current number of bytes */
  size_t max;                 /* size of buffer */
  size_t offset;              /* has been read */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const BIO_METHOD g_mem_method =
{
  BIO_TYPE_MEM,
  "memory buffer",
  mem_write,
  mem_read,
  mem_new,
  mem_free,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mem_buf_sync(BIO *b)
{
  if (b != NULL && b->ptr != NULL)
    {
      BIO_BUF_MEM *bbm = (BIO_BUF_MEM *)b->ptr;

      if (bbm->offset)
        {
          bbm->length = bbm->length - bbm->offset;
          memmove(bbm->data, bbm->data + bbm->offset, bbm->length);
          bbm->offset = 0;
        }
    }

  return 0;
}

static size_t BUF_MEM_grow_clean(BIO_BUF_MEM *bbm, size_t len)
{
  char *ret;
  size_t n;

  if (bbm->length >= len)
    {
      if (bbm->data != NULL)
        {
          memset(&bbm->data[len], 0, bbm->length - len);
        }

      bbm->length = len;
      return len;
    }

  if (bbm->max >= len)
    {
      memset(&bbm->data[bbm->length], 0, len - bbm->length);
      bbm->length = len;
      return len;
    }

  /* This limit is sufficient to ensure (len+3)/3*4 < 2**31 */

  if (len > LIMIT_BEFORE_EXPANSION)
    {
      return 0;
    }

  n = MAX((len + 3) / 3 * 4, bbm->max);
  ret = realloc(bbm->data, n);
  if (ret == NULL)
    {
      return 0;
    }

  bbm->data = ret;
  bbm->max = n;
  bbm->length = len;
  return len;
}

static int mem_write(BIO *b, const char *in, int inl)
{
  BIO_BUF_MEM *bbm = (BIO_BUF_MEM *)b->ptr;
  int blen;

  if (b->flags & BIO_FLAGS_MEM_RDONLY)
    {
      return -1;
    }

  if (inl <= 0)
    {
      return 0;
    }
  else if (in == NULL)
    {
      return -1;
    }

  blen = bbm->length - bbm->offset;
  mem_buf_sync(b);
  if (BUF_MEM_grow_clean(bbm, blen + inl) == 0)
    {
      return -1;
    }

  memcpy(bbm->data + blen, in, inl);
  return inl;
}

static int mem_read(BIO *b, char *out, int outl)
{
  BIO_BUF_MEM *bbm = (BIO_BUF_MEM *)b->ptr;

  if (outl <= 0)
    {
      return 0;
    }

  if (out == NULL)
    {
      return -1;
    }

  if (outl > bbm->length - bbm->offset)
    {
      outl = bbm->length - bbm->offset;
    }

  memcpy(out, bbm->data + bbm->offset, outl);
  bbm->offset += outl;
  return outl;
}

static int mem_new(BIO *bi)
{
  BIO_BUF_MEM *bbm = zalloc(sizeof(*bbm));

  if (bbm == NULL)
    {
      return 0;
    }

  bi->ptr = bbm;
  return 1;
}

static int mem_free(BIO *a)
{
  BIO_BUF_MEM *bbm;

  if (a == NULL)
    {
      return 0;
    }

  bbm = (BIO_BUF_MEM *)a->ptr;
  if (bbm->data)
    {
      free(bbm->data);
    }

  free(bbm);
  return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

const BIO_METHOD *BIO_s_mem(void)
{
  return &g_mem_method;
}
