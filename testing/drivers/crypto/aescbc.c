/****************************************************************************
 * apps/testing/drivers/crypto/aescbc.c
 *
 * SPDX-License-Identifier: ISC
 * SPDX-FileCopyrightText: 2005 Markus Friedl.  All rights reserved.
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

/* aescbc test case is come form
 * https://www.rfc-editor.org/rfc/rfc3602
 * The AES-192/256 vectors are from NIST SP 800-38A.
 */

struct tb
{
  FAR const char *key;
  size_t klen;
  FAR const char *iv;
  FAR const char *plain;
  FAR const char *cipher;
  uint32_t len;
}
static testcase [] =
{
  {
    "\x06\xa9\x21\x40\x36\xb8\xa1\x5b\x51\x2e\x03\xd5\x34\x12\x00\x06",
    16,
    "\x3d\xaf\xba\x42\x9d\x9e\xb4\x30\xb4\x22\xda\x80\x2c\x9f\xac\x41",
    "Single block msg",
    "\xe3\x53\x77\x9c\x10\x79\xae\xb8\x27\x08\x94\x2d\xbe\x77\x18\x1a",
    16,
  },
  {
    "\xc2\x86\x69\x6d\x88\x7c\x9a\xa0\x61\x1b\xbb\x3e\x20\x25\xa4\x5a",
    16,
    "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58",
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f",
    "\xd2\x96\xcd\x94\xc2\xcc\xcf\x8a\x3a\x86\x30\x28\xb5\xe1\xdc\x0a"
    "\x75\x86\x60\x2d\x25\x3c\xff\xf9\x1b\x82\x66\xbe\xa6\xd6\x1a\xb1",
    32
  },
  {
    "\x6c\x3e\xa0\x47\x76\x30\xce\x21\xa2\xce\x33\x4a\xa7\x46\xc2\xcd",
    16,
    "\xc7\x82\xdc\x4c\x09\x8c\x66\xcb\xd9\xcd\x27\xd8\x25\x68\x2c\x81",
    "This is a 48-byte message (exactly 3 AES blocks)",
    "\xd0\xa0\x2b\x38\x36\x45\x17\x53\xd4\x93\x66\x5d\x33\xf0\xe8\x86"
    "\x2d\xea\x54\xcd\xb2\x93\xab\xc7\x50\x69\x39\x27\x67\x72\xf8\xd5"
    "\x02\x1c\x19\x21\x6b\xad\x52\x5c\x85\x79\x69\x5d\x83\xba\x26\x84",
    48
  },
  {
    "\x56\xe4\x7a\x38\xc5\x59\x89\x74\xbc\x46\x90\x3d\xba\x29\x03\x49",
    16,
    "\x8c\xe8\x2e\xef\xbe\xa0\xda\x3c\x44\x69\x9e\xd7\xdb\x51\xb7\xd9",
    "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
    "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
    "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
    "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf",
    "\xc3\x0e\x32\xff\xed\xc0\x77\x4e\x6a\xff\x6a\xf0\x86\x9f\x71\xaa"
    "\x0f\x3a\xf0\x7a\x9a\x31\xa9\xc6\x84\xdb\x20\x7e\xb0\xef\x8e\x4e"
    "\x35\x90\x7a\xa6\x32\xc3\xff\xdf\x86\x8b\xb7\xb2\x9d\x3d\x46\xad"
    "\x83\xce\x9f\x9a\x10\x2e\xe9\x9d\x49\xa5\x3e\x87\xf4\xc3\xda\x55",
    64
  },
  {
    "\x8e\x73\xb0\xf7\xda\x0e\x64\x52\xc8\x10\xf3\x2b\x80\x90\x79\xe5"
    "\x62\xf8\xea\xd2\x52\x2c\x6b\x7b",
    24,
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
    "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a"
    "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51"
    "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef"
    "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
    "\x4f\x02\x1d\xb2\x43\xbc\x63\x3d\x71\x78\x18\x3a\x9f\xa0\x71\xe8"
    "\xb4\xd9\xad\xa9\xad\x7d\xed\xf4\xe5\xe7\x38\x76\x3f\x69\x14\x5a"
    "\x57\x1b\x24\x20\x12\xfb\x7a\xe0\x7f\xa9\xba\xac\x3d\xf1\x02\xe0"
    "\x08\xb0\xe2\x79\x88\x59\x88\x81\xd9\x20\xa9\xe6\x4f\x56\x15\xcd",
    64,
  },
  {
    "\x60\x3d\xeb\x10\x15\xca\x71\xbe\x2b\x73\xae\xf0\x85\x7d\x77\x81"
    "\x1f\x35\x2c\x07\x3b\x61\x08\xd7\x2d\x98\x10\xa3\x09\x14\xdf\xf4",
    32,
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
    "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a"
    "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51"
    "\x30\xc8\x1c\x46\xa3\x5c\xe4\x11\xe5\xfb\xc1\x19\x1a\x0a\x52\xef"
    "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
    "\xf5\x8c\x4c\x04\xd6\xe5\xf1\xba\x77\x9e\xab\xfb\x5f\x7b\xfb\xd6"
    "\x9c\xfc\x4e\x96\x7e\xdb\x80\x8d\x67\x9f\x77\x7b\xc6\x70\x2c\x7d"
    "\x39\xf2\x33\x69\xa9\xd9\xba\xcf\xa5\x30\xe2\x63\x04\x23\x14\x61"
    "\xb2\xeb\x05\xe2\xc3\x9b\xe9\xfc\xda\x6c\x19\x07\x8c\x6a\x9d\x1b",
    64,
  },
};

