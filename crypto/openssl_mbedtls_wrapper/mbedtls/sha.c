/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/sha.c
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

#include <mbedtls/sha1.h>
#include <openssl/sha.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int SHA1_Init(SHA_CTX *sha)
{
  mbedtls_sha1_init((mbedtls_sha1_context *)sha);
  mbedtls_sha1_starts((mbedtls_sha1_context *)sha);
  return 0;
}

int SHA1_Update(SHA_CTX *sha, const void *data, size_t len)
{
  return mbedtls_sha1_update((mbedtls_sha1_context *)sha, data, len);
}

int SHA1_Final(uint8_t out[SHA_DIGEST_LENGTH], SHA_CTX *sha)
{
  return mbedtls_sha1_finish((mbedtls_sha1_context *)sha,
                             (unsigned char *)out);
}

uint8_t *SHA1(const uint8_t *data, size_t len,
              uint8_t out[SHA_DIGEST_LENGTH])
{
  if (mbedtls_sha1(data, len, out) != 0)
    {
      return NULL;
    }

  return out;
}

int SHA256_Init(SHA256_CTX *sha)
{
  return 0;
}

int SHA256_Update(SHA256_CTX *sha, const void *data, size_t len)
{
  return 0;
}

int SHA256_Final(uint8_t out[SHA256_DIGEST_LENGTH], SHA256_CTX *sha)
{
  return 0;
}
