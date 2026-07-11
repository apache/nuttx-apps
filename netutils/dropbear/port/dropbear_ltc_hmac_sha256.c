/****************************************************************************
 * apps/netutils/dropbear/port/dropbear_ltc_hmac_sha256.c
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

/* LibTomCrypt-compatible incremental HMAC API backed by a
 * CRYPTO_SHA2_256_HMAC /dev/crypto session (hmac-sha2-256, the only MAC the
 * NuttX Dropbear configuration enables): init opens the session, process
 * feeds data with COP_FLAG_UPDATE, done reads the tag and frees the session.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "includes.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DROPBEAR_HMAC_SHA256_DIGESTLEN 32
#define DROPBEAR_HMAC_SHA256_BLOCKLEN  64

/* Stash the process-local /dev/crypto descriptor and session id in the
 * unused md hash-state buffer so the upstream hmac_state needs no patch.
 */

static_assert(sizeof(int) + sizeof(uint32_t) <= sizeof(hash_state),
              "cryptodev state does not fit in hmac_state.md");

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dropbear_hmac_cryptodev_open(void)
{
  int fd;
  int cfd;

  fd = open("/dev/crypto", O_RDWR);
  if (fd < 0)
    {
      return -1;
    }

  if (ioctl(fd, CRIOGET, &cfd) < 0)
    {
      close(fd);
      return -1;
    }

  close(fd);

  if (fcntl(cfd, F_SETFD, FD_CLOEXEC) < 0)
    {
      close(cfd);
      return -1;
    }

  return cfd;
}

static int dropbear_hmac_hash_is_sha256(int hash)
{
  int ret;

  ret = hash_is_valid(hash);
  if (ret != CRYPT_OK)
    {
      return ret;
    }

  if (hash_descriptor[hash].hashsize != DROPBEAR_HMAC_SHA256_DIGESTLEN ||
      hash_descriptor[hash].blocksize != DROPBEAR_HMAC_SHA256_BLOCKLEN)
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
  struct session_op session;
  int cfd;
  int ret;

  LTC_ARGCHK(hmac != NULL);
  LTC_ARGCHK(key != NULL);

  if (keylen == 0 || keylen > INT_MAX)
    {
      return CRYPT_INVALID_KEYSIZE;
    }

  ret = dropbear_hmac_hash_is_sha256(hash);
  if (ret != CRYPT_OK)
    {
      return ret;
    }

  zeromem(hmac, sizeof(*hmac));
  cfd = dropbear_hmac_cryptodev_open();
  if (cfd < 0)
    {
      return CRYPT_ERROR;
    }

  memset(&session, 0, sizeof(session));
  session.mac = CRYPTO_SHA2_256_HMAC;
  session.mackey = (caddr_t)key;
  session.mackeylen = (int)keylen;
  if (ioctl(cfd, CIOCGSESSION, &session) < 0)
    {
      close(cfd);
      zeromem(hmac, sizeof(*hmac));
      return CRYPT_ERROR;
    }

  memcpy(&hmac->md, &cfd, sizeof(cfd));
  memcpy((FAR uint8_t *)&hmac->md + sizeof(cfd),
         &session.ses, sizeof(session.ses));
  hmac->hash = hash;
  return CRYPT_OK;
}

int hmac_process(hmac_state *hmac, const unsigned char *in,
                 unsigned long inlen)
{
  struct crypt_op cryp;
  uint32_t ses;
  int cfd;
  int ret;

  LTC_ARGCHK(hmac != NULL);
  LTC_ARGCHK(in != NULL || inlen == 0);

  ret = dropbear_hmac_hash_is_sha256(hmac->hash);
  if (ret != CRYPT_OK)
    {
      return ret;
    }

  if (inlen > UINT_MAX)
    {
      return CRYPT_OVERFLOW;
    }

  memcpy(&cfd, &hmac->md, sizeof(cfd));
  memcpy(&ses, (FAR uint8_t *)&hmac->md + sizeof(cfd), sizeof(ses));
  if (cfd < 0)
    {
      return CRYPT_ERROR;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = ses;
  cryp.op = COP_ENCRYPT;
  cryp.flags = COP_FLAG_UPDATE;
  cryp.len = (unsigned int)inlen;
  cryp.src = (caddr_t)in;
  if (ioctl(cfd, CIOCCRYPT, &cryp) < 0)
    {
      return CRYPT_ERROR;
    }

  return CRYPT_OK;
}

int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen)
{
  unsigned char digest[DROPBEAR_HMAC_SHA256_DIGESTLEN];
  struct crypt_op cryp;
  unsigned long copylen;
  uint32_t ses;
  int cfd;
  int ret = CRYPT_ERROR;

  LTC_ARGCHK(hmac != NULL);
  LTC_ARGCHK(out != NULL);
  LTC_ARGCHK(outlen != NULL);

  memcpy(&cfd, &hmac->md, sizeof(cfd));
  memcpy(&ses, (FAR uint8_t *)&hmac->md + sizeof(cfd), sizeof(ses));
  if (cfd < 0)
    {
      goto out;
    }

  ret = dropbear_hmac_hash_is_sha256(hmac->hash);
  if (ret != CRYPT_OK)
    {
      goto out;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = ses;
  cryp.op = COP_ENCRYPT;
  cryp.len = 0;
  cryp.src = (caddr_t)digest;
  cryp.mac = (caddr_t)digest;
  if (ioctl(cfd, CIOCCRYPT, &cryp) < 0)
    {
      ret = CRYPT_ERROR;
      goto out;
    }

  copylen = MIN(*outlen, DROPBEAR_HMAC_SHA256_DIGESTLEN);
  memcpy(out, digest, copylen);
  *outlen = copylen;
  ret = CRYPT_OK;

out:
  if (cfd >= 0)
    {
      ioctl(cfd, CIOCFSESSION, &ses);
      close(cfd);
    }

  zeromem(digest, sizeof(digest));
  zeromem(hmac, sizeof(*hmac));
  return ret;
}
