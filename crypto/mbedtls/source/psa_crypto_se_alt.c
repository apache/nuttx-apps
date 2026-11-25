/****************************************************************************
 * apps/crypto/mbedtls/source/psa_crypto_se_alt.c
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

/* This is a PSA secure element driver that routes the PSA key management
 * operations to the NuttX cryptodev key management interface (CIOCKEY),
 * so keys can be held by a cryptodev key store instead of the PSA local
 * storage.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "psa_crypto_se_alt.h"
#include <psa/crypto.h>
#include <psa/crypto_se_driver.h>

#include "dev_alt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The cryptodev identifies keys by a 32-bit key id. */

#define KEYID_BITS  (sizeof(uint32_t) * 8)
#define KEYLEN_BITS (sizeof(uint32_t) * 8)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Translate a cryptodev error code into a PSA status. */

static psa_status_t psa_alt_map_status(int ret)
{
  switch (ret)
    {
      case 0:
        return PSA_SUCCESS;
      case -EINVAL:
        return PSA_ERROR_INVALID_ARGUMENT;
      case -ENOENT:
        return PSA_ERROR_DOES_NOT_EXIST;
      case -ENOMEM:
        return PSA_ERROR_INSUFFICIENT_MEMORY;
      case -ENOSYS:
      case -ENODEV:
        return PSA_ERROR_NOT_SUPPORTED;
      case -EPERM:
      case -EACCES:
        return PSA_ERROR_NOT_PERMITTED;
      default:
        return PSA_ERROR_GENERIC_ERROR;
    }
}

/* Run one key management operation through the cryptodev.  On success the
 * kernel writes the output parameters back through the pointers stored in
 * kop.
 */

static psa_status_t psa_alt_key_op(FAR const struct crypt_kop *kop)
{
  cryptodev_context_t ctx;
  psa_status_t status;
  int ret;

  ret = cryptodev_init(&ctx);
  if (ret < 0)
    {
      return PSA_ERROR_COMMUNICATION_FAILURE;
    }

  ctx.cryptk = *kop;
  ret = cryptodev_cryptk(&ctx);
  status = psa_alt_map_status(ret);

  cryptodev_free(&ctx);
  return status;
}

/* Generate a key inside the cryptodev key store. */

static psa_status_t psa_alt_generate_key(
                      FAR psa_drv_se_context_t *drv_context,
                      psa_key_slot_number_t key_slot,
                      FAR const psa_key_attributes_t *attributes,
                      FAR uint8_t *pubkey,
                      size_t pubkey_size,
                      FAR size_t *pubkey_length)
{
  struct crypt_kop kop;
  psa_key_type_t type;
  uint32_t keyid = (uint32_t)key_slot;
  uint32_t keylen;
  size_t bits;
  int op;

  (void)drv_context;
  (void)pubkey;
  (void)pubkey_size;

  *pubkey_length = 0;

  type = psa_get_key_type(attributes);
  bits = psa_get_key_bits(attributes);
  switch (type)
    {
      case PSA_KEY_TYPE_AES:
        if (bits != 128 && bits != 192 && bits != 256)
          {
            return PSA_ERROR_NOT_SUPPORTED;
          }

        op = CRK_GENERATE_AES_KEY;
        keylen = (uint32_t)bits / 8;
        break;

      case PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1):
        if (bits != 256)
          {
            return PSA_ERROR_NOT_SUPPORTED;
          }

        op = CRK_GENERATE_SECP256R1_KEY;
        break;

      case PSA_KEY_TYPE_RSA_KEY_PAIR:
        if (bits != 1024)
          {
            return PSA_ERROR_NOT_SUPPORTED;
          }

        op = CRK_GENERATE_RSA_KEY;
        break;

      default:
        return PSA_ERROR_NOT_SUPPORTED;
    }

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = (u_int)op;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;

  /* The kernel expects the AES key length (16/24/32 bytes) as a second
   * input parameter; the asymmetric generators only take the key id.
   */

  if (op == CRK_GENERATE_AES_KEY)
    {
      kop.crk_iparams = 2;
      kop.crk_param[1].crp_p = (caddr_t)&keylen;
      kop.crk_param[1].crp_nbits = KEYLEN_BITS;
    }
  else
    {
      kop.crk_iparams = 1;
    }

  return psa_alt_key_op(&kop);
}

