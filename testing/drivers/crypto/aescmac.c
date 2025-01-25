/****************************************************************************
 * apps/testing/drivers/crypto/aescmac.c
 *
 * SPDX-License-Identifier: ISC
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/* AES-CMAC test vector from
 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/AES_CMAC.pdf
 */

static const unsigned char g_test_message[] =
{
    0x6b, 0xc1, 0xbe, 0xe2,     0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11,     0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57,     0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac,     0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46,     0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19,     0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45,     0xdf, 0x4f, 0x9b, 0x17,
    0xad, 0x2b, 0x41, 0x7b,     0xe6, 0x6c, 0x37, 0x10
};

static const unsigned char g_aes_128_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16,     0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,     0x09, 0xcf, 0x4f, 0x3c
};

static const  unsigned int g_message_lengths[4] =
{
    0,
    16,
    20,
    64
};

static const unsigned char g_aes_128_expected_result[4][16] =
{
    {
        /* Example #1 */

        0xbb, 0x1d, 0x69, 0x29,     0xe9, 0x59, 0x37, 0x28,
        0x7f, 0xa3, 0x7d, 0x12,     0x9b, 0x75, 0x67, 0x46
    },
    {
        /* Example #2 */

        0x07, 0x0a, 0x16, 0xb4,     0x6b, 0x4d, 0x41, 0x44,
        0xf7, 0x9b, 0xdd, 0x9d,     0xd0, 0x4a, 0x28, 0x7c
    },
    {
        /* Example #3 */

        0x7d, 0x85, 0x44, 0x9e,     0xa6, 0xea, 0x19, 0xc8,
        0x23, 0xa7, 0xbf, 0x78,     0x83, 0x7d, 0xfa, 0xde
    },
    {
        /* Example #4 */

        0x51, 0xf0, 0xbe, 0xbf,     0x7e, 0x3b, 0x9d, 0x92,
        0xfc, 0x49, 0x74, 0x17,     0x79, 0x36, 0x3c, 0xfe
    }
};

static int syscrypt(FAR const unsigned char *key, size_t klen,
                    FAR const unsigned char *message, size_t len,
                    FAR unsigned char *output, size_t outlen)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;

  if ((fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      perror("/dev/crypto");
      goto err;
    }

  if (ioctl(fd, CRIOGET, &cryptodev_fd) == -1)
    {
      perror("CRIOGET");
      goto err;
    }

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CMAC;
  session.key = (caddr_t)key;
  session.keylen = klen;
  session.mac = CRYPTO_AES_128_CMAC;
  session.mackey = (caddr_t)key;
  session.mackeylen = klen;

  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      perror("CIOCGSESSION");
      goto err;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.src = (caddr_t)message;
  cryp.mac = (caddr_t)output;

  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT");
      goto err;
    }

  if (ioctl(cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      perror("CIOCFSESSION");
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
                 size_t len)
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

int main(int argc, FAR char *argv[])
{
  int ret;
  unsigned char out[64];

  for (int i = 0; i < 4; i++)
    {
      ret = syscrypt(g_aes_128_key, sizeof(g_aes_128_key), g_test_message,
                     g_message_lengths[i], out, sizeof(out));
      if (ret)
        {
          printf("aes cmac failed in testcase:%d\n", i);
          return -1;
        }

      ret = match(out, g_aes_128_expected_result[i], 16);
      if (ret)
        {
          printf("aes cmac failed in testcase:%d\n", i);
          return -1;
        }

      printf("aescbc test %d ok\n", i);
    }

  return 0;
}
