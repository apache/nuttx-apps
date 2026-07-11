/****************************************************************************
 * apps/netutils/dropbear/port/dropbear_chachapoly.c
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

/* chacha20-poly1305@openssh.com backed by the NuttX crypto device.
 *
 * The SSH construction (see OpenSSH PROTOCOL.chacha20poly1305) uses the
 * original DJB ChaCha20 parameterization: a 64-bit block counter in state
 * words 12..13 and a 64-bit nonce (the packet sequence number, big endian)
 * in words 14..15.  This maps to the kernel CRYPTO_CHACHA20_DJB transform,
 * whose 16-byte IV is loaded verbatim into words 12..15 as a 64-bit
 * little-endian counter followed by the 64-bit nonce.  The Poly1305 tag is
 * computed with the kernel CRYPTO_POLY1305 transform, keyed with the first
 * keystream block (counter 0) of the main key, as the protocol requires.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "includes.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <crypto/cryptodev.h>

#include "algo.h"
#include "dbutil.h"
#include "chachapoly.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHACHA20_KEY_LEN   32
#define CHACHA20_BLOCKSIZE 8
#define CHACHA20_IV_LEN    16
#define POLY1305_KEY_LEN   32
#define POLY1305_TAG_LEN   16

/* The keystream comes from the NuttX crypto device, so each upstream
 * chacha_state is unused and its input[] buffer just stores the 32-byte key,
 * keeping the upstream header unpatched.
 */

#define KEY_MAIN(s)   ((FAR unsigned char *)(s)->chacha.input)
#define KEY_HEADER(s) ((FAR unsigned char *)(s)->header.input)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct ltc_cipher_descriptor g_dropbear_chachapoly_dummy =
{
  .name = NULL
};

static const struct dropbear_hash g_dropbear_chachapoly_mac =
{
  NULL,
  POLY1305_KEY_LEN,
  POLY1305_TAG_LEN
};

