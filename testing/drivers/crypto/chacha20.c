/****************************************************************************
 * apps/testing/drivers/crypto/chacha20.c
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

enum
{
  TST_KEY,
  TST_IV,
  TST_PLAIN,
  TST_CIPHER,
  TST_NUM
};

/* Test vectors aligned with mbedtls library/chacha20.c self_test
 * (RFC 7539/8439 ChaCha20 encryption).
 *
 * The cryptodev CHACHA20 session key packs the 32-byte key and a 4-byte
 * little-endian initial block counter (salt), matching the mbedtls-alt
 * chacha20 bridge (counter memcpy-ed after the key).  The "iv" field is the
 * 12-byte nonce.  Vector 1 (375 bytes) spans multiple 64-byte blocks to
 * exercise cross-block counter increment and non-block-aligned tail.
 */

struct
{
  FAR char *data[TST_NUM];
}

static const g_tests[] =
{
  /* mbedtls chacha20 self_test vector 0 (RFC 7539/8439, counter=0) */

  {
    {
      /* 32-byte key + 4-byte LE counter (0x00000000) */

      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
      "00 00 00 00",

      /* 12-byte nonce */

      "00 00 00 00 00 00 00 00 00 00 00 00",

      /* 64-byte plaintext */

      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00",

      /* expected ciphertext */

      "76 b8 e0 ad a0 f1 3d 90 40 5d 6a e5 53 86 bd 28 "
      "bd d2 19 b8 a0 8d ed 1a a8 36 ef cc 8b 77 0d c7 "
      "da 41 59 7c 51 57 48 8d 77 24 e0 3f b8 d8 4a 37 "
      "6a 43 b8 f4 15 18 a1 1c c3 87 b6 69 b2 ee 65 86"
    }
  },

  /* mbedtls chacha20 self_test vector 1 (RFC 7539/8439, counter=1) */

  {
    {
      /* 32-byte key + 4-byte LE counter (0x00000001) */

      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 "
      "01 00 00 00",

      /* 12-byte nonce */

      "00 00 00 00 00 00 00 00 00 00 00 02",

      /* 375-byte plaintext */

      "41 6e 79 20 73 75 62 6d 69 73 73 69 6f 6e 20 74 "
      "6f 20 74 68 65 20 49 45 54 46 20 69 6e 74 65 6e "
      "64 65 64 20 62 79 20 74 68 65 20 43 6f 6e 74 72 "
      "69 62 75 74 6f 72 20 66 6f 72 20 70 75 62 6c 69 "
      "63 61 74 69 6f 6e 20 61 73 20 61 6c 6c 20 6f 72 "
      "20 70 61 72 74 20 6f 66 20 61 6e 20 49 45 54 46 "
      "20 49 6e 74 65 72 6e 65 74 2d 44 72 61 66 74 20 "
      "6f 72 20 52 46 43 20 61 6e 64 20 61 6e 79 20 73 "
      "74 61 74 65 6d 65 6e 74 20 6d 61 64 65 20 77 69 "
      "74 68 69 6e 20 74 68 65 20 63 6f 6e 74 65 78 74 "
      "20 6f 66 20 61 6e 20 49 45 54 46 20 61 63 74 69 "
      "76 69 74 79 20 69 73 20 63 6f 6e 73 69 64 65 72 "
      "65 64 20 61 6e 20 22 49 45 54 46 20 43 6f 6e 74 "
      "72 69 62 75 74 69 6f 6e 22 2e 20 53 75 63 68 20 "
      "73 74 61 74 65 6d 65 6e 74 73 20 69 6e 63 6c 75 "
      "64 65 20 6f 72 61 6c 20 73 74 61 74 65 6d 65 6e "
      "74 73 20 69 6e 20 49 45 54 46 20 73 65 73 73 69 "
      "6f 6e 73 2c 20 61 73 20 77 65 6c 6c 20 61 73 20 "
      "77 72 69 74 74 65 6e 20 61 6e 64 20 65 6c 65 63 "
      "74 72 6f 6e 69 63 20 63 6f 6d 6d 75 6e 69 63 61 "
      "74 69 6f 6e 73 20 6d 61 64 65 20 61 74 20 61 6e "
      "79 20 74 69 6d 65 20 6f 72 20 70 6c 61 63 65 2c "
      "20 77 68 69 63 68 20 61 72 65 20 61 64 64 72 65 "
      "73 73 65 64 20 74 6f",

      /* expected ciphertext */

      "a3 fb f0 7d f3 fa 2f de 4f 37 6c a2 3e 82 73 70 "
      "41 60 5d 9f 4f 4f 57 bd 8c ff 2c 1d 4b 79 55 ec "
      "2a 97 94 8b d3 72 29 15 c8 f3 d3 37 f7 d3 70 05 "
      "0e 9e 96 d6 47 b7 c3 9f 56 e0 31 ca 5e b6 25 0d "
      "40 42 e0 27 85 ec ec fa 4b 4b b5 e8 ea d0 44 0e "
      "20 b6 e8 db 09 d8 81 a7 c6 13 2f 42 0e 52 79 50 "
      "42 bd fa 77 73 d8 a9 05 14 47 b3 29 1c e1 41 1c "
      "68 04 65 55 2a a6 c4 05 b7 76 4d 5e 87 be a8 5a "
      "d0 0f 84 49 ed 8f 72 d0 d6 62 ab 05 26 91 ca 66 "
      "42 4b c8 6d 2d f8 0e a4 1f 43 ab f9 37 d3 25 9d "
      "c4 b2 d0 df b4 8a 6c 91 39 dd d7 f7 69 66 e9 28 "
      "e6 35 55 3b a7 6c 5c 87 9d 7b 35 d4 9e b2 e6 2b "
      "08 71 cd ac 63 89 39 e2 5e 8a 1e 0e f9 d5 28 0f "
      "a8 ca 32 8b 35 1c 3c 76 59 89 cb cf 3d aa 8b 6c "
      "cc 3a af 9f 39 79 c9 2b 37 20 fc 88 dc 95 ed 84 "
      "a1 be 05 9c 64 99 b9 fd a2 36 e7 e8 18 b0 4b 0b "
      "c3 9c 1e 87 6b 19 3b fe 55 69 75 3f 88 12 8c c0 "
      "8a aa 9b 63 d1 a1 6f 80 ef 25 54 d7 18 9c 41 1f "
      "58 69 ca 52 c5 b8 3f a3 6f f2 16 b9 c1 d3 00 62 "
      "be bc fd 2d c5 bc e0 91 19 34 fd a7 9a 86 f6 e6 "
      "98 ce d7 59 c3 ff 9b 64 77 33 8f 3d a4 f9 cd 85 "
      "14 ea 99 82 cc af b3 41 b2 38 4d d9 02 f3 d1 ab "
      "7a c6 1d d2 9c 6f 21 ba 5b 86 2f 37 30 e3 7c fd "
      "c4 fd 80 6c 22 f2 21"
    }
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int syscrypt(FAR const unsigned char *key, size_t klen,
                    FAR const unsigned char *iv, FAR const unsigned char *in,
                    FAR unsigned char *out, size_t len, int encrypt)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;

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
  session.cipher = CRYPTO_CHACHA20;
  session.key = (caddr_t)key;
  session.keylen = klen;
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.olen = len;
  cryp.ivlen = 12;
  cryp.src = (caddr_t)in;
  cryp.dst = (caddr_t)out;
  cryp.iv = (caddr_t)iv;
  cryp.mac = 0;
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

static int match(FAR unsigned char *a, FAR unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    {
      return 1;
    }

  warnx("ciphertext mismatch");

  for (i = 0; i < (int)len; i++)
    {
      printf("%2.2x", a[i]);
    }

  printf("\n");
  for (i = 0; i < (int)len; i++)
    {
      printf("%2.2x", b[i]);
    }

  printf("\n");

  return 0;
}

static int run(int num)
{
  FAR u_char *data[TST_NUM];
  FAR u_char *cipher = NULL;
  FAR u_char *plain = NULL;
  FAR u_char *p;
  FAR char *from;
  FAR char *ep;
  u_long val;
  int length[TST_NUM];
  int fail = 1;
  int len;
  int j;
  int i;

  for (i = 0; i < TST_NUM; i++)
    {
      data[i] = NULL;
    }

  for (i = 0; i < TST_NUM; i++)
    {
      from = g_tests[num].data[i];
      len = strlen(from);
      if ((p = malloc(len)) == 0)
        {
          warn("malloc");
          goto done;
        }

      errno = 0;
      for (j = 0; j < len; j++)
        {
          val = strtoul(&from[j * 3], &ep, 16);
          p[j] = (u_char)val;
          if (*ep == '\0' || errno)
            {
              break;
            }
        }

      length[i] = j + 1;
      data[i] = p;
    }

  len = length[TST_PLAIN];

  /* Encrypt: plaintext -> ciphertext, compare against expected */

  if ((cipher = malloc(len)) == 0)
    {
      warn("malloc");
      goto done;
    }

  if (syscrypt(data[TST_KEY], length[TST_KEY], data[TST_IV],
      data[TST_PLAIN], cipher, len, 1) < 0)
    {
      warnx("encrypt with /dev/crypto failed");
      goto done;
    }

  if (!match(data[TST_CIPHER], cipher, len))
    {
      printf("FAILED encrypt test vector %d\n", num);
      goto done;
    }

  /* Decrypt: ciphertext -> plaintext, compare against original */

  if ((plain = malloc(len)) == 0)
    {
      warn("malloc");
      goto done;
    }

  if (syscrypt(data[TST_KEY], length[TST_KEY], data[TST_IV],
      data[TST_CIPHER], plain, len, 0) < 0)
    {
      warnx("decrypt with /dev/crypto failed");
      goto done;
    }

  fail = !match(data[TST_PLAIN], plain, len);
  printf("%s test vector %d\n", fail ? "FAILED" : "OK", num);

done:
  for (i = 0; i < TST_NUM; i++)
    {
      free(data[i]);
    }

  free(cipher);
  free(plain);
  return fail;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fail = 0;
  int i;

  for (i = 0; i < nitems(g_tests); i++)
    {
      fail += run(i);
    }

  if (fail)
    {
      printf("chacha20: %d test(s) failed\n", fail);
    }
  else
    {
      printf("chacha20: all tests passed\n");
    }

  return fail;
}
