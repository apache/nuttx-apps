/****************************************************************************
 * apps/testing/drivers/crypto/keymanagement.c
 * Copyright (c) 2008 Damien Bergamini <damien.bergamini@free.fr>
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

typedef enum
{
  SYNC = 0,
  ASYNC,
}
crypto_mode_t;

typedef struct crypto_context_s
{
  int fd;
  int cryptodev_fd;
}
crypto_context_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const unsigned char g_sha256_message[32] =
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
  printf("Usage: %s -a/-s \n", progname);
  printf("  [-s] selects synchronous mode.\n");
  printf("  [-a] selects asynchronous mode.\n");
  exit(exitcode);
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

static int keyid_operation(FAR crypto_context_t *ctx,
                           int op, FAR int *keyid,
                           int paramin, int paramout,
                           FAR unsigned char *buf, int buflen,
                           FAR struct pollfd *fds)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = op;
  cryptk.crk_iparams = paramin;
  cryptk.crk_oparams = paramout;
  cryptk.crk_param[0].crp_p = (caddr_t)keyid;
  cryptk.crk_param[0].crp_nbits = sizeof(int) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)buf;
  cryptk.crk_param[1].crp_nbits = buflen * 8;
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

static int p256_sign(FAR crypto_context_t *ctx, FAR int *keyid,
                     FAR const unsigned char *buf, size_t buflen,
                     FAR unsigned char *r, size_t rlen,
                     FAR unsigned char *s, size_t slen,
                     FAR struct pollfd *fds)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_ECDSA_SECP256R1_SIGN;
  cryptk.crk_iparams = 2;
  cryptk.crk_oparams = 2;
  cryptk.crk_param[0].crp_p = (caddr_t)keyid;
  cryptk.crk_param[0].crp_nbits = sizeof(int) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)buf;
  cryptk.crk_param[1].crp_nbits = buflen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)r;
  cryptk.crk_param[2].crp_nbits = rlen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)s;
  cryptk.crk_param[3].crp_nbits = slen * 8;
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

static int p256_verify(FAR crypto_context_t *ctx, FAR int *keyid,
                       FAR const unsigned char *r, size_t rlen,
                       FAR const unsigned char *s, size_t slen,
                       FAR const unsigned char *buf, size_t buflen,
                       FAR struct pollfd *fds)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_ECDSA_SECP256R1_VERIFY;
  cryptk.crk_iparams = 6;
  cryptk.crk_oparams = 0;
  cryptk.crk_param[0].crp_p = (caddr_t)keyid;
  cryptk.crk_param[0].crp_nbits = sizeof(int) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)keyid;
  cryptk.crk_param[1].crp_nbits = sizeof(int) * 8;
  cryptk.crk_param[2].crp_p = NULL;
  cryptk.crk_param[2].crp_nbits = 0;
  cryptk.crk_param[3].crp_p = (caddr_t)r;
  cryptk.crk_param[3].crp_nbits = rlen * 8;
  cryptk.crk_param[4].crp_p = (caddr_t)s;
  cryptk.crk_param[4].crp_nbits = slen * 8;
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

int keymanagement_test(int mode)
{
  crypto_context_t ctx;
  struct pollfd fds;
  unsigned char buf[32];
  unsigned char r[32];
  unsigned char s[32];
  int keyid = -1;
  int p256_prv_keyid = -1;
  int p256_pub_keyid = -1;
  int ret = 0;

  ret = crypto_init(&ctx);
  if (ret != 0)
    {
      goto free;
    }

  /* testcase 1: check keyid */

  ret = keyid_operation(&ctx, CRK_ALLOCATE_KEY, &keyid, 0, 1, NULL, 0,
                        mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("keyid allocate %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = keyid_operation(&ctx, CRK_VALIDATE_KEYID, &keyid, 1, 0, NULL, 0,
                        mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("keyid validate %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  printf("keymanagement testcase 1 %s success\n",
          mode == SYNC ? "sync" : "async");

  /* testcase 2: import and export key */

  ret = keyid_operation(&ctx, CRK_IMPORT_KEY, &keyid, 2, 0,
                        g_sha256_message, sizeof(g_sha256_message),
                        mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("import key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = keyid_operation(&ctx, CRK_EXPORT_KEY, &keyid, 1, 1, buf,
                        sizeof(buf), mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("export key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  if (memcmp(buf, g_sha256_message, sizeof(g_sha256_message)) != 0)
    {
      printf("keymanagement testcase 2 %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = keyid_operation(&ctx, CRK_SAVE_KEY, &keyid, 1, 0,
                        NULL, 0, mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("save key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  printf("keymanagement testcase 2 %s success\n",
          mode == SYNC ? "sync" : "async");

  /* testcase 3: load/unload key */

  ret = keyid_operation(&ctx, CRK_LOAD_KEY, &keyid, 1, 0,
                        NULL, 0, mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("load key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = keyid_operation(&ctx, CRK_UNLOAD_KEY, &keyid, 1, 0,
                        NULL, 0, mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("unload key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  printf("keymanagement testcase 3 %s success\n",
          mode == SYNC ? "sync" : "async");

  /* testcase 4: generate p256 key */

  ret = keyid_operation(&ctx, CRK_ALLOCATE_KEY, &p256_prv_keyid, 0, 1,
                        NULL, 0, mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("allocate p256 private keyid %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = keyid_operation(&ctx, CRK_ALLOCATE_KEY, &p256_pub_keyid, 0, 1,
                        NULL, 0, mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("allocate p256 public keyid %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = keyid_operation(&ctx, CRK_GENERATE_SECP256R1_KEY, &p256_prv_keyid,
                        2, 0, (unsigned char *)&p256_pub_keyid, sizeof(int),
                        mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("generate p256 key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  printf("keymanagement testcase 4 %s success\n",
          mode == SYNC ? "sync" : "async");

  /* testcase 5: p256 test */

  ret = p256_sign(&ctx, &p256_prv_keyid, g_sha256_message,
                  sizeof(g_sha256_message), r, 32, s, 32,
                  mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("p256 sign with key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  ret = p256_verify(&ctx, &p256_pub_keyid, r, 32, s, 32,
                    g_sha256_message, sizeof(g_sha256_message),
                    mode == SYNC ? NULL : &fds);
  if (ret != 0)
    {
      printf("p256 verify with key %s failed\n",
              mode == SYNC ? "sync" : "async");
      goto free;
    }

  printf("keymanagement testcase 5 %s success\n",
          mode == SYNC ? "sync" : "async");

free:
  if (keyid != -1)
    {
      keyid_operation(&ctx, CRK_DELETE_KEY, &keyid, 1, 0, NULL, 0,
                      mode == SYNC ? NULL : &fds);
    }

  if (p256_prv_keyid != -1)
    {
      keyid_operation(&ctx, CRK_DELETE_KEY, &p256_prv_keyid, 1, 0, NULL, 0,
                      mode == SYNC ? NULL : &fds);
    }

  if (p256_pub_keyid != -1)
    {
      keyid_operation(&ctx, CRK_DELETE_KEY, &p256_pub_keyid, 1, 0, NULL, 0,
                      mode == SYNC ? NULL : &fds);
    }

  crypto_free(&ctx);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  crypto_mode_t mode = SYNC;

  if (argc == 2 && argv[1] != NULL)
    {
      if (strcmp(argv[1], "-s") == 0)
        {
          mode = SYNC;
        }
      else if (strcmp(argv[1], "-a") == 0)
        {
          mode = ASYNC;
        }
      else
        {
          show_usage(argv[0], EXIT_FAILURE);
        }
    }

  return keymanagement_test(mode);
}
