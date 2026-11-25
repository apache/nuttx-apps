/****************************************************************************
 * apps/crypto/mbedtls/source/pkcs5_alt.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"
#include "mbedtls/error.h"

#include <string.h>

#include "dev_alt.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pkcs5_alt_mac_from_md
 *
 * Description:
 *   Map an mbedtls digest type to the matching cryptodev PBKDF2 MAC
 *   algorithm. Only SHA1 and SHA256 are backed by the hardware engine;
 *   every other digest is reported as unsupported.
 *
 ****************************************************************************/

static int pkcs5_alt_mac_from_md(mbedtls_md_type_t md_alg, FAR uint32_t *mac)
{
  switch (md_alg)
    {
      case MBEDTLS_MD_SHA1:
        *mac = CRYPTO_PBKDF2_HMAC_SHA1;
        return 0;
      case MBEDTLS_MD_SHA256:
        *mac = CRYPTO_PBKDF2_HMAC_SHA256;
        return 0;
      default:
        return MBEDTLS_ERR_PKCS5_FEATURE_UNAVAILABLE;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_pkcs5_pbkdf2_hmac_ext(mbedtls_md_type_t md_alg,
                                  FAR const unsigned char *password,
                                  size_t plen,
                                  FAR const unsigned char *salt, size_t slen,
                                  unsigned int iteration_count,
                                  uint32_t key_length,
                                  FAR unsigned char *output)
{
  uint8_t keybuf[HMAC_MAX_BLOCK_LEN];
  cryptodev_context_t ctx;
  uint32_t mac;
  int ret;

  ret = pkcs5_alt_mac_from_md(md_alg, &mac);
  if (ret != 0)
    {
      return ret;
    }

  /* The HMAC key (password) is loaded into a fixed CSS key slot, so it must
   * fit the largest key the driver can accept. Longer passwords are not
   * supported by this hardware path.
   */

  if (plen > sizeof(keybuf))
    {
      return MBEDTLS_ERR_PKCS5_FEATURE_UNAVAILABLE;
    }

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      return MBEDTLS_ERR_PKCS5_BAD_INPUT_DATA;
    }

  /* Copy the (possibly const) password into a writable buffer: the kernel
   * HMAC session setup XORs the key in place (ipad/opad).
   */

  memcpy(keybuf, password, plen);

  ctx.session.cipher = 0;
  ctx.session.mac = mac;
  ctx.session.mackey = (caddr_t)keybuf;
  ctx.session.mackeylen = plen;

  ret = cryptodev_get_session(&ctx);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_PKCS5_FEATURE_UNAVAILABLE;
      goto errout;
    }

  ctx.crypt.ses = ctx.session.ses;
  ctx.crypt.op = COP_ENCRYPT;
  ctx.crypt.src = (caddr_t)salt;
  ctx.crypt.len = slen;
  ctx.crypt.mac = (caddr_t)output;
  ctx.crypt.olen = key_length;
  ctx.crypt.iterations = iteration_count;

  ret = cryptodev_crypt(&ctx);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_PKCS5_BAD_INPUT_DATA;
    }

  cryptodev_free_session(&ctx);

errout:
  memset(keybuf, 0, sizeof(keybuf));
  cryptodev_free(&ctx);
  return ret;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
int mbedtls_pkcs5_pbkdf2_hmac(FAR mbedtls_md_context_t *ctx,
                              FAR const unsigned char *password,
                              size_t plen,
                              FAR const unsigned char *salt, size_t slen,
                              unsigned int iteration_count,
                              uint32_t key_length,
                              FAR unsigned char *output)
{
  mbedtls_md_type_t md_alg;

  if (ctx == NULL || ctx->MBEDTLS_PRIVATE(md_info) == NULL)
    {
      return MBEDTLS_ERR_PKCS5_BAD_INPUT_DATA;
    }

  md_alg = mbedtls_md_get_type(ctx->MBEDTLS_PRIVATE(md_info));
  return mbedtls_pkcs5_pbkdf2_hmac_ext(md_alg, password, plen, salt, slen,
                                       iteration_count, key_length, output);
}
#endif /* !MBEDTLS_DEPRECATED_REMOVED */
