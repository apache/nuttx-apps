/****************************************************************************
 * apps/crypto/mbedtls/include/cmac_alt.h
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

#ifndef __APPS_CRYPTO_MBEDTLS_INCLUDE_CMAC_ALT_H
#define __APPS_CRYPTO_MBEDTLS_INCLUDE_CMAC_ALT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dev_alt.h"

#define CMAC_KEY_MAX_SIZE 32

struct mbedtls_cmac_context_t
{
    cryptodev_context_t dev;
    unsigned char key[CMAC_KEY_MAX_SIZE];
    uint32_t keybits;
    uint32_t cipher_type;
    uint32_t mac_type;
};

#endif /* __APPS_CRYPTO_MBEDTLS_INCLUDE_CMAC_ALT_H */
