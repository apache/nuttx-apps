/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/bio_lib.c
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

#include <openssl/bio.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

BIO *BIO_new(const BIO_METHOD *method)
{
  BIO *bio = zalloc(sizeof(*bio));

  if (bio == NULL)
    {
      return NULL;
    }

  bio->method = method;

  if (method->create != NULL && !method->create(bio))
    {
      free(bio);
      return NULL;
    }

  return bio;
}

BIO *BIO_push(BIO *b, BIO *bio)
{
  BIO *lb;

  if (b == NULL)
    {
      return bio;
    }
  else if (bio == NULL)
    {
      return b;
    }

  lb = b;
  while (lb->next_bio != NULL)
    {
      lb = lb->next_bio;
    }

  lb->next_bio = bio;
  bio->prev_bio = lb;
  return b;
}

void BIO_set_flags(BIO *b, int flags)
{
  b->flags |= flags;
}

int BIO_write(BIO *b, const void *data, int dlen)
{
  if (b == NULL || dlen <= 0)
    {
      return 0;
    }

  if (b->method == NULL || b->method->bwrite_old == NULL)
    {
      return -2;
    }

  return b->method->bwrite_old(b, data, dlen);
}

int BIO_read(BIO *b, void *data, int dlen)
{
  if (b == NULL || dlen <= 0)
    {
      return 0;
    }

  if (b->method == NULL || b->method->bread_old == NULL)
    {
      return -2;
    }

  return b->method->bread_old(b, data, dlen);
}

int BIO_free(BIO *a)
{
  if (a == NULL)
    {
      return 0;
    }

  if (a->method != NULL && a->method->destroy != NULL)
    {
      a->method->destroy(a);
    }

  free(a);
  return 1;
}

void BIO_free_all(BIO *bio)
{
  while (bio != NULL)
    {
      BIO *b = bio;
      bio = bio->next_bio;
      BIO_free(b);
    }
}

BIO *BIO_next(BIO *b)
{
  if (b == NULL)
    {
      return NULL;
    }

  return b->next_bio;
}

int BIO_flush(BIO *b)
{
  return 1;
}
