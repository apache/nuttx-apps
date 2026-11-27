/****************************************************************************
 * apps/crypto/mbedtls/source/psa_crypto_se_alt.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* This is a PSA secure element driver that routes the PSA key management
 * operations to the NuttX cryptodev key management interface (CIOCKEY),
 * so keys can be held by a cryptodev key store instead of the PSA local
 * storage.  Only AES (128/192/256 bit) keys are supported: the in-tree
 * software backend implements CRK_GENERATE_AES_KEY, CRK_IMPORT_KEY and
 * CRK_EXPORT_KEY for those, but its asymmetric generators need a second
 * key id parameter that the PSA single-slot model does not provide, and
 * it has no CRK_EXPORT_PUBLIC_KEY.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "psa_crypto_se_alt.h"
#include <psa/crypto.h>
#include <psa/crypto_se_driver.h>

#include "dev_alt.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Check the key attributes against what this driver supports, before any
 * key id is reserved.  The PSA core calls p_allocate first; rejecting
 * here avoids burning a key id on a key that can never be created.
 */

static psa_status_t psa_alt_check_attributes(
                      FAR const psa_key_attributes_t *attributes)
{
  psa_key_type_t type = psa_get_key_type(attributes);
  size_t bits = psa_get_key_bits(attributes);

  if (type == PSA_KEY_TYPE_AES)
    {
      if (bits != 128 && bits != 192 && bits != 256)
        {
          return PSA_ERROR_NOT_SUPPORTED;
        }
    }
  else
    {
      return PSA_ERROR_NOT_SUPPORTED;
    }

  return PSA_SUCCESS;
}

/* Translate a cryptodev error code into a PSA status. */

static psa_status_t psa_alt_map_status(int ret)
{
  switch (ret)
    {
      case 0:
        return PSA_SUCCESS;
      case -EINVAL:
        return PSA_ERROR_INVALID_ARGUMENT;
      case -EEXIST:
        return PSA_ERROR_ALREADY_EXISTS;
      case -ENOENT:
        return PSA_ERROR_DOES_NOT_EXIST;
      case -ENOMEM:
        return PSA_ERROR_INSUFFICIENT_MEMORY;
      case -ENOSYS:
      case -ENODEV:
        return PSA_ERROR_NOT_SUPPORTED;
      case -ENOBUFS:
        return PSA_ERROR_BUFFER_TOO_SMALL;
      case -EFBIG:
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

static psa_status_t psa_alt_key_op(FAR struct crypt_kop *kop)
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

/* Best effort: release the key id after a failed creation.  The PSA core
 * does not call p_destroy on a failed creation (mbed-crypto#217), so
 * without this a generate or import error would leave the id reserved
 * in the store for good.
 */

static void psa_alt_release_keyid(uint32_t keyid)
{
  struct crypt_kop kop;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_DELETE_KEY;
  kop.crk_iparams = 1;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;

  psa_alt_key_op(&kop);
}

/* Reject slot numbers the cryptodev cannot represent, so an application
 * asking for slot 0x100000001 does not silently get slot 1.  This is
 * called from both p_validate_slot_number and the import/generate paths
 * that carry an explicit slot.
 */

static psa_status_t psa_alt_check_slot(psa_key_slot_number_t key_slot)
{
  if (key_slot > (psa_key_slot_number_t)UINT32_MAX)
    {
      return PSA_ERROR_INVALID_ARGUMENT;
    }

  return PSA_SUCCESS;
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
  psa_status_t status;
  uint32_t keyid = (uint32_t)key_slot;
  uint32_t keylen;
  size_t bits;

  (void)drv_context;
  (void)pubkey;
  (void)pubkey_size;

  *pubkey_length = 0;

  status = psa_alt_check_slot(key_slot);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  status = psa_alt_check_attributes(attributes);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  bits = psa_get_key_bits(attributes);
  keylen = (uint32_t)bits / 8;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_GENERATE_AES_KEY;
  kop.crk_iparams = 2;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;
  kop.crk_param[1].crp_p = (caddr_t)&keylen;
  kop.crk_param[1].crp_nbits = sizeof(uint32_t) * 8;

  return psa_alt_key_op(&kop);
}

/* Allocate a free key slot in the cryptodev key store.  Unsupported key
 * attributes are rejected here, before the id is reserved.
 */

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
  (void)method;

  (void)drv_context;

  status = psa_alt_check_attributes(attributes);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_ALLOCATE_KEY;
  kop.crk_iparams = 0;
  kop.crk_oparams = 1;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;

  status = psa_alt_key_op(&kop);
  if (status == PSA_SUCCESS)
    {
      *key_slot = keyid;
    }

  return status;
}

/* Export the raw key material from the cryptodev key store.  The kernel
 * updates crp_nbits of the output param with the number of bytes it
 * wrote; the PSA core passes the caller's buffer size as data_size, which
 * is an upper bound, not the key size.
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

  status = psa_alt_check_slot(key);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_EXPORT_KEY;
  kop.crk_iparams = 1;
  kop.crk_oparams = 1;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;
  kop.crk_param[1].crp_p = (caddr_t)p_data;
  kop.crk_param[1].crp_nbits = data_size * 8;

  status = psa_alt_key_op(&kop);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  /* The kernel writes back the actual byte count in crp_nbits. */

  *p_data_length = kop.crk_param[1].crp_nbits / 8;
  return PSA_SUCCESS;
}

