/****************************************************************************
 * apps/crypto/mbedtls/include/psa_crypto_se_alt.h
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

#ifndef __APPS_CRYPTO_MBEDTLS_INCLUDE_PSA_CRYPTO_SE_ALT_H
#define __APPS_CRYPTO_MBEDTLS_INCLUDE_PSA_CRYPTO_SE_ALT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <psa/crypto.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The PSA key location served by the secure element driver registered by
 * this module.  Keys whose lifetime places them at this location are
 * managed through the NuttX cryptodev key management interface.
 */

#define PSA_KEY_LOCATION_LOCAL_CRYPTODEV 1

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Register the cryptodev secure element driver with the PSA core.  Call
 * this before psa_crypto_init().
 */

psa_status_t psa_alt_register_se_driver(void);

#endif /* __APPS_CRYPTO_MBEDTLS_INCLUDE_PSA_CRYPTO_SE_ALT_H */