/* Allocate a free key slot in the cryptodev key store. */

static psa_status_t psa_alt_allocate_key(
                      FAR psa_drv_se_context_t *drv_context,
                      FAR void *persistent_data,
                      FAR const psa_key_attributes_t *attributes,
                      psa_key_creation_method_t method,
                      FAR psa_key_slot_number_t *key_slot)
{
  struct crypt_kop kop;
  psa_status_t status;
  uint32_t keyid = 0;

  (void)persistent_data;
  (void)attributes;
  (void)method;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_ALLOCATE_KEY;
  kop.crk_iparams = 0;
  kop.crk_oparams = 1;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;

  status = psa_alt_key_op(&kop);
  if (status == PSA_SUCCESS)
    {
      *key_slot = keyid;
    }

  return status;
}

/* Export the raw key material from the cryptodev key store.  The cryptodev
 * does not report how many bytes the driver wrote, so the buffer size is
 * reported; the PSA core sizes export buffers exactly for the key type.
 */

static psa_status_t psa_alt_export_key(FAR psa_drv_se_context_t *drv_context,
                                       psa_key_slot_number_t key,
                                       FAR uint8_t *p_data,
                                       size_t data_size,
                                       FAR size_t *p_data_length)
{
  struct crypt_kop kop;
  psa_status_t status;
  uint32_t keyid = (uint32_t)key;

  (void)drv_context;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_EXPORT_KEY;
  kop.crk_iparams = 1;
  kop.crk_oparams = 1;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;
  kop.crk_param[1].crp_p = (caddr_t)p_data;
  kop.crk_param[1].crp_nbits = data_size * 8;

  status = psa_alt_key_op(&kop);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  *p_data_length = data_size;
  return PSA_SUCCESS;
}

/* Export the public part of an asymmetric key. */

static psa_status_t psa_alt_export_public_key(
                      FAR psa_drv_se_context_t *drv_context,
                      psa_key_slot_number_t key,
                      FAR uint8_t *p_data,
                      size_t data_size,
                      FAR size_t *p_data_length)
{
  struct crypt_kop kop;
  psa_status_t status;
  uint32_t keyid = (uint32_t)key;

  (void)drv_context;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_EXPORT_PUBLIC_KEY;
  kop.crk_iparams = 1;
  kop.crk_oparams = 1;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;
  kop.crk_param[1].crp_p = (caddr_t)p_data;
  kop.crk_param[1].crp_nbits = data_size * 8;

  status = psa_alt_key_op(&kop);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  *p_data_length = data_size;
  return PSA_SUCCESS;
}

/* Destroy the key bound to a key slot. */

static psa_status_t psa_alt_destroy_key(
                      FAR psa_drv_se_context_t *drv_context,
                      FAR void *persistent_data,
                      psa_key_slot_number_t key_slot)
{
  struct crypt_kop kop;
  uint32_t keyid = (uint32_t)key_slot;

  (void)drv_context;
  (void)persistent_data;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_DELETE_KEY;
  kop.crk_iparams = 1;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;

  return psa_alt_key_op(&kop);
}

/* Import raw key material into the cryptodev key store. */

