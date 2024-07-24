/****************************************************************************
 * apps/testing/crypto/ecdsa.c
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SECP256R1_KEYLEN 32

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned char sha256_message[32] =
{
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
  0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
  0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int ecdsa_sign(int op, FAR unsigned char *r, size_t rlen,
                      FAR unsigned char *s, size_t slen,
                      FAR unsigned char *d, size_t dlen,
                      FAR const unsigned char *buf, size_t buflen)
{
  struct crypt_kop cryptk;
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

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_iparams = 2;
  cryptk.crk_oparams = 2;
  cryptk.crk_param[0].crp_p = (caddr_t)d;
  cryptk.crk_param[0].crp_nbits = dlen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)buf;
  cryptk.crk_param[1].crp_nbits = buflen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)r;
  cryptk.crk_param[2].crp_nbits = rlen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)s;
  cryptk.crk_param[3].crp_nbits = slen * 8;

  if (ioctl(cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      goto err;
    }

  close(cryptodev_fd);
  close(fd);
  return cryptk.crk_status;

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

static int ecdsa_verify(int op, FAR const unsigned char *qx, size_t qxlen,
                        FAR const unsigned char *qy, size_t qylen,
                        FAR const unsigned char *qz, size_t qzlen,
                        FAR const unsigned char *r, size_t rlen,
                        FAR const unsigned char *s, size_t slen,
                        FAR const unsigned char *buf, size_t buflen)
{
  struct crypt_kop cryptk;
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

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_iparams = 6;
  cryptk.crk_oparams = 0;
  cryptk.crk_param[0].crp_p = (caddr_t)qx;
  cryptk.crk_param[0].crp_nbits = qxlen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)qy;
  cryptk.crk_param[1].crp_nbits = qylen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)qz;
  cryptk.crk_param[2].crp_nbits = qzlen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)r;
  cryptk.crk_param[3].crp_nbits = rlen * 8;
  cryptk.crk_param[4].crp_p = (caddr_t)s;
  cryptk.crk_param[4].crp_nbits = slen * 8;
  cryptk.crk_param[5].crp_p = (caddr_t)buf;
  cryptk.crk_param[5].crp_nbits = buflen * 8;

  if (ioctl(cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      goto err;
    }

  close(cryptodev_fd);
  close(fd);
  return cryptk.crk_status;

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

static int ecdsa_genkey(int op, FAR const unsigned char *d, size_t dlen,
                        FAR const unsigned char *qx, size_t qxlen,
                        FAR const unsigned char *qy, size_t qylen,
                        FAR const unsigned char *qz, size_t qzlen)
{
  struct crypt_kop cryptk;
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

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_iparams = 0;
  cryptk.crk_oparams = 4;
  cryptk.crk_param[0].crp_p = (caddr_t)d;
  cryptk.crk_param[0].crp_nbits = dlen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)qx;
  cryptk.crk_param[1].crp_nbits = qxlen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)qy;
  cryptk.crk_param[2].crp_nbits = qylen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)qz;
  cryptk.crk_param[3].crp_nbits = qzlen * 8;

  if (ioctl(cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      goto err;
    }

  close(cryptodev_fd);
  close(fd);
  return cryptk.crk_status;

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

/* Test curve SECP256R1 */

static int test_p256 (void)
{
  int ret = 0;
  int len = SECP256R1_KEYLEN;
  unsigned char *x = (unsigned char *)malloc(len);
  unsigned char *y = (unsigned char *)malloc(len);
  unsigned char *z = (unsigned char *)malloc(len);
  unsigned char *d = (unsigned char *)malloc(len);
  unsigned char *r = (unsigned char *)malloc(len);
  unsigned char *s = (unsigned char *)malloc(len);

  if (x == NULL || y == NULL || z == NULL ||
      d == NULL || r == NULL || s == NULL)
    {
      perror("Error no enough buffer");
      goto free;
    }

  memset(x, 0, len);
  memset(y, 0, len);
  memset(z, 0, len);
  memset(d, 0, len);

  ret = ecdsa_genkey(CRK_ECDSA_SECP256R1_GENKEY, x, len,
                     y, len, z, len, d, len);
  if (ret != 0)
    {
      printf("p256 generate key failed\n");
      goto free;
    }

  ret = ecdsa_sign(CRK_ECDSA_SECP256R1_SIGN, r, len, s, len,
                   d, len, sha256_message, len);
  if (ret != 0)
    {
      printf("p256 sign failed\n");
      goto free;
    }

  ret = ecdsa_verify(CRK_ECDSA_SECP256R1_VERIFY, x, len, y, len,
                     z, len, r, len, s, len, sha256_message, len);
  if (ret != 0)
    {
      printf("p256 verify failed\n");
    }

free:
  if (x)
    {
      free(x);
    }

  if (y)
    {
      free(y);
    }

  if (z)
    {
      free(z);
    }

  if (d)
    {
      free(d);
    }

  if (r)
    {
      free(r);
    }

  if (s)
    {
      free(s);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  if (test_p256() == 0)
    {
      printf("p256 test success\n");
    }

  return 0;
}
