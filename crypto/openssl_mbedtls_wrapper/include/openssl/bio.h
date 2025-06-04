/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/bio.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_BIO_H
#define OPENSSL_MBEDTLS_WRAPPER_BIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>
#include <openssl/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* There are the classes of BIOs */

#define BIO_TYPE_DESCRIPTOR     0x0100 /* socket, fd, connect or accept */
#define BIO_TYPE_FILTER         0x0200
#define BIO_TYPE_SOURCE_SINK    0x0400

#define BIO_FLAGS_BASE64_NO_NL  0x100

/* This is used with memory BIOs: BIO_FLAGS_MEM_RDONLY
 * means we shouldn't free up or change the data in any way;
 * BIO_FLAGS_NONCLEAR_RST means we shouldn't clear data on reset.
 */

#define BIO_FLAGS_MEM_RDONLY    0x200
#define BIO_FLAGS_NONCLEAR_RST  0x400
#define BIO_FLAGS_IN_EOF        0x800

#define BIO_TYPE_NONE           0
#define BIO_TYPE_MEM           (1 | BIO_TYPE_SOURCE_SINK)
#define BIO_TYPE_FILE          (2 | BIO_TYPE_SOURCE_SINK)
#define BIO_TYPE_BASE64        (11 | BIO_TYPE_FILTER)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct bio_method_st
{
  int type;
  char *name;
  int (*bwrite_old) (BIO *, const char *, int);
  int (*bread_old) (BIO *, char *, int);
  int (*create) (BIO *);
  int (*destroy) (BIO *);
};

struct bio_st
{
  const BIO_METHOD *method;
  int flags;                  /* extra storage */
  void *ptr;
  struct bio_st *next_bio;    /* used by filter BIOs */
  struct bio_st *prev_bio;    /* used by filter BIOs */
};

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

const BIO_METHOD *BIO_f_base64(void);

const BIO_METHOD *BIO_s_mem(void);

BIO *BIO_new(const BIO_METHOD *method);

BIO *BIO_push(BIO *b, BIO *bio);

void BIO_set_flags(BIO *b, int flags);

int BIO_write(BIO *b, const void *data, int dlen);

int BIO_read(BIO *b, void *data, int dlen);

void BIO_free_all(BIO *bio);

BIO *BIO_next(BIO *b);

int BIO_flush(BIO *b);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_BIO_H */
