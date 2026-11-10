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
#include <crypto/rmd160.h>

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
    {
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa",
      131,
      "Test Using Larger Than Block-Size Key - Hash Key First",
      54
    },
    {
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
      "\xaa",
      131,
      "This is a test using a larger than block-size key and a "
      "larger than block-size data. The key needs to be hashed "
      "before being used by the HMAC algorithm.",
      152
    },
};

FAR char *md5_result[] =
{
  "\x92\x94\x72\x7a\x36\x38\xbb\x1c\x13\xf4\x8e\xf8\x15\x8b\xfc\x9d",
  "\x75\x0c\x78\x3e\x6a\xb0\xb5\x03\xea\xa8\x6e\x31\x0a\x5d\xb7\x38",
  "\x56\xbe\x34\x52\x1d\x14\x4c\x88\xdb\xb8\xc7\x33\xf0\xe8\xb3\xf6",
  "\xbf\xec\xaf\x4e\xff\xf9\x0a\x3a\x66\x8f\x39\x22\xfe\xc3\x76\x2d",
  "\x09\xb8\xae\x7b\x15\xad\xbb\xb2\x43\xac\xa3\x49\x1b\x51\x51\x2b"
};

FAR char *sha1_result[] =
{
  "\x67\x5b\x0b\x3a\x1b\x4d\xdf\x4e\x12\x48\x72\xda\x6c\x2f\x63\x2b"
  "\xfe\xd9\x57\xe9",
  "\xef\xfc\xdf\x6a\xe5\xeb\x2f\xa2\xd2\x74\x16\xd5\xf1\x84\xdf\x9c"
  "\x25\x9a\x7c\x79",
  "\xd7\x30\x59\x4d\x16\x7e\x35\xd5\x95\x6f\xd8\x00\x3d\x0d\xb3\xd3"
  "\xf4\x6d\xc7\xbb",
  "\x90\xd0\xda\xce\x1c\x1b\xdc\x95\x73\x39\x30\x78\x03\x16\x03\x35"
  "\xbd\xe6\xdf\x2b",
  "\x21\x7e\x44\xbb\x08\xb6\xe0\x6a\x2d\x6c\x30\xf3\xcb\x9f\x53\x7f"
  "\x97\xc6\x33\x56"
};

FAR char *sha224_result[] =
{
  "\x4e\x84\x1c\xe7\xa4\xae\x83\xfb\xcf\x71\xe3\xcd\x64\xbf\xbf\x27"
  "\x7f\x73\xa1\x46\x80\xaa\xe8\xc5\x18\xac\x78\x61",
  "\xa3\x0e\x01\x09\x8b\xc6\xdb\xbf\x45\x69\x0f\x3a\x7e\x9e\x6d\x0f"
  "\x8b\xbe\xa2\xa3\x9e\x61\x48\x00\x8f\xd0\x5e\x44",
  "\xcb\xff\x7c\x27\x16\xbb\xaa\x7c\x77\xbe\xd4\xf4\x91\xd3\xe8\x45"
  "\x6c\xb6\xc5\x74\xe9\x2f\x67\x2b\x29\x1a\xcf\x5b",
  "\x95\xe9\xa0\xdb\x96\x20\x95\xad\xae\xbe\x9b\x2d\x6f\x0d\xbc\xe2"
  "\xd4\x99\xf1\x12\xf2\xd2\xb7\x27\x3f\xa6\x87\x0e",
  "\x3a\x85\x41\x66\xac\x5d\x9f\x02\x3f\x54\xd5\x17\xd0\xb3\x9d\xbd"
  "\x94\x67\x70\xdb\x9c\x2b\x95\xc9\xf6\xf5\x65\xd1"
};

FAR char *sha256_result[] =
{
  "\x49\x2c\xe0\x20\xfe\x25\x34\xa5\x78\x9d\xc3\x84\x88\x06\xc7\x8f"
  "\x4f\x67\x11\x39\x7f\x08\xe7\xe7\xa1\x2c\xa5\xa4\x48\x3c\x8a\xa6",
  "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75\xc7"
  "\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec\x38\x43",
  "\x7d\xda\x3c\xc1\x69\x74\x3a\x64\x84\x64\x9f\x94\xf0\xed\xa0\xf9"
  "\xf2\xff\x49\x6a\x97\x33\xfb\x79\x6e\xd5\xad\xb4\x0a\x44\xc3\xc1",
  "\x60\xe4\x31\x59\x1e\xe0\xb6\x7f\x0d\x8a\x26\xaa\xcb\xf5\xb7\x7f"
  "\x8e\x0b\xc6\x21\x37\x28\xc5\x14\x05\x46\x04\x0f\x0e\xe3\x7f\x54",
  "\x9b\x09\xff\xa7\x1b\x94\x2f\xcb\x27\x63\x5f\xbc\xd5\xb0\xe9\x44"
  "\xbf\xdc\x63\x64\x4f\x07\x13\x93\x8a\x7f\x51\x53\x5c\x3a\x35\xe2"
};

