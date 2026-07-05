/****************************************************************************
 * apps/netutils/dropbear/port/dropbear_ltc_hmac_sha256.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/* LibTomCrypt-compatible HMAC API backed by NuttX crypto/hmac.  The NuttX
 * Dropbear configuration only enables hmac-sha2-256.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "includes.h"

#include <crypto/hmac.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dropbear_hmac_hash_is_sha256(int hash)
{
  int ret;

  ret = hash_is_valid(hash);
  if (ret != CRYPT_OK)
    {
      return ret;
    }

  if (hash_descriptor[hash].hashsize != SHA256_DIGEST_LENGTH ||
      hash_descriptor[hash].blocksize != SHA256_BLOCK_LENGTH)
    {
      return CRYPT_INVALID_HASH;
    }

  return CRYPT_OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int hmac_init(hmac_state *hmac, int hash, const unsigned char *key,
              unsigned long keylen)
{
  int ret;

  LTC_ARGCHK(hmac != NULL);
  LTC_ARGCHK(key != NULL);

  if (keylen == 0)
    {
      return CRYPT_INVALID_KEYSIZE;
    }

  ret = dropbear_hmac_hash_is_sha256(hash);
  if (ret != CRYPT_OK)
    {
      return ret;
    }

  hmac->hash = hash;
  hmac_sha256_init(&hmac->ctx, key, (u_int)keylen);
  return CRYPT_OK;
}

int hmac_process(hmac_state *hmac, const unsigned char *in,
                 unsigned long inlen)
{
  LTC_ARGCHK(hmac != NULL);
  LTC_ARGCHK(in != NULL || inlen == 0);

  hmac_sha256_update(&hmac->ctx, in, (u_int)inlen);
  return CRYPT_OK;
}

int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen)
{
  uint8_t digest[SHA256_DIGEST_LENGTH];
  unsigned long copylen;

  LTC_ARGCHK(hmac != NULL);
  LTC_ARGCHK(out != NULL);
  LTC_ARGCHK(outlen != NULL);

  if (dropbear_hmac_hash_is_sha256(hmac->hash) != CRYPT_OK)
    {
      return CRYPT_INVALID_HASH;
    }

  hmac_sha256_final(digest, &hmac->ctx);

  copylen = MIN(*outlen, SHA256_DIGEST_LENGTH);
  memcpy(out, digest, copylen);
  *outlen = copylen;

  zeromem(digest, sizeof(digest));
  zeromem(hmac, sizeof(*hmac));
  return CRYPT_OK;
}