static psa_status_t psa_alt_import_key(
                      FAR psa_drv_se_context_t *drv_context,
                      psa_key_slot_number_t key_slot,
                      FAR const psa_key_attributes_t *attributes,
                      FAR const uint8_t *data,
                      size_t data_length,
                      FAR size_t *bits)
{
  struct crypt_kop kop;
  psa_status_t status;
  uint32_t keyid = (uint32_t)key_slot;

  (void)drv_context;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_IMPORT_KEY;
  kop.crk_iparams = 2;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;
  kop.crk_param[1].crp_p = (caddr_t)data;
  kop.crk_param[1].crp_nbits = data_length * 8;

  status = psa_alt_key_op(&kop);
  if (status == PSA_SUCCESS)
    {
      *bits = data_length * 8;
    }
  else
    {
      return status;
    }

  /* The key store may keep an imported key in a RAM cache only, in which
   * case a persistent key has to be written back to the key store device
   * too, or it is gone once the cache is.  A volatile key stays in RAM
   * either way, and the save is a no-op for a key the store kept all
   * along.
   */

  if (PSA_KEY_LIFETIME_GET_PERSISTENCE(psa_get_key_lifetime(attributes))
      != PSA_KEY_PERSISTENCE_VOLATILE)
    {
      memset(&kop, 0, sizeof(kop));
      kop.crk_op = CRK_SAVE_KEY;
      kop.crk_iparams = 1;
      kop.crk_oparams = 0;
      kop.crk_param[0].crp_p = (caddr_t)&keyid;
      kop.crk_param[0].crp_nbits = KEYID_BITS;

      status = psa_alt_key_op(&kop);
    }

  return status;
}

/* Check that a key slot is available in the cryptodev key store. */

static psa_status_t psa_alt_validate_slot_number(
                      FAR psa_drv_se_context_t *drv_context,
                      FAR void *persistent_data,
                      FAR const psa_key_attributes_t *attributes,
                      psa_key_creation_method_t method,
                      psa_key_slot_number_t key_slot)
{
  struct crypt_kop kop;
  uint32_t keyid = (uint32_t)key_slot;

  (void)drv_context;
  (void)persistent_data;
  (void)attributes;
  (void)method;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_VALIDATE_KEYID;
  kop.crk_iparams = 1;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = KEYID_BITS;

  return psa_alt_key_op(&kop);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Register the cryptodev key management operations as the secure element
 * driver for the PSA_KEY_LOCATION_LOCAL_CRYPTODEV key location.  The two
 * tables are kept for the lifetime of the registration.
 */

psa_status_t psa_alt_register_se_driver(void)
{
  FAR psa_drv_se_t *drv;
  FAR psa_drv_se_key_management_t *key_management;

  drv = malloc(sizeof(psa_drv_se_t));
  if (drv == NULL)
    {
      return PSA_ERROR_INSUFFICIENT_MEMORY;
    }

  memset(drv, 0, sizeof(psa_drv_se_t));
  drv->MBEDTLS_PRIVATE(hal_version) = PSA_DRV_SE_HAL_VERSION;

  key_management = malloc(sizeof(psa_drv_se_key_management_t));
  if (key_management == NULL)
    {
      free(drv);
      return PSA_ERROR_INSUFFICIENT_MEMORY;
    }

  memset(key_management, 0, sizeof(psa_drv_se_key_management_t));
  key_management->MBEDTLS_PRIVATE(p_allocate) = psa_alt_allocate_key;
  key_management->MBEDTLS_PRIVATE(p_validate_slot_number) =
    psa_alt_validate_slot_number;
  key_management->MBEDTLS_PRIVATE(p_import) = psa_alt_import_key;
  key_management->MBEDTLS_PRIVATE(p_generate) = psa_alt_generate_key;
  key_management->MBEDTLS_PRIVATE(p_destroy) = psa_alt_destroy_key;
  key_management->MBEDTLS_PRIVATE(p_export) = psa_alt_export_key;
  key_management->MBEDTLS_PRIVATE(p_export_public) =
    psa_alt_export_public_key;
  drv->MBEDTLS_PRIVATE(key_management) = key_management;

  return psa_register_se_driver(PSA_KEY_LOCATION_LOCAL_CRYPTODEV, drv);
}
