/****************************************************************************
 * apps/testing/drivers/crypto/castcbc.c
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

#include <err.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tb
{
  FAR const char *key;
  size_t keylen;
  FAR const char *iv;
  FAR const char *plain;
  FAR const char *cipher;
  size_t len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* CAST-128 vectors are taken from RFC 2144 Appendix B.1, run as one CBC
 * block with an all-zero IV.  Keys of 80 bits or less use 12 rounds.
 */

static const struct tb g_testcase[] =
{
  {
    /* RFC 2144 B.1, 128-bit key */

    "\x01\x23\x45\x67\x12\x34\x56\x78\x23\x45\x67\x89\x34\x56\x78\x9a",
    16,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x01\x23\x45\x67\x89\xab\xcd\xef",
    "\x23\x8b\x4f\xe5\x84\x7e\x44\xb2",
    8
  },
  {
    /* RFC 2144 B.1, 80-bit key */

    "\x01\x23\x45\x67\x12\x34\x56\x78\x23\x45",
    10,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x01\x23\x45\x67\x89\xab\xcd\xef",
    "\xeb\x6a\x71\x1a\x2c\x02\x27\x1b",
    8
  },
  {
    /* RFC 2144 B.1, 40-bit key */

    "\x01\x23\x45\x67\x12",
    5,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x01\x23\x45\x67\x89\xab\xcd\xef",
    "\x7a\xc8\x16\xd1\x6e\x9b\x30\x2e",
    8
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int syscrypt(FAR const char *key, size_t klen,
                    FAR const char *iv, FAR const char *in,
                    FAR unsigned char *out, size_t len, int encrypt)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;
  char tmp_iv[CAST128_BLOCK_LEN];

  if ((fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("/dev/crypto");
      goto err;
    }

  if (ioctl(fd, CRIOGET, &cryptodev_fd) == -1)
    {
      warn("CRIOGET");
      goto err;
    }

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_CAST_CBC;
  session.key = (caddr_t)key;
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  session.keylen = klen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  memcpy(tmp_iv, iv, CAST128_BLOCK_LEN);
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.mac = 0;
  cryp.ivlen = CAST128_BLOCK_LEN;
  cryp.iv = (caddr_t)tmp_iv;
  cryp.len = len;
  cryp.olen = len;
  cryp.src = (caddr_t)in;
  cryp.dst = (caddr_t)out;
  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      warn("CIOCCRYPT");
      goto err;
    }

  if (ioctl(cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      warn("CIOCFSESSION");
      goto err;
    }

  close(cryptodev_fd);
  close(fd);
  return 0;

err:
  if (cryptodev_fd != -1)
    {
      close(cryptodev_fd);
    }

  if (fd != -1)
    {
      close(fd);
    }

  return -1;
}

static int match(FAR const unsigned char *a, FAR const unsigned char *b,
                 size_t len, int idx)
{
  size_t i;

  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  warnx("vector %d: mismatch", idx);

  for (i = 0; i < len; i++)
    {
      printf("%2.2x", a[i]);
    }

  printf("\n");
  for (i = 0; i < len; i++)
    {
      printf("%2.2x", b[i]);
    }

  printf("\n");

  return -1;
}

static int check_vector(FAR const struct tb *t, int idx)
{
  unsigned char out[8];

  if (syscrypt(t->key, t->keylen, t->iv, t->plain, out, t->len, 1) != 0 ||
      match(out, (FAR const unsigned char *)t->cipher, t->len, idx) != 0)
    {
      return -1;
    }

  if (syscrypt(t->key, t->keylen, t->iv, t->cipher, out, t->len, 0) != 0 ||
      match(out, (FAR const unsigned char *)t->plain, t->len, idx) != 0)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fail = 0;
  int i;

  for (i = 0; i < nitems(g_testcase); i++)
    {
      if (check_vector(&g_testcase[i], i) != 0)
        {
          fail++;
        }
      else
        {
          printf("castcbc test vector %d ok\n", i);
        }
    }

  if (fail)
    {
      printf("castcbc: %d test(s) failed\n", fail);
    }
  else
    {
      printf("castcbc: all tests passed\n");
    }

  return fail;
}
