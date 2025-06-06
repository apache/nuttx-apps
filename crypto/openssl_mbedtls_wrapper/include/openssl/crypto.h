/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/crypto.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_CRYPTO_H
#define OPENSSL_MBEDTLS_WRAPPER_CRYPTO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CRYPTO_num_locks()      1
#define CRYPTO_set_locking_callback(func)

#define CRYPTO_set_id_callback(func)

/* These defines where used in combination with the old locking callbacks,
 * they are not called anymore, but old code that's not called might still
 * use them.
 */

#define CRYPTO_LOCK             1
#define CRYPTO_UNLOCK           2
#define CRYPTO_READ             4
#define CRYPTO_WRITE            8

#endif /* OPENSSL_MBEDTLS_WRAPPER_CRYPTO_H */
