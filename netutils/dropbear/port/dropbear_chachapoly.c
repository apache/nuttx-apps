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

  memset(iv, 0, sizeof(iv));
  for (i = 0; i < 8; i++)
    {
      iv[i] = (unsigned char)(counter >> (8 * i));
    }

  STORE64H((uint64_t)seq, iv + 8);

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_CHACHA20;
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
                                     FAR dropbear_chachapoly_state *state)
{
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

  memcpy(state->key_main, key, CHACHA20_KEY_LEN);
  memcpy(state->key_header, key + CHACHA20_KEY_LEN, CHACHA20_KEY_LEN);
  return CRYPT_OK;
}

static int dropbear_chachapoly_crypt(unsigned int seq,
                                     FAR const unsigned char *in,
                                     FAR unsigned char *out,
                                     unsigned long len, unsigned long taglen,
                                     FAR dropbear_chachapoly_state *state,
                                     int direction)
{
  unsigned char key[POLY1305_KEY_LEN];
  unsigned char tag[POLY1305_TAG_LEN];
  unsigned char zero[POLY1305_KEY_LEN];
  int ret = CRYPT_ERROR;

  if (len < 4 || taglen != POLY1305_TAG_LEN)
    {
      return CRYPT_ERROR;
    }

  memset(zero, 0, sizeof(zero));
  if (dropbear_chacha(state->key_main, seq, 0, zero, key,
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

  if (dropbear_chacha(state->key_header, seq, 0, in, out, 4) != CRYPT_OK)
    {
      goto out;
    }

  if (dropbear_chacha(state->key_main, seq, 1, in + 4, out + 4,
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
                              FAR dropbear_chachapoly_state *state)
{
  unsigned char buf[4];

  if (len < sizeof(buf))
    {
      return CRYPT_ERROR;
    }

  if (dropbear_chacha(state->key_header, seq, 0, in, buf,
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
  (void *)dropbear_chachapoly_start,
  NULL,
  NULL,
  (void *)dropbear_chachapoly_crypt,
  (void *)dropbear_chachapoly_getlength,
  &g_dropbear_chachapoly_mac
};
