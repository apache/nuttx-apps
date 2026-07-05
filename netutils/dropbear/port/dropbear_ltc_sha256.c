/****************************************************************************
 * apps/netutils/dropbear/port/dropbear_ltc_sha256.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/* LibTomCrypt-compatible SHA256 descriptor backed by NuttX crypto/sha2.
 * Dropbear still expects the libtomcrypt hash API for KEX and HMAC.  This
 * adapter keeps that API while avoiding the bundled SHA256 implementation.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "includes.h"

#include <crypto/sha2.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct ltc_hash_descriptor sha256_desc =
{
  "sha256",
  0,
  SHA256_DIGEST_LENGTH,
  SHA256_BLOCK_LENGTH,
  { 2, 16, 840, 1, 101, 3, 4, 2, 1 },
  9,

  sha256_init,
  sha256_process,
  sha256_done,
  sha256_test,
  NULL
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sha256_init(hash_state *md)
{
  LTC_ARGCHK(md != NULL);

  sha256init(&md->sha256.ctx);
  return CRYPT_OK;
}

int sha256_process(hash_state *md, const unsigned char *in,
                   unsigned long inlen)
{
  LTC_ARGCHK(md != NULL);
  LTC_ARGCHK(in != NULL || inlen == 0);

  sha256update(&md->sha256.ctx, in, inlen);
  return CRYPT_OK;
}

int sha256_done(hash_state *md, unsigned char *out)
{
  LTC_ARGCHK(md != NULL);
  LTC_ARGCHK(out != NULL);

  sha256final(out, &md->sha256.ctx);
  zeromem(md, sizeof(*md));
  return CRYPT_OK;
}

int sha256_test(void)
{
  return CRYPT_NOP;
}
