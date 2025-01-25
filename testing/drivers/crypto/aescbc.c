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
 */

struct tb
{
  FAR const char *key;
  FAR const char *iv;
  FAR const char *plain;
  FAR const char *cipher;
  uint32_t len;
}
static testcase [] =
{
  {
    "\x06\xa9\x21\x40\x36\xb8\xa1\x5b\x51\x2e\x03\xd5\x34\x12\x00\x06",
    "\x3d\xaf\xba\x42\x9d\x9e\xb4\x30\xb4\x22\xda\x80\x2c\x9f\xac\x41",
    "Single block msg",
    "\xe3\x53\x77\x9c\x10\x79\xae\xb8\x27\x08\x94\x2d\xbe\x77\x18\x1a",
    16,
  },
  {
    "\xc2\x86\x69\x6d\x88\x7c\x9a\xa0\x61\x1b\xbb\x3e\x20\x25\xa4\x5a",
    "\x56\x2e\x17\x99\x6d\x09\x3d\x28\xdd\xb3\xba\x69\x5a\x2e\x6f\x58",
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f",
    "\xd2\x96\xcd\x94\xc2\xcc\xcf\x8a\x3a\x86\x30\x28\xb5\xe1\xdc\x0a"
    "\x75\x86\x60\x2d\x25\x3c\xff\xf9\x1b\x82\x66\xbe\xa6\xd6\x1a\xb1",
    32
  },
  {
    "\x6c\x3e\xa0\x47\x76\x30\xce\x21\xa2\xce\x33\x4a\xa7\x46\xc2\xcd",
    "\xc7\x82\xdc\x4c\x09\x8c\x66\xcb\xd9\xcd\x27\xd8\x25\x68\x2c\x81",
    "This is a 48-byte message (exactly 3 AES blocks)",
    "\xd0\xa0\x2b\x38\x36\x45\x17\x53\xd4\x93\x66\x5d\x33\xf0\xe8\x86"
    "\x2d\xea\x54\xcd\xb2\x93\xab\xc7\x50\x69\x39\x27\x67\x72\xf8\xd5"
    "\x02\x1c\x19\x21\x6b\xad\x52\x5c\x85\x79\x69\x5d\x83\xba\x26\x84",
    48
  },
  {
    "\x56\xe4\x7a\x38\xc5\x59\x89\x74\xbc\x46\x90\x3d\xba\x29\x03\x49",
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
  }
};

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
  session.cipher = CRYPTO_AES_CBC;
  session.key = (caddr_t) key;
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
      ret = syscrypt(testcase[i].key, 16, testcase[i].iv, testcase[i].plain,
                     out, testcase[i].len, 1);
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

      ret = syscrypt(testcase[i].key, 16, testcase[i].iv, testcase[i].cipher,
                     out, testcase[i].len, 0);
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
    }

  printf("aescbc test ok\n");
  return 0;
}
