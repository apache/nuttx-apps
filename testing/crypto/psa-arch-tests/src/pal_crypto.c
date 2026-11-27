/****************************************************************************
 * apps/testing/crypto/psa-arch-tests/src/pal_crypto.c
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

#include <pal_crypto_config.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

psa_status_t psa_key_agreement(psa_key_id_t private_key,
                               FAR const uint8_t *peer_key,
                               size_t peer_key_length,
                               psa_algorithm_t alg,
                               FAR const psa_key_attributes_t *attributes,
                               FAR psa_key_id_t *key)
{
  return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_pake_set_context(FAR psa_pake_operation_t *operation,
                                  FAR const uint8_t *context,
                                  size_t context_len)
{
  return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_pake_get_shared_key(
                  FAR psa_pake_operation_t *operation,
                  FAR const psa_key_attributes_t *attributes,
                  FAR psa_key_id_t *key)
{
  return PSA_ERROR_NOT_SUPPORTED;
}

void psa_pake_cs_set_key_confirmation(
                     FAR psa_pake_cipher_suite_t *cipher_suite,
                     uint32_t key_confirmation)
{
}

psa_status_t psa_key_derivation_verify_bytes(
                     FAR psa_key_derivation_operation_t *operation,
                     FAR const uint8_t *expected_output,
                     size_t output_length)
{
  return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_key_derivation_verify_key(
                     FAR psa_key_derivation_operation_t *operation,
                     psa_key_id_t expected)
{
  return PSA_ERROR_NOT_SUPPORTED;
}
