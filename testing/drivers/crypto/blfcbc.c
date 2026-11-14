/****************************************************************************
 * apps/testing/drivers/crypto/blfcbc.c
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

/* Blowfish vectors are taken from mbedtls 2.28.10
 * tests/suites/test_suite_blowfish.data (SSLeay reference data).
 * The ECB vectors are run as one CBC block with an all-zero IV.
 */

static const struct tb g_testcase[] =
{
  {
    /* BLOWFISH-ECB Encrypt SSLeay reference #1 */

    "\x00\x00\x00\x00\x00\x00\x00\x00",
    8,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x4e\xf9\x97\x45\x61\x98\xdd\x78",
    8
  },
  {
    /* BLOWFISH-ECB Encrypt SSLeay reference #2 */

    "\xff\xff\xff\xff\xff\xff\xff\xff",
    8,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\xff\xff\xff\xff\xff\xff\xff\xff",
    "\x51\x86\x6f\xd5\xb8\x5e\xcb\x8a",
    8
  },
  {
    /* BLOWFISH-ECB Encrypt SSLeay reference #3 */

    "\x30\x00\x00\x00\x00\x00\x00\x00",
    8,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x10\x00\x00\x00\x00\x00\x00\x01",
    "\x7d\x85\x6f\x9a\x61\x30\x63\xf2",
    8
  },
  {
    /* BLOWFISH-SETKEY Setkey SSLeay reference #5 */

    "\xf0\xe1\xd2\xc3\xb4",
    5,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\xfe\xdc\xba\x98\x76\x54\x32\x10",
    "\xb3\x9e\x44\x48\x1b\xdb\x1e\x6e",
    8
  },
  {
    /* BLOWFISH-SETKEY Setkey 448 bits */

    "\xf0\xe1\xd2\xc3\xb4\xa5\x96\x87\x78\x69\x5a\x4b\x3c\x2d\x1e\x0f"
    "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff"
    "\x01\x23\x45\x67\x89\xab\xcd\xef\x01\x02\x03\x04\x05\x06\x07\x08"
    "\x09\x0a\x0b\x0c\x0d\x0e\x0f\xff",
    56,
    "\x00\x00\x00\x00\x00\x00\x00\x00",
    "\xfe\xdc\xba\x98\x76\x54\x32\x10",
    "\x2f\xb3\xab\x7f\x0e\xe9\x1b\x69",
    8
  },
  {
    /* BLOWFISH-CBC Encrypt [#1] */

    "\x01\x23\x45\x67\x89\xab\xcd\xef\xf0\xe1\xd2\xc3\xb4\xa5\x96\x87",
    16,
    "\xfe\xdc\xba\x98\x76\x54\x32\x10",
    "\x37\x36\x35\x34\x33\x32\x31\x20\x4e\x6f\x77\x20\x69\x73\x20\x74"
    "\x68\x65\x20\x74\x69\x6d\x65\x20\x66\x6f\x72\x20\x00\x00\x00\x00",
    "\x6b\x77\xb4\xd6\x30\x06\xde\xe6\x05\xb1\x56\xe2\x74\x03\x97\x93"
    "\x58\xde\xb9\xe7\x15\x46\x16\xd9\x59\xf1\x65\x2b\xd5\xff\x92\xcc",
    32
  }
};

/* enc_xform_blf accepts 5..56 byte keys */

static const char g_badkey[57];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int syscrypt(FAR const char *key, size_t klen,
                    FAR const char *iv, FAR const char *in,
                    FAR unsigned char *out, size_t len, int encrypt,
                    size_t chunk)
{
  struct session_op session;
  struct crypt_op cryp;
  size_t offset = 0;
  int cryptodev_fd = -1;
  int fd = -1;
  char tmp_iv[BLOWFISH_BLOCK_LEN];

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
  session.cipher = CRYPTO_BLF_CBC;
  session.key = (caddr_t)key;
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  session.keylen = klen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  memcpy(tmp_iv, iv, BLOWFISH_BLOCK_LEN);
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.mac = 0;
  cryp.ivlen = BLOWFISH_BLOCK_LEN;
  cryp.iv = (caddr_t)tmp_iv;
  while (offset < len)
    {
      cryp.len = MIN(len - offset, chunk);
      cryp.olen = cryp.len;
      cryp.src = (caddr_t)in + offset;
      cryp.dst = (caddr_t)out + offset;
      if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
        {
          warn("CIOCCRYPT");
          goto err;
        }

      cryp.flags |= COP_FLAG_UPDATE;
      offset += cryp.len;
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

static int syssession(FAR const char *key, size_t klen)
{
  struct session_op session;
  int cryptodev_fd = -1;
  int fd = -1;
  int ret = -1;

  if ((fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("/dev/crypto");
      return -1;
    }

  if (ioctl(fd, CRIOGET, &cryptodev_fd) == -1)
    {
      warn("CRIOGET");
      close(fd);
      return -1;
    }

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_BLF_CBC;
  session.key = (caddr_t)key;
  session.op = COP_ENCRYPT;
  session.keylen = klen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == 0)
    {
      ioctl(cryptodev_fd, CIOCFSESSION, &session.ses);
      ret = 0;
    }

  close(cryptodev_fd);
  close(fd);
  return ret;
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

static int check_vector(FAR const struct tb *t, int idx, size_t chunk)
{
  unsigned char out[32];

  if (syscrypt(t->key, t->keylen, t->iv, t->plain, out,
               t->len, 1, chunk) != 0 ||
      match(out, (FAR const unsigned char *)t->cipher, t->len, idx) != 0)
    {
      return -1;
    }

  if (syscrypt(t->key, t->keylen, t->iv, t->cipher, out,
               t->len, 0, chunk) != 0 ||
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
      /* Whole message in a single CIOCCRYPT */

      if (check_vector(&g_testcase[i], i, g_testcase[i].len) != 0)
        {
          fail++;
        }
      else
        {
          printf("blfcbc test vector %d ok\n", i);
        }

      /* One block per CIOCCRYPT, chained with COP_FLAG_UPDATE */

      if (check_vector(&g_testcase[i], i, BLOWFISH_BLOCK_LEN) != 0)
        {
          fail++;
        }
      else
        {
          printf("blfcbc test vector %d block chunks ok\n", i);
        }
    }

  /* enc_xform_blf accepts 5..56 byte keys: 4 and 57 must be rejected */

  if (syssession(g_badkey, 4) == 0 || syssession(g_badkey, 57) == 0)
    {
      printf("blfcbc accepted an out-of-range key length\n");
      fail++;
    }
  else
    {
      printf("blfcbc key length check ok\n");
    }

  if (fail)
    {
      printf("blfcbc: %d test(s) failed\n", fail);
    }
  else
    {
      printf("blfcbc: all tests passed\n");
    }

  return fail;
}
