/****************************************************************************
 * apps/crypto/mbedtls/include/aes_alt.h
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

#ifndef __APPS_CRYPTO_MBEDTLS_INCLUDE_AES_ALT_H
#define __APPS_CRYPTO_MBEDTLS_INCLUDE_AES_ALT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dev_alt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_KEY_SIZE      64

typedef struct mbedtls_aes_context
{
  cryptodev_context_t dev;
  unsigned char key[MAX_KEY_SIZE];
}
mbedtls_aes_context;

#define mbedtls_aes_xts_context mbedtls_aes_context

#endif /* __APPS_CRYPTO_MBEDTLS_INCLUDE_AES_ALT_H */
