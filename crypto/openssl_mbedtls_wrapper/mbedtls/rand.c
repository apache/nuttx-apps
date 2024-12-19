/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/rand.c
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

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <openssl/rand.h>
#include <string.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* RAND_bytes() generates num random bytes using a
 * cryptographically secure pseudo random generator
 * (CSPRNG) and stores them in buf.
 *
 * @param buf the buffer to contains the generated random data
 * @param num the length of the generated random data
 * @return return 1 on success, other value on failure
 */

int RAND_bytes(unsigned char *buf, int num)
{
  int ret;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  const char *keymaster_personalization =
    "softkeymaster_random_generator";
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
    &entropy, (const unsigned char *)keymaster_personalization,
    strlen(keymaster_personalization)) == 0
    && mbedtls_ctr_drbg_random(&ctr_drbg, buf, num) == 0 ? 1 : 0;
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
  return ret;
}

/* RAND_add method using to seed the random generator
 * This function also do nothing on android
 * https://github.com/google/boringssl/blob/
 * 94b477cea5057d9372984a311aba9276f737f748/
 * crypto/rand_extra/rand_extra.c#L39
 */

void RAND_add(const void *buf, int num, double randomness)
{
}