/* HMAC-SHA384, HMAC-SHA512 and HMAC-RIPEMD160 over the same five
 * inputs, computed with two independent implementations.
 */

FAR char *sha384_result[] =
{
  "\x7a\xfa\xa6\x33\xe2\x0d\x37\x9b\x02\x39\x59\x15\xfb\xc3\x85\xff"
  "\x8d\xc2\x7d\xcd\x38\x85\xe1\x06\x8a\xb9\x42\xee\xab\x52\xec\x1f"
  "\x20\xad\x38\x2a\x92\x37\x0d\x8b\x2e\x0a\xc8\xb8\x3c\x4d\x53\xbf",
  "\xaf\x45\xd2\xe3\x76\x48\x40\x31\x61\x7f\x78\xd2\xb5\x8a\x6b\x1b"
  "\x9c\x7e\xf4\x64\xf5\xa0\x1b\x47\xe4\x2e\xc3\x73\x63\x22\x44\x5e"
  "\x8e\x22\x40\xca\x5e\x69\xe2\xc7\x8b\x32\x39\xec\xfa\xb2\x16\x49",
  "\x13\x83\xe8\x2e\x28\x28\x6b\x91\xf4\xcc\x7a\xfb\xd1\x3d\x5b\x5c"
  "\x6f\x88\x7c\x05\xe7\xc4\x54\x24\x84\x04\x3a\x37\xa5\xfe\x45\x80"
  "\x2a\x94\x70\xfb\x66\x3b\xd7\xb6\x57\x0f\xe2\xf5\x03\xfc\x92\xf5",
  "\x4e\xce\x08\x44\x85\x81\x3e\x90\x88\xd2\xc6\x3a\x04\x1b\xc5\xb4"
  "\x4f\x9e\xf1\x01\x2a\x2b\x58\x8f\x3c\xd1\x1f\x05\x03\x3a\xc4\xc6"
  "\x0c\x2e\xf6\xab\x40\x30\xfe\x82\x96\x24\x8d\xf1\x63\xf4\x49\x52",
  "\x66\x17\x17\x8e\x94\x1f\x02\x0d\x35\x1e\x2f\x25\x4e\x8f\xd3\x2c"
  "\x60\x24\x20\xfe\xb0\xb8\xfb\x9a\xdc\xce\xbb\x82\x46\x1e\x99\xc5"
  "\xa6\x78\xcc\x31\xe7\x99\x17\x6d\x38\x60\xe6\x11\x0c\x46\x52\x3e"
};

FAR char *sha512_result[] =
{
  "\x76\x41\xc4\x8a\x3b\x4a\xa8\xf8\x87\xc0\x7b\x3e\x83\xf9\x6a\xff"
  "\xb8\x9c\x97\x8f\xed\x8c\x96\xfc\xbb\xf4\xad\x59\x6e\xeb\xfe\x49"
  "\x6f\x9f\x16\xda\x6c\xd0\x80\xba\x39\x3c\x6f\x36\x5a\xd7\x2b\x50"
  "\xd1\x5c\x71\xbf\xb1\xd6\xb8\x1f\x66\xa9\x11\x78\x6c\x6c\xe9\x32",
  "\x16\x4b\x7a\x7b\xfc\xf8\x19\xe2\xe3\x95\xfb\xe7\x3b\x56\xe0\xa3"
  "\x87\xbd\x64\x22\x2e\x83\x1f\xd6\x10\x27\x0c\xd7\xea\x25\x05\x54"
  "\x97\x58\xbf\x75\xc0\x5a\x99\x4a\x6d\x03\x4f\x65\xf8\xf0\xe6\xfd"
  "\xca\xea\xb1\xa3\x4d\x4a\x6b\x4b\x63\x6e\x07\x0a\x38\xbc\xe7\x37",
  "\xad\x9b\x5c\x7d\xe7\x26\x93\x73\x7c\xd5\xe9\xd9\xf4\x11\x70\xd1"
  "\x88\x41\xfe\xc1\x20\x1c\x1c\x1b\x02\xe0\x5c\xae\x11\x67\x18\x00"
  "\x9f\x77\x1c\xad\x99\x46\xdd\xbf\x7e\x3c\xde\x3e\x81\x8d\x9a\xe8"
  "\x5d\x91\xb2\xba\xda\xe9\x41\x72\xd0\x96\xa4\x4a\x79\xc9\x1e\x86",
  "\x80\xb2\x42\x63\xc7\xc1\xa3\xeb\xb7\x14\x93\xc1\xdd\x7b\xe8\xb4"
  "\x9b\x46\xd1\xf4\x1b\x4a\xee\xc1\x12\x1b\x01\x37\x83\xf8\xf3\x52"
  "\x6b\x56\xd0\x37\xe0\x5f\x25\x98\xbd\x0f\xd2\x21\x5d\x6a\x1e\x52"
  "\x95\xe6\x4f\x73\xf6\x3f\x0a\xec\x8b\x91\x5a\x98\x5d\x78\x65\x98",
  "\xe3\x7b\x6a\x77\x5d\xc8\x7d\xba\xa4\xdf\xa9\xf9\x6e\x5e\x3f\xfd"
  "\xde\xbd\x71\xf8\x86\x72\x89\x86\x5d\xf5\xa3\x2d\x20\xcd\xc9\x44"
  "\xb6\x02\x2c\xac\x3c\x49\x82\xb1\x0d\x5e\xeb\x55\xc3\xe4\xde\x15"
  "\x13\x46\x76\xfb\x6d\xe0\x44\x60\x65\xc9\x74\x40\xfa\x8c\x6a\x58"
};

