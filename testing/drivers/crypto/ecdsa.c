/****************************************************************************
 * apps/testing/drivers/crypto/ecdsa.c
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

#include <fcntl.h>
#include <poll.h>
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

#define ECC_KEYLEN_MAX   64
#define SECP256R1_KEYLEN 32

typedef enum
{
  SYNC = 0,
  ASYNC,
}
crypto_mode_t;

typedef struct ecdsa_data_s
{
  unsigned char x[ECC_KEYLEN_MAX];
  unsigned char y[ECC_KEYLEN_MAX];
  unsigned char z[ECC_KEYLEN_MAX];
  unsigned char d[ECC_KEYLEN_MAX];
  unsigned char r[ECC_KEYLEN_MAX];
  unsigned char s[ECC_KEYLEN_MAX];
  size_t xlen;
  size_t ylen;
  size_t zlen;
  size_t dlen;
  size_t rlen;
  size_t slen;
}
ecdsa_data_t;

typedef struct crypto_context_s
{
  int fd;
  int cryptodev_fd;
}
crypto_context_t;

typedef struct ecdsa_test_s
{
  const char *name;
  int (*function)(int);
}
ecdsa_test_t;

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

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s -a/-s <curve> \n", progname);
  printf("  [-s] selects synchronous mode.\n");
  printf("  [-a] selects asynchronous mode.\n");
  printf("  [curve] selects the support curve.\n"
         "  support: SECP256R1\n");
  exit(exitcode);
}

static int ecdsa_data_init(FAR ecdsa_data_t *data,
                           size_t xlen, size_t ylen, size_t zlen,
                           size_t dlen, size_t rlen, size_t slen)
{
  memset(data, 0, sizeof(ecdsa_data_t));
  data->xlen = xlen;
  data->ylen = ylen;
  data->zlen = zlen;
  data->dlen = dlen;
  data->rlen = rlen;
  data->slen = slen;
  return 0;
}

static void crypto_free(FAR crypto_context_t *ctx)
{
  if (ctx->cryptodev_fd != 0)
    {
      close(ctx->cryptodev_fd);
      ctx->cryptodev_fd = 0;
    }

  if (ctx->fd != 0)
    {
      close(ctx->fd);
      ctx->fd = 0;
    }
}

static int crypto_init(FAR crypto_context_t *ctx)
{
  memset(ctx, 0, sizeof(crypto_context_t));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      perror("CRIOGET");
      crypto_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->cryptodev_fd) == -1)
    {
      perror("CRIOGET");
      crypto_free(ctx);
      return 1;
    }

  return 0;
}

static int ecdsa_sign(int op, FAR struct pollfd *fds,
                      FAR crypto_context_t *ctx, FAR ecdsa_data_t *data,
                      FAR const unsigned char *buf, size_t buflen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_iparams = 2;
  cryptk.crk_oparams = 2;
  cryptk.crk_param[0].crp_p = (caddr_t)data->d;
  cryptk.crk_param[0].crp_nbits = data->dlen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)buf;
  cryptk.crk_param[1].crp_nbits = buflen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)data->r;
  cryptk.crk_param[2].crp_nbits = data->rlen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)data->s;
  cryptk.crk_param[3].crp_nbits = data->slen * 8;
  if (fds != NULL)
    {
      cryptk.crk_flags |= CRYPTO_F_CBIMM;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  if (fds != NULL)
    {
      memset(fds, 0, sizeof(struct pollfd));
      fds->fd = ctx->cryptodev_fd;
      fds->events = POLLIN;

      if (poll(fds, 1, -1) < 0)
        {
          perror("poll");
          return -1;
        }

      if (fds->revents & POLLIN)
        {
          if (ioctl(ctx->cryptodev_fd, CIOCKEYRET, &cryptk) == -1)
            {
              perror("CIOCKEYRET");
              return -1;
            }
        }
      else
        {
          perror("poll back");
          return -1;
        }
    }

  return cryptk.crk_status;
}

static int ecdsa_verify(int op, FAR struct pollfd *fds,
                        FAR crypto_context_t *ctx, FAR ecdsa_data_t *data,
                        FAR const unsigned char *buf, size_t buflen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_iparams = 6;
  cryptk.crk_oparams = 0;
  cryptk.crk_param[0].crp_p = (caddr_t)data->x;
  cryptk.crk_param[0].crp_nbits = data->xlen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)data->y;
  cryptk.crk_param[1].crp_nbits = data->ylen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)data->z;
  cryptk.crk_param[2].crp_nbits = data->zlen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)data->r;
  cryptk.crk_param[3].crp_nbits = data->rlen * 8;
  cryptk.crk_param[4].crp_p = (caddr_t)data->s;
  cryptk.crk_param[4].crp_nbits = data->slen * 8;
  cryptk.crk_param[5].crp_p = (caddr_t)buf;
  cryptk.crk_param[5].crp_nbits = buflen * 8;
  if (fds != NULL)
    {
      cryptk.crk_flags |= CRYPTO_F_CBIMM;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  if (fds != NULL)
    {
      memset(fds, 0, sizeof(struct pollfd));
      fds->fd = ctx->cryptodev_fd;
      fds->events = POLLIN;

      if (poll(fds, 1, -1) < 0)
        {
          perror("poll");
          return -1;
        }

      if (fds->revents & POLLIN)
        {
          if (ioctl(ctx->cryptodev_fd, CIOCKEYRET, &cryptk) == -1)
            {
              perror("CIOCKEYRET");
              return -1;
            }
        }
      else
        {
          perror("poll back");
          return -1;
        }
    }

  return cryptk.crk_status;
}

static int ecdsa_genkey(int op, FAR struct pollfd *fds,
                        FAR crypto_context_t *ctx, FAR ecdsa_data_t *data)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_iparams = 0;
  cryptk.crk_oparams = 4;
  cryptk.crk_param[0].crp_p = (caddr_t)data->d;
  cryptk.crk_param[0].crp_nbits = data->dlen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)data->x;
  cryptk.crk_param[1].crp_nbits = data->xlen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)data->y;
  cryptk.crk_param[2].crp_nbits = data->ylen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)data->z;
  cryptk.crk_param[3].crp_nbits = data->zlen * 8;
  if (fds != NULL)
    {
      cryptk.crk_flags |= CRYPTO_F_CBIMM;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  if (fds != NULL)
    {
      memset(fds, 0, sizeof(struct pollfd));
      fds->fd = ctx->cryptodev_fd;
      fds->events = POLLIN;

      if (poll(fds, 1, -1) < 0)
        {
          perror("poll");
          return -1;
        }

      if (fds->revents & POLLIN)
        {
          if (ioctl(ctx->cryptodev_fd, CIOCKEYRET, &cryptk) == -1)
            {
              perror("CIOCKEYRET");
              return -1;
            }
        }
      else
        {
          perror("poll back");
          return -1;
        }
    }

  return cryptk.crk_status;
}

/* Test curve SECP256R1 */

