/****************************************************************************
 * apps/testing/drivers/crypto/3descbc.c
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * SPDX-FileCopyrightText: 2002 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

static int syscrypt(FAR const unsigned char *key, size_t klen,
                    FAR const unsigned char *iv,
                    FAR const unsigned char *in, FAR unsigned char *out,
                    size_t len, int encrypt)
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
  session.cipher = CRYPTO_3DES_CBC;
  session.key = (caddr_t) key;
  session.keylen = klen;
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
  cryp.src = (caddr_t) in;
  cryp.dst = (caddr_t) out;
  cryp.iv = (caddr_t) iv;
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
  return (0);

err:
  if (cryptodev_fd != -1)
    {
      close(cryptodev_fd);
    }

  if (fd != -1)
    {
      close(fd);
    }

  return (-1);
}

static int match(FAR unsigned char *a, FAR unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    return (1);

  warnx("decrypt/plaintext mismatch");

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

  return (0);
}
#define SZ 16

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  unsigned char iv0[8];
  unsigned char iv[8];
  unsigned char key[24] = "012345670123456701234567";
  unsigned char b1[SZ];
  unsigned char b2[SZ];
  int i;
  int fail = 0;
  u_int32_t rand = 0;

  /* setup data and iv */

  for (i = 0; i < sizeof(b1); i++ )
    {
      if (i % 4 == 0)
        {
          rand = random();
        }

      b1[i] = rand;
      rand >>= 8;
    }

  for (i = 0; i < sizeof(iv0); i++ )
    {
      if (i % 4 == 0)
        {
          rand = random();
        }

      iv0[i] = rand;
      rand >>= 8;
    }

  memset(b2, 0, sizeof(b2));
  memcpy(iv, iv0, sizeof(iv0));

  if (syscrypt(key, sizeof(key), iv, b1, b2, sizeof(b1), 1) < 0)
    {
      warnx("encrypt with /dev/crypto failed");
      fail++;
    }

  memcpy(iv, iv0, sizeof(iv0));
  if (syscrypt(key, sizeof(key), iv, b2, b2, sizeof(b1), 0) < 0)
    {
      warnx("decrypt with /dev/crypto failed");
      fail++;
    }

  if (!match(b1, b2, sizeof(b1)))
    {
      fail++;
    }
  else
    {
      printf("ok, encrypt with /dev/crypto, decrypt with /dev/crypto\n");
    }

  exit((fail > 0) ? 1 : 0);
}