static int g_dropbear_cryptofd = -1;

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct dropbear_cipher dropbear_chachapoly =
{
  &g_dropbear_chachapoly_dummy,
  CHACHA20_KEY_LEN * 2,
  CHACHA20_BLOCKSIZE
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dropbear_cryptodev_fd(void)
{
  int fd;
  int cfd;

  if (g_dropbear_cryptofd >= 0)
    {
      return g_dropbear_cryptofd;
    }

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
  g_dropbear_cryptofd = cfd;
  return g_dropbear_cryptofd;
}

/* One ChaCha20 (DJB layout) pass: encrypt/decrypt len bytes with the given
 * 64-bit block counter and the packet sequence number as nonce.
 */

static int dropbear_chacha(FAR const unsigned char *key, unsigned int seq,
                           uint64_t counter, FAR const unsigned char *in,
                           FAR unsigned char *out, size_t len)
{
  struct session_op session;
  struct crypt_op cryp;
  unsigned char iv[CHACHA20_IV_LEN];
  int cfd;
  int ret = CRYPT_ERROR;
  int i;

  cfd = dropbear_cryptodev_fd();
  if (cfd < 0)
    {
      return CRYPT_ERROR;
    }

  /* IV = 64-bit little-endian block counter || 64-bit nonce.  The nonce is
   * the packet sequence number stored big endian, as OpenSSH does.
   */

  for (i = 0; i < 8; i++)
    {
      iv[i] = (unsigned char)(counter >> (8 * i));
    }

  STORE64H((uint64_t)seq, iv + 8);

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_CHACHA20_DJB;
  session.key = (caddr_t)key;
  session.keylen = CHACHA20_KEY_LEN;
  if (ioctl(cfd, CIOCGSESSION, &session) < 0)
    {
      return CRYPT_ERROR;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.len = len;
  cryp.src = (caddr_t)in;
  cryp.dst = (caddr_t)out;
  cryp.iv = (caddr_t)iv;
  cryp.ivlen = sizeof(iv);
  if (ioctl(cfd, CIOCCRYPT, &cryp) == 0)
    {
      ret = CRYPT_OK;
    }

  ioctl(cfd, CIOCFSESSION, &session.ses);
  return ret;
}

static int dropbear_poly1305(FAR const unsigned char *key,
                             FAR const unsigned char *in, size_t len,
                             FAR unsigned char *tag)
{
  struct session_op session;
  struct crypt_op cryp;
  int cfd;
  int ret = CRYPT_ERROR;

  cfd = dropbear_cryptodev_fd();
  if (cfd < 0)
    {
      return CRYPT_ERROR;
    }

  memset(&session, 0, sizeof(session));
  session.mac = CRYPTO_POLY1305;
  session.mackey = (caddr_t)key;
  session.mackeylen = POLY1305_KEY_LEN;
  if (ioctl(cfd, CIOCGSESSION, &session) < 0)
    {
      return CRYPT_ERROR;
    }

  /* Plain (non-HMAC) MACs are driven in two steps through /dev/crypto:
   * COP_FLAG_UPDATE feeds the data, then a final call without the flag
   * writes out the tag.
   */

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.flags = COP_FLAG_UPDATE;
  cryp.len = len;
  cryp.src = (caddr_t)in;
  if (ioctl(cfd, CIOCCRYPT, &cryp) == 0)
    {
      cryp.flags = 0;
      cryp.len = 0;
      cryp.src = NULL;
      cryp.mac = (caddr_t)tag;
      if (ioctl(cfd, CIOCCRYPT, &cryp) == 0)
        {
          ret = CRYPT_OK;
        }
    }

  ioctl(cfd, CIOCFSESSION, &session.ses);
  return ret;
}

static int dropbear_chachapoly_start(int cipher,
                                     FAR const unsigned char *iv,
                                     FAR const unsigned char *key,
                                     int keylen, int num_rounds,
                                     FAR void *cipher_state)
{
  FAR dropbear_chachapoly_state *state = cipher_state;

  UNUSED(cipher);
  UNUSED(iv);

  if (keylen != CHACHA20_KEY_LEN * 2 || num_rounds != 0)
    {
      return CRYPT_ERROR;
    }

  if (dropbear_cryptodev_fd() < 0)
    {
      return CRYPT_ERROR;
    }

  memcpy(KEY_MAIN(state), key, CHACHA20_KEY_LEN);
  memcpy(KEY_HEADER(state), key + CHACHA20_KEY_LEN, CHACHA20_KEY_LEN);
  return CRYPT_OK;
}

static int dropbear_chachapoly_crypt(unsigned int seq,
                                     FAR const unsigned char *in,
                                     FAR unsigned char *out,
                                     unsigned long len, unsigned long taglen,
                                     FAR void *cipher_state,
                                     int direction)
{
  FAR dropbear_chachapoly_state *state = cipher_state;
  unsigned char key[POLY1305_KEY_LEN];
  unsigned char tag[POLY1305_TAG_LEN];
  unsigned char zero[POLY1305_KEY_LEN];
  int ret = CRYPT_ERROR;

  if (len < 4 || taglen != POLY1305_TAG_LEN)
    {
      return CRYPT_ERROR;
    }

  /* Poly1305 key = first keystream block of the main key at counter 0 */

  memset(zero, 0, sizeof(zero));
  if (dropbear_chacha(KEY_MAIN(state), seq, 0, zero, key,
                      sizeof(key)) != CRYPT_OK)
    {
      goto out;
    }

  if (direction == LTC_DECRYPT)
    {
      if (dropbear_poly1305(key, in, len, tag) != CRYPT_OK)
        {
          goto out;
        }

      if (constant_time_memcmp(in + len, tag, sizeof(tag)) != 0)
        {
          goto out;
        }
    }

  /* Packet length: header key, counter 0.  Payload: main key, counter 1. */

  if (dropbear_chacha(KEY_HEADER(state), seq, 0, in, out, 4) != CRYPT_OK)
    {
      goto out;
    }

  if (dropbear_chacha(KEY_MAIN(state), seq, 1, in + 4, out + 4,
                      len - 4) != CRYPT_OK)
    {
      goto out;
    }

  if (direction == LTC_ENCRYPT)
    {
      if (dropbear_poly1305(key, out, len, out + len) != CRYPT_OK)
        {
          goto out;
        }
    }

  ret = CRYPT_OK;

out:
  zeromem(key, sizeof(key));
  zeromem(tag, sizeof(tag));
  return ret;
}

static int
dropbear_chachapoly_getlength(unsigned int seq, FAR const unsigned char *in,
                              FAR unsigned int *outlen, unsigned long len,
                              FAR void *cipher_state)
{
  FAR dropbear_chachapoly_state *state = cipher_state;
  unsigned char buf[4];

  if (len < sizeof(buf))
    {
      return CRYPT_ERROR;
    }

  if (dropbear_chacha(KEY_HEADER(state), seq, 0, in, buf,
                      sizeof(buf)) != CRYPT_OK)
    {
      return CRYPT_ERROR;
    }

  LOAD32H(*outlen, buf);
  return CRYPT_OK;
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct dropbear_cipher_mode dropbear_mode_chachapoly =
{
  dropbear_chachapoly_start,
  NULL,
  NULL,
  dropbear_chachapoly_crypt,
  dropbear_chachapoly_getlength,
  &g_dropbear_chachapoly_mac
};