FAR char *ripemd160_result[] =
{
  "\x1f\x75\x1b\xf7\x2b\xf8\xad\x4f\x19\x0e\xb1\x61\x65\x19\xd6\x8c"
  "\xf5\x98\xb5\x30",
  "\xdd\xa6\xc0\x21\x3a\x48\x5a\x9e\x24\xf4\x74\x20\x64\xa7\xf0\x33"
  "\xb4\x3c\x40\x69",
  "\x53\x81\x4b\x60\xf2\x3b\x19\xa1\x83\x5f\x7f\x3c\xb9\x0c\x74\x33"
  "\xe2\x71\x61\x87",
  "\x71\xbb\x52\xd2\x64\x08\xe5\xa2\x21\x39\x3d\x58\x11\xb0\x3c\xc7"
  "\xf9\x4b\xcd\x3a",
  "\x1e\xd1\x06\xe5\xa8\xef\x0a\x90\xef\xa3\xbe\xb0\x6b\x39\x1e\x86"
  "\x93\xcd\x31\x37"
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
  char output[64];
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
      ret = syshmac(CRYPTO_SHA2_224_HMAC, testcase[i].key,
                    testcase[i].keylen,
                    testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshamc sha224 failed\n");
        }

      ret = match((unsigned char *)sha224_result[i],
                   (unsigned char *)output,
                   SHA224_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha224 failed\n");
        }
      else
        {
          printf("hmac sha224 success\n");
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

  for (int i = 0; i < nitems(testcase); i++)
    {
      ret = syshmac(CRYPTO_SHA2_384_HMAC, testcase[i].key,
                    testcase[i].keylen,
                    testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshmac sha384 failed\n");
        }

      ret = match((unsigned char *)sha384_result[i],
                   (unsigned char *)output,
                   SHA384_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha384 failed\n");
        }
      else
        {
          printf("hmac sha384 success\n");
        }
    }

  for (int i = 0; i < nitems(testcase); i++)
    {
      ret = syshmac(CRYPTO_SHA2_512_HMAC, testcase[i].key,
                    testcase[i].keylen,
                    testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshmac sha512 failed\n");
        }

      ret = match((unsigned char *)sha512_result[i],
                   (unsigned char *)output,
                   SHA512_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha512 failed\n");
        }
      else
        {
          printf("hmac sha512 success\n");
        }
    }

  for (int i = 0; i < nitems(testcase); i++)
    {
      ret = syshmac(CRYPTO_RIPEMD160_HMAC, testcase[i].key,
                    testcase[i].keylen,
                    testcase[i].data, testcase[i].datalen, output);
      if (ret)
        {
          printf("syshmac ripemd160 failed\n");
        }

      ret = match((unsigned char *)ripemd160_result[i],
                   (unsigned char *)output,
                   RMD160_DIGEST_LENGTH);
      if (ret)
        {
          printf("match ripemd160 failed\n");
        }
      else
        {
          printf("hmac ripemd160 success\n");
        }
    }

  return 0;
}
