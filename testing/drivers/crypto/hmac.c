/****************************************************************************
 * apps/testing/drivers/crypto/hmac.c
 *
 * SPDX-License-Identifier: ISC
 * SPDX-FileCopyrightText: 2008 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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
#include <crypto/md5.h>
#include <crypto/sha1.h>
#include <crypto/sha2.h>

struct tb
{
  FAR char *key;
  int keylen;
  FAR char *data;
  int datalen;
}
testcase[] =
{
    {
      "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
      16,
      "Hi There",
      8,
    },
    {
      "Jefe",
      4,
      "what do ya want for nothing?",
      28,
    },
    {
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
      16,
      "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
      "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
      "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
      "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
      "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd",
      50
    },
};

FAR char *md5_result[] =
{
  "\x92\x94\x72\x7a\x36\x38\xbb\x1c\x13\xf4\x8e\xf8\x15\x8b\xfc\x9d",
  "\x75\x0c\x78\x3e\x6a\xb0\xb5\x03\xea\xa8\x6e\x31\x0a\x5d\xb7\x38",
  "\x56\xbe\x34\x52\x1d\x14\x4c\x88\xdb\xb8\xc7\x33\xf0\xe8\xb3\xf6"
};

FAR char *sha1_result[] =
{
  "\x67\x5b\x0b\x3a\x1b\x4d\xdf\x4e\x12\x48\x72\xda\x6c\x2f\x63\x2b"
  "\xfe\xd9\x57\xe9",
  "\xef\xfc\xdf\x6a\xe5\xeb\x2f\xa2\xd2\x74\x16\xd5\xf1\x84\xdf\x9c"
  "\x25\x9a\x7c\x79",
  "\xd7\x30\x59\x4d\x16\x7e\x35\xd5\x95\x6f\xd8\x00\x3d\x0d\xb3\xd3"
  "\xf4\x6d\xc7\xbb"
};

FAR char *sha256_result[] =
{
  "\x49\x2c\xe0\x20\xfe\x25\x34\xa5\x78\x9d\xc3\x84\x88\x06\xc7\x8f"
  "\x4f\x67\x11\x39\x7f\x08\xe7\xe7\xa1\x2c\xa5\xa4\x48\x3c\x8a\xa6",
  "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75\xc7"
  "\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec\x38\x43",
  "\x7d\xda\x3c\xc1\x69\x74\x3a\x64\x84\x64\x9f\x94\xf0\xed\xa0\xf9"
  "\xf2\xff\x49\x6a\x97\x33\xfb\x79\x6e\xd5\xad\xb4\x0a\x44\xc3\xc1"
};

int syshmac(int mac, FAR const char *key, size_t keylen,
            FAR const char *s, size_t len, FAR char *out)
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
  session.cipher = 0;
  session.mac = mac;
  session.mackey = (caddr_t)key;
  session.mackeylen = keylen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.flags = 0;
  cryp.src = (caddr_t) s;
  cryp.len = len;
  cryp.dst = 0;
  cryp.mac = (caddr_t) out;
  cryp.iv = 0;
  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      warn("CIOCCRYPT");
      goto err;
    }

  if (ioctl(cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      warn("CIOCFSESSION");
      goto err;
    };

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

  return 1;
}

static int match(unsigned char *a, unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    return (0);

  warnx("hmac mismatch");

  for (i = 0; i < len; i++)
    {
      printf("%02x", a[i]);
    }

  printf("\n");
  for (i = 0; i < len; i++)
    {
      printf("%02x", b[i]);
    }

  printf("\n");

  return (1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  char output[32];
  int ret = 0;
  for (int i = 0; i < nitems(testcase); i++)
    {
      ret += syshmac(CRYPTO_MD5_HMAC, testcase[i].key,
                     testcase[i].keylen,
                     testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshamc md5 failed\n");
        }

      ret += match((unsigned char *)md5_result[i],
                   (unsigned char *)output,
                   MD5_DIGEST_LENGTH);
      if (ret)
        {
          printf("match md5 failed\n");
        }
      else
        {
          printf("hmac md5 success\n");
        }
    }

  for (int i = 0; i < nitems(testcase); i++)
    {
      ret = syshmac(CRYPTO_SHA1_HMAC, testcase[i].key,
                    testcase[i].keylen,
                    testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshamc sha1 failed\n");
        }

      ret = match((unsigned char *)sha1_result[i],
                   (unsigned char *)output,
                   SHA1_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha1 failed\n");
        }
      else
        {
          printf("hmac sha1 success\n");
        }
    }

  for (int i = 0; i < nitems(testcase); i++)
    {
      ret = syshmac(CRYPTO_SHA2_256_HMAC, testcase[i].key,
                    testcase[i].keylen,
                    testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshamc sha256 failed\n");
        }

      ret = match((unsigned char *)sha256_result[i],
                   (unsigned char *)output,
                   SHA256_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha256 failed\n");
        }
      else
        {
          printf("hmac sha256 success\n");
        }
    }

  return 0;
}