/* Export the public part of an asymmetric key.  Not supported: the
 * in-tree software backend has no CRK_EXPORT_PUBLIC_KEY.
 */

static psa_status_t psa_alt_export_public_key(
                      FAR psa_drv_se_context_t *drv_context,
                      psa_key_slot_number_t key,
                      FAR uint8_t *p_data,
                      size_t data_size,
                      FAR size_t *p_data_length)
{
  (void)drv_context;
  (void)key;
  (void)p_data;
  (void)data_size;
  (void)p_data_length;

  return PSA_ERROR_NOT_SUPPORTED;
}

/* Destroy the key bound to a key slot. */

static psa_status_t psa_alt_destroy_key(
                      FAR psa_drv_se_context_t *drv_context,
                      FAR void *persistent_data,
                      psa_key_slot_number_t key_slot)
{
  struct crypt_kop kop;
  uint32_t keyid;

  (void)drv_context;
  (void)persistent_data;

  if (psa_alt_check_slot(key_slot) != PSA_SUCCESS)
    {
      return PSA_ERROR_INVALID_ARGUMENT;
    }

  keyid = (uint32_t)key_slot;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_DELETE_KEY;
  kop.crk_iparams = 1;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;

  return psa_alt_key_op(&kop);
}

/* Import raw key material into the cryptodev key store.  Only raw AES
 * keys of a supported size are accepted; the PSA core checks bits
 * consistency after p_import returns, so a mismatch would have already
 * written the key by then.
 */

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
  psa_key_type_t type;
  uint32_t keyid;
  int persistent;

  (void)drv_context;

  status = psa_alt_check_slot(key_slot);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  keyid = (uint32_t)key_slot;

  /* Validate the key material against the attributes: the type must be
   * AES and the length must match the declared bit count.  Doing this
   * before the import avoids writing a key the PSA core would reject
   * afterwards (bits mismatch) and leaves nothing behind.
   */

  type = psa_get_key_type(attributes);
  *bits = psa_get_key_bits(attributes);
  if (type != PSA_KEY_TYPE_AES || data_length != (*bits + 7) / 8 ||
      (data_length != 16 && data_length != 24 && data_length != 32))
    {
      return PSA_ERROR_NOT_SUPPORTED;
    }

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_IMPORT_KEY;
  kop.crk_iparams = 2;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;
  kop.crk_param[1].crp_p = (caddr_t)data;
  kop.crk_param[1].crp_nbits = data_length * 8;

  status = psa_alt_key_op(&kop);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  /* The key store may keep an imported key in a RAM cache only, in which
   * case a persistent key has to be written back to the key store device
   * too, or it is gone once the cache is.  A volatile key stays in RAM
   * either way, and the save is a no-op for a key the store kept all
   * along.
   */

  persistent = PSA_KEY_LIFETIME_GET_PERSISTENCE(
      psa_get_key_lifetime(attributes)) != PSA_KEY_PERSISTENCE_VOLATILE;
  if (persistent)
    {
      memset(&kop, 0, sizeof(kop));
      kop.crk_op = CRK_SAVE_KEY;
      kop.crk_iparams = 1;
      kop.crk_oparams = 0;
      kop.crk_param[0].crp_p = (caddr_t)&keyid;
      kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;

      status = psa_alt_key_op(&kop);
    }

  if (status != PSA_SUCCESS)
    {
      psa_alt_release_keyid(keyid);
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
  psa_status_t status;
  uint32_t keyid;

  (void)drv_context;
  (void)persistent_data;
  (void)method;

  status = psa_alt_check_slot(key_slot);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  status = psa_alt_check_attributes(attributes);
  if (status != PSA_SUCCESS)
    {
      return status;
    }

  keyid = (uint32_t)key_slot;

  memset(&kop, 0, sizeof(kop));
  kop.crk_op = CRK_VALIDATE_KEYID;
  kop.crk_iparams = 1;
  kop.crk_oparams = 0;
  kop.crk_param[0].crp_p = (caddr_t)&keyid;
  kop.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;

  return psa_alt_key_op(&kop);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* The driver description, as static const tables: no heap, no leak on a
 * failed registration, and the same object can be reused.
 */

static const psa_drv_se_key_management_t psa_alt_key_management =
{
  .MBEDTLS_PRIVATE(p_allocate) = psa_alt_allocate_key,
  .MBEDTLS_PRIVATE(p_validate_slot_number) = psa_alt_validate_slot_number,
  .MBEDTLS_PRIVATE(p_import) = psa_alt_import_key,
  .MBEDTLS_PRIVATE(p_generate) = psa_alt_generate_key,
  .MBEDTLS_PRIVATE(p_destroy) = psa_alt_destroy_key,
  .MBEDTLS_PRIVATE(p_export) = psa_alt_export_key,
  .MBEDTLS_PRIVATE(p_export_public) = psa_alt_export_public_key,
};

static const psa_drv_se_t psa_alt_driver =
{
  .MBEDTLS_PRIVATE(hal_version) = PSA_DRV_SE_HAL_VERSION,
  .MBEDTLS_PRIVATE(key_management) = &psa_alt_key_management,
};

/* Register the cryptodev key management operations as the secure element
 * driver for the PSA_KEY_LOCATION_LOCAL_CRYPTODEV key location.
 */

psa_status_t psa_alt_register_se_driver(void)
{
  return psa_register_se_driver(PSA_KEY_LOCATION_LOCAL_CRYPTODEV,
                                &psa_alt_driver);
}
