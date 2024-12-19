/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/rsa.c
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

#include <mbedtls/rsa.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

unsigned RSA_size(const RSA *rsa)
{
  return (mbedtls_rsa_get_len((const mbedtls_rsa_context *)rsa) + 7) / 8;
}

const BIGNUM *RSA_get0_e(const RSA *rsa)
{
  return NULL;
}

RSA *RSA_new(void)
{
  return NULL;
}

void RSA_free(RSA *rsa)
{
}

int RSA_generate_key_ex(RSA *rsa, int bits,
                        const BIGNUM *e_value, BN_GENCB *cb)
{
  return 0;
}

int RSA_private_encrypt(size_t flen, const uint8_t *from,
                        uint8_t *to, RSA *rsa, int padding)
{
  return 0;
}

int RSA_public_decrypt(size_t flen, const uint8_t *from,
                       uint8_t *to, RSA *rsa, int padding)
{
  return 0;
}
