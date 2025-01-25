/****************************************************************************
 * apps/testing/drivers/crypto/aesctr.c
 *
 * SPDX-License-Identifier: ISC
 * SPDX-FileCopyrightText: 2005 Markus Friedl <markus@openbsd.org>
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

static int debug = 0;

enum
{
  TST_KEY,
  TST_IV,
  TST_PLAIN,
  TST_CIPHER,
  TST_NUM
};

/* Test vectors from RFC 3686 */

struct
{
  FAR char *data[TST_NUM];
}
static tests[] =
{
  /* 128 bit key */

  {
    {
      "ae 68 52 f8 12 10 67 cc 4b f7 a5 76 55 77 f3 9e "
      "00 00 00 30",
      "00 00 00 00 00 00 00 00 00 00 00 01",
      "53 69 6e 67 6c 65 20 62 6c 6f 63 6b 20 6d 73 67",
      "e4 09 5d 4f b7 a7 b3 79 2d 61 75 a3 26 13 11 b8"
    }
  },
  {
    {
      "7e 24 06 78 17 fa e0 d7 43 d6 ce 1f 32 53 91 63 "
      "00 6c b6 db",
      "c0 54 3b 59 da 48 d9 0b 00 00 00 01",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
      "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f",
      "51 04 a1 06 16 8a 72 d9 79 0d 41 ee 8e da d3 88 "
      "eb 2e 1e fc 46 da 57 c8 fc e6 30 df 91 41 be 28"
    }
  },
  {
    {
      "76 91 be 03 5e 50 20 a8 ac 6e 61 85 29 f9 a0 dc "
      "00 e0 01 7b",
      "27 77 7f 3f 4a 17 86 f0 00 00 00 01",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
      "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f"
      /* "20 21 22 23" */,

      "c1 cf 48 a8 9f 2f fd d9 cf 46 52 e9 ef db 72 d7 "
      "45 40 a4 2b de 6d 78 36 d5 9a 5c ea ae f3 10 53"

     /* "25 b2 07 2f" */
    }
  },

  /* 192 bit key */

  {
    {
      "16 af 5b 14 5f c9 f5 79 c1 75 f9 3e 3b fb 0e ed "
      "86 3d 06 cc fd b7 85 15 "
      "00 00 00 48",
      "36 73 3c 14 7d 6d 93 cb 00 00 00 01",
      "53 69 6e 67 6c 65 20 62 6c 6f 63 6b 20 6d 73 67",
      "4b 55 38 4f e2 59 c9 c8 4e 79 35 a0 03 cb e9 28",
    }
  },
  {
    {
      "7c 5c b2 40 1b 3d c3 3c 19 e7 34 08 19 e0 f6 9c "
      "67 8c 3d b8 e6 f6 a9 1a "
      "00 96 b0 3b",
      "02 0c 6e ad c2 cb 50 0d 00 00 00 01",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
      "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f",
      "45 32 43 fc 60 9b 23 32 7e df aa fa 71 31 cd 9f "
      "84 90 70 1c 5a d4 a7 9c fc 1f e0 ff 42 f4 fb 00",
    }
  },
  {
    {
      "02 bf 39 1e e8 ec b1 59 b9 59 61 7b 09 65 27 9b "
      "f5 9b 60 a7 86 d3 e0 fe "
      "00 07 bd fd",
      "5c bd 60 27 8d cc 09 12 00 00 00 01",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
      "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f"
      /* "20 21 22 23" */,

      "96 89 3f c5 5e 5c 72 2f 54 0b 7d d1 dd f7 e7 58 "
      "d2 88 bc 95 c6 91 65 88 45 36 c8 11 66 2f 21 88"
      /* "ab ee 09 35" */,
    }
  },

  /* 256 bit key */

  {
    {
      "77 6b ef f2 85 1d b0 6f 4c 8a 05 42 c8 69 6f 6c "
      "6a 81 af 1e ec 96 b4 d3 7f c1 d6 89 e6 c1 c1 04 "
      "00 00 00 60",
      "db 56 72 c9 7a a8 f0 b2 00 00 00 01",
      "53 69 6e 67 6c 65 20 62 6c 6f 63 6b 20 6d 73 67",
      "14 5a d0 1d bf 82 4e c7 56 08 63 dc 71 e3 e0 c0"
    }
  },
  {
    {
      "f6 d6 6d 6b d5 2d 59 bb 07 96 36 58 79 ef f8 86 "
      "c6 6d d5 1a 5b 6a 99 74 4b 50 59 0c 87 a2 38 84 "
      "00 fa ac 24",
      "c1 58 5e f1 5a 43 d8 75 00 00 00 01",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
      "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f",
      "f0 5e 23 1b 38 94 61 2c 49 ee 00 0b 80 4e b2 a9 "
      "b8 30 6b 50 8f 83 9d 6a 55 30 83 1d 93 44 af 1c",
    }
  },
  {
    {
      "ff 7a 61 7c e6 91 48 e4 f1 72 6e 2f 43 58 1d e2 "
      "aa 62 d9 f8 05 53 2e df f1 ee d6 87 fb 54 15 3d "
      "00 1c c5 b7",
      "51 a5 1d 70 a1 c1 11 48 00 00 00 01",
      "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
      "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f"
      /* "20 21 22 23" */,

      "eb 6c 52 82 1d 0b bb f7 ce 75 94 46 2a ca 4f aa "
      "b4 07 df 86 65 69 fd 07 f4 8c c0 b5 83 d6 07 1f"
      /* "1e c0 e6 b8" */,
    }
  },
};

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
  session.cipher = CRYPTO_AES_CTR;
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
    {
      return (1);
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

  return (0);
}

static int run(int num)
{
  int i;
  int fail = 1;
  int len;
  int j;
  int length[TST_NUM];
  u_long val;
  FAR char *ep;
  FAR char *from;
  FAR u_char *p;
  FAR u_char *data[TST_NUM];

  for (i = 0; i < TST_NUM; i++)
    {
      data[i] = NULL;
    }

  for (i = 0; i < TST_NUM; i++)
    {
      from = tests[num].data[i];
      if (debug)
        {
          printf("%s\n", from);
        }

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
            break;
        }

      length[i] = j + 1;
      data[i] = p;
    }

  len = length[TST_PLAIN];
  if ((p = malloc(len)) == 0)
    {
      warn("malloc");
      return (1);
    }

  if (syscrypt(data[TST_KEY], length[TST_KEY],
      data[TST_IV], data[TST_PLAIN], p,
      length[TST_PLAIN], 0) < 0)
    {
      warnx("crypt with /dev/crypto failed");
      goto done;
    }

  fail = !match(data[TST_CIPHER], p, len);
  printf("%s test vector %d\n", fail ? "FAILED" : "OK", num);
done:
  for (i = 0; i < TST_NUM; i++)
    {
      free(data[i]);
    }

  free(p);
  return (fail);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  int fail = 0;
  int i;

  for (i = 0; i < nitems(tests); i++)
    {
      fail += run(i);
    }

  exit((fail > 0) ? 1 : 0);
}