static int test_p256(int mode)
{
  crypto_context_t ctx;
  ecdsa_data_t data;
  struct pollfd fds;
  int ret = 0;
  int len = SECP256R1_KEYLEN;

  ret = crypto_init(&ctx);
  if (ret != 0)
    {
      goto free;
    }

  ret = ecdsa_data_init(&data, len, len, len, len, len, len);
  if (ret != 0)
    {
      goto free;
    }

  ret = ecdsa_genkey(CRK_ECDSA_SECP256R1_GENKEY, mode == SYNC ? NULL : &fds,
                     &ctx, &data);
  if (ret != 0)
    {
      printf("p256 generate key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }
  else
    {
      printf("p256 generate key %s success\n",
              mode == SYNC ? "sync" : "async");
    }

  ret = ecdsa_sign(CRK_ECDSA_SECP256R1_SIGN, mode == SYNC ? NULL : &fds,
                   &ctx, &data, sha256_message, len);
  if (ret != 0)
    {
      printf("p256 sign %s failed\n", mode == SYNC ? "sync" : "async");
      goto free;
    }
  else
    {
      printf("p256 sign %s success\n", mode == SYNC ? "sync" : "async");
    }

  ret = ecdsa_verify(CRK_ECDSA_SECP256R1_VERIFY, mode == SYNC ? NULL : &fds,
                     &ctx, &data, sha256_message, len);
  if (ret != 0)
    {
      printf("p256 verify %s failed\n", mode == SYNC ? "sync" : "async");
      goto free;
    }
  else
    {
      printf("p256 verify %s success\n", mode == SYNC ? "sync" : "async");
    }

free:
  crypto_free(&ctx);
  return ret;
}

static const ecdsa_test_t g_ecdsa_test[] =
{
    {
      "SECP256R1", test_p256,
    },

  /* end test case */

    {
      NULL, NULL,
    },
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char **argp;
  crypto_mode_t mode = SYNC;
  const ecdsa_test_t *test;

  for (argp = argv + (argc >= 1 ? 1 : argc); *argp != NULL; ++argp)
    {
      if (strcmp(*argp, "-s") == 0)
        {
          mode = SYNC;
        }
      else if (strcmp(*argp, "-a") == 0)
        {
          mode = ASYNC;
        }
      else if (strcmp(*argp, "-h") == 0)
        {
          show_usage(argv[0], EXIT_FAILURE);
        }
      else
        {
          break;
        }
    }

  if (*argp != NULL)
    {
      for (; *argp != NULL; argp++)
        {
          for (test = g_ecdsa_test; test->name != NULL; test++)
            {
              if (strcmp(*argp, test->name) == 0)
                {
                  if (test->function(mode) != 0)
                    {
                      printf("Test %s case failed\n\n", *argp);
                    }
                  else
                    {
                      printf("Test %s case success\n\n", *argp);
                    }

                  break;
                }
            }
        }
    }
  else
    {
      for (test = g_ecdsa_test; test->name != NULL; test++)
        {
          if (test->function(mode) != 0)
            {
              printf("Test %s case failed\n\n", test->name);
              break;
            }
          else
            {
              printf("Test %s case success\n\n", test->name);
            }
        }
    }

  return 0;
}
