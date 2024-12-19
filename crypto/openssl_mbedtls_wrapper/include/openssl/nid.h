/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/nid.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_NID_H
#define OPENSSL_MBEDTLS_WRAPPER_NID_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NID_rsaEncryption 6
#define NID_commonName 13
#define NID_key_usage 83
#define NID_X9_62_id_ecPublicKey 408
#define NID_X9_62_prime256v1 415
#define NID_sha256WithRSAEncryption 668
#define NID_secp224r1 713
#define NID_secp384r1 715
#define NID_secp521r1 716
#define NID_X25519 948
#define NID_ED25519 949

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_NID_H */