/* The cryptodev AES-128-CBC id is CRYPTO_AES_CBC; pick the variant that
 * matches the key length of the session.
 */

static uint32_t aescbc_cipher_id(size_t klen)
{
  if (klen == 24)
    {
      return CRYPTO_AES_192_CBC;
    }

  if (klen == 32)
    {
      return CRYPTO_AES_256_CBC;
    }

  return CRYPTO_AES_CBC;
}

static int syscrypt(FAR const char *key, size_t klen,
                    FAR const char *iv, FAR const char *in,
                    FAR unsigned char *out, size_t len, int encrypt)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;
  char tmp_iv[16];

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
  session.cipher = aescbc_cipher_id(klen);
  session.key = (caddr_t) key;
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  session.keylen = klen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  memcpy(tmp_iv, iv, 16);
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.olen = len;
  cryp.ivlen = 16;
  cryp.src = (caddr_t) in;
  cryp.dst = (caddr_t) out;
  cryp.iv = (caddr_t) tmp_iv;
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

/* One block per CIOCCRYPT, chained with COP_FLAG_UPDATE; the result must
 * match the one-shot run above.
 */

static int syscrypt_stream(FAR const char *key, size_t klen,
                           FAR const char *iv, FAR const char *in,
                           FAR unsigned char *out, size_t len, int encrypt)
{
  struct session_op session;
  struct crypt_op cryp;
  size_t offset = 0;
  int cryptodev_fd = -1;
  int fd = -1;
  char tmp_iv[16];

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
  session.cipher = aescbc_cipher_id(klen);
  session.key = (caddr_t) key;
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  session.keylen = klen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  memcpy(tmp_iv, iv, 16);
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.mac = 0;
  cryp.iv = (caddr_t) tmp_iv;
  cryp.ivlen = 16;
  while (len >= 16)
    {
      cryp.len = 16;
      cryp.olen = 16;
      cryp.src = (caddr_t) in + offset;
      cryp.dst = (caddr_t) out + offset;
      if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
        {
          warn("CIOCCRYPT");
          goto err;
        }

      cryp.flags |= COP_FLAG_UPDATE;
      len -= 16;
      offset += 16;
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
    {
      return 0;
    }

  warnx("ciphertext mismatch");

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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int ret;
  unsigned char out[64];

  for (int i = 0; i < nitems(testcase); i++)
    {
      ret = syscrypt(testcase[i].key, testcase[i].klen, testcase[i].iv,
                     testcase[i].plain, out, testcase[i].len, 1);
      if (ret)
        {
          printf("aescbc encrypt field in testcase:%d\n", i);
          return -1;
        }

      ret = match(out, (FAR unsigned char *)testcase[i].cipher,
                  testcase[i].len);
      if (ret)
        {
          printf("aescbc encrypt field in testcase:%d\n", i);
          return -1;
        }

      ret = syscrypt(testcase[i].key, testcase[i].klen, testcase[i].iv,
                     testcase[i].cipher, out, testcase[i].len, 0);
      if (ret)
        {
          printf("aescbc decrypt field in testcase:%d\n", i);
          return -1;
        }

      ret = match(out, (FAR unsigned char *)testcase[i].plain,
                  testcase[i].len);
      if (ret)
        {
          printf("aescbc decrypt field in testcase:%d\n", i);
          return -1;
        }

      ret = syscrypt_stream(testcase[i].key, testcase[i].klen,
                            testcase[i].iv, testcase[i].plain, out,
                            testcase[i].len, 1);
      if (ret)
        {
          printf("aescbc encrypt stream field in testcase:%d\n", i);
          return -1;
        }

      ret = match(out, (FAR unsigned char *)testcase[i].cipher,
                  testcase[i].len);
      if (ret)
        {
          printf("aescbc encrypt stream field in testcase:%d\n", i);
          return -1;
        }

      ret = syscrypt_stream(testcase[i].key, testcase[i].klen,
                            testcase[i].iv, testcase[i].cipher, out,
                            testcase[i].len, 0);
      if (ret)
        {
          printf("aescbc decrypt stream field in testcase:%d\n", i);
          return -1;
        }

      ret = match(out, (FAR unsigned char *)testcase[i].plain,
                  testcase[i].len);
      if (ret)
        {
          printf("aescbc decrypt stream field in testcase:%d\n", i);
          return -1;
        }

      printf("aescbc test vector %d ok\n", i);
    }

  printf("aescbc test ok\n");
  return 0;
}
