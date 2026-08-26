/****************************************************************************
 * apps/testing/drivers/crypto/chachapoly.c
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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

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
  TST_AAD,
  TST_PLAIN,
  TST_CIPHER,
  TST_MAC,
  TST_NUM
};

/* Test vectors aligned with mbedtls library/chachapoly.c self_test
 * (RFC 8439 Section 2.8.2 ChaCha20-Poly1305 AEAD).
 *
 * The cryptodev CHACHA20_POLY1305 session key packs the 32-byte key and a
 * 4-byte counter salt (unused for AEAD; the block counter is fixed by the
 * reinit), matching the mbedtls-alt chachapoly bridge.  The "iv" field is
 * the 12-byte nonce.  Each vector verifies encrypt (ciphertext + tag) and
 * authenticated decrypt (plaintext + tag check).
 */

struct
{
  FAR char *data[TST_NUM];
}

static const g_tests[] =
{
  /* mbedtls chachapoly self_test vector (RFC 8439 Section 2.8.2) */

  {
    {
      /* 32-byte key + 4-byte counter salt (0, unused for AEAD) */

      "80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f "
      "90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f "
      "00 00 00 00",

      /* 12-byte nonce */

      "07 00 00 00 40 41 42 43 44 45 46 47",

      /* additional authenticated data */

      "50 51 52 53 c0 c1 c2 c3 c4 c5 c6 c7",

      /* 114-byte plaintext */

      "4c 61 64 69 65 73 20 61 6e 64 20 47 65 6e 74 6c "
      "65 6d 65 6e 20 6f 66 20 74 68 65 20 63 6c 61 73 "
      "73 20 6f 66 20 27 39 39 3a 20 49 66 20 49 20 63 "
      "6f 75 6c 64 20 6f 66 66 65 72 20 79 6f 75 20 6f "
      "6e 6c 79 20 6f 6e 65 20 74 69 70 20 66 6f 72 20 "
      "74 68 65 20 66 75 74 75 72 65 2c 20 73 75 6e 73 "
      "63 72 65 65 6e 20 77 6f 75 6c 64 20 62 65 20 69 "
      "74 2e",

      /* expected ciphertext */

      "d3 1a 8d 34 64 8e 60 db 7b 86 af bc 53 ef 7e c2 "
      "a4 ad ed 51 29 6e 08 fe a9 e2 b5 a7 36 ee 62 d6 "
      "3d be a4 5e 8c a9 67 12 82 fa fb 69 da 92 72 8b "
      "1a 71 de 0a 9e 06 0b 29 05 d6 a5 b6 7e cd 3b 36 "
      "92 dd bd 7f 2d 77 8b 8c 98 03 ae e3 28 09 1b 58 "
      "fa b3 24 e4 fa d6 75 94 55 85 80 8b 48 31 d7 bc "
      "3f f4 de f0 8e 4b 7a 9d e5 76 d2 65 86 ce c6 4b "
      "61 16",

      /* expected 16-byte Poly1305 tag */

      "1a e1 0b 59 4f 09 e2 6a 7e 90 2e cb d0 60 06 91"
    }
  },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int syscrypt(FAR const unsigned char *key, size_t klen,
                    FAR const unsigned char *iv,
                    FAR const unsigned char *aad, size_t aadlen,
                    FAR const unsigned char *in, FAR unsigned char *out,
                    size_t len, FAR unsigned char *mac, int encrypt)
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
  session.cipher = CRYPTO_CHACHA20_POLY1305;
  session.key = (caddr_t)key;
  session.keylen = klen;
  session.mac = CRYPTO_CHACHA20_POLY1305_MAC;
  session.mackey = (caddr_t)key;
  session.mackeylen = klen;
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
  cryp.aad = (caddr_t)aad;
  cryp.aadlen = aadlen;
  cryp.mac = (caddr_t)mac;
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

static int match(FAR const char *tag, FAR unsigned char *a,
                 FAR unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    {
      return 1;
    }

  warnx("%s mismatch", tag);

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
  unsigned char mac[16];
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

  /* Encrypt: plaintext -> ciphertext + tag */

  if ((cipher = malloc(len)) == 0)
    {
      warn("malloc");
      goto done;
    }

  memset(mac, 0, sizeof(mac));
  if (syscrypt(data[TST_KEY], length[TST_KEY], data[TST_IV],
      data[TST_AAD], length[TST_AAD], data[TST_PLAIN], cipher,
      len, mac, 1) < 0)
    {
      warnx("encrypt with /dev/crypto failed");
      goto done;
    }

  if (!match("ciphertext", data[TST_CIPHER], cipher, len))
    {
      printf("FAILED encrypt test vector %d\n", num);
      goto done;
    }

  if (!match("tag", data[TST_MAC], mac, length[TST_MAC]))
    {
      printf("FAILED tag test vector %d\n", num);
      goto done;
    }

  /* Decrypt: ciphertext -> plaintext + tag */

  if ((plain = malloc(len)) == 0)
    {
      warn("malloc");
      goto done;
    }

  memset(mac, 0, sizeof(mac));
  if (syscrypt(data[TST_KEY], length[TST_KEY], data[TST_IV],
      data[TST_AAD], length[TST_AAD], data[TST_CIPHER], plain,
      len, mac, 0) < 0)
    {
      warnx("decrypt with /dev/crypto failed");
      goto done;
    }

  if (!match("plaintext", data[TST_PLAIN], plain, len))
    {
      printf("FAILED decrypt test vector %d\n", num);
      goto done;
    }

  fail = !match("tag", data[TST_MAC], mac, length[TST_MAC]);
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

static void test_chachapoly(void **state)
{
  int fail = 0;
  int i;

  for (i = 0; i < nitems(g_tests); i++)
    {
      fail += run(i);
    }

  assert_int_equal(fail, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest chachapoly_tests[] = {
      cmocka_unit_test(test_chachapoly),
  };

  return cmocka_run_group_tests(chachapoly_tests, NULL, NULL);
}
