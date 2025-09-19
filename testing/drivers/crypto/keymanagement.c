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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

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
  struct crypt_kop cryptk;
  struct cryptkop  cryptkcb;
  struct pollfd fds;
  unsigned char bufcb[64];
}
crypto_context_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static crypto_context_t g_keymgmt_ctx;

static FAR char *g_aescbc_key =
  "\x06\xa9\x21\x40\x36\xb8\xa1\x5b\x51\x2e\x03\xd5\x34\x12\x00\x06";
static FAR char *g_aescbc_iv =
  "\x3d\xaf\xba\x42\x9d\x9e\xb4\x30\xb4\x22\xda\x80\x2c\x9f\xac\x41";
static FAR char *g_aescbc_plain = "Single block msg";
static FAR char *g_aescbc_cipher =
  "\xe3\x53\x77\x9c\x10\x79\xae\xb8\x27\x08\x94\x2d\xbe\x77\x18\x1a";

static unsigned char g_aescmac_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16,     0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,     0x09, 0xcf, 0x4f, 0x3c
};

static unsigned char g_aescmac_message[] =
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

static unsigned char g_aescmac_mac[] =
{
    0x51, 0xf0, 0xbe, 0xbf,     0x7e, 0x3b, 0x9d, 0x92,
    0xfc, 0x49, 0x74, 0x17,     0x79, 0x36, 0x3c, 0xfe
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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
  ctx->cryptk.crk_arg = &ctx->cryptkcb;

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
                           bool is_sync)
{
  FAR unsigned char *bufcb;

  ctx->cryptk.crk_op = op;
  ctx->cryptk.crk_iparams = paramin;
  ctx->cryptk.crk_oparams = paramout;
  ctx->cryptk.crk_param[0].crp_p = (caddr_t)keyid;
  ctx->cryptk.crk_param[0].crp_nbits = sizeof(int) * 8;
  ctx->cryptk.crk_param[1].crp_p = (caddr_t)buf;
  ctx->cryptk.crk_param[1].crp_nbits = buflen * 8;
  if (!is_sync)
    {
      ctx->cryptk.crk_flags |= CRYPTO_F_CBIMM;
      ctx->cryptkcb.krp_op = op;
      ctx->cryptkcb.krp_iparams = paramin;
      ctx->cryptkcb.krp_oparams = paramout;
      ctx->cryptkcb.krp_param[0].crp_p = (caddr_t)keyid;
      ctx->cryptkcb.krp_param[0].crp_nbits = sizeof(int) * 8;
      ctx->cryptkcb.krp_flags |= CRYPTO_F_CBIMM;
      ctx->cryptkcb.krp_param[1].crp_p = (caddr_t)ctx->bufcb;
      ctx->cryptkcb.krp_param[1].crp_nbits = buflen * 8;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &ctx->cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  if (!is_sync)
    {
      memset(&ctx->fds, 0, sizeof(struct pollfd));
      ctx->fds.fd = ctx->cryptodev_fd;
      ctx->fds.events = POLLIN;

      if (poll(&ctx->fds, 1, -1) < 0)
        {
          perror("poll");
          return -1;
        }

      if (ctx->fds.revents & POLLIN)
        {
          if (ioctl(ctx->cryptodev_fd, CIOCKEYRET, &ctx->cryptk) == -1)
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

  return ctx->cryptk.crk_status;
}

static int keymanagement_test(int mode)
{
  unsigned char data[32];
  unsigned char buf[32];
  int keyid = -1;

  arc4random_buf(data, sizeof(data));

  assert_int_equal(crypto_init(&g_keymgmt_ctx), 0);

  /* testcase 1: check keyid */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_ALLOCATE_KEY, &keyid,
                                   0, 1, NULL, 0, mode == SYNC), 0);

  /* testcase 2: import and export key */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_IMPORT_KEY, &keyid,
                                   2, 0, data, sizeof(data),
                                   mode == SYNC), 0);

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_EXPORT_KEY, &keyid,
                                   1, 1, buf, sizeof(buf), mode == SYNC), 0);

  assert_int_equal(memcmp(buf, data, sizeof(data)), 0);

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_SAVE_KEY, &keyid,
                                   1, 0, NULL, 0, mode == SYNC), 0);

  /* testcase 3: load/unload key */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_LOAD_KEY, &keyid,
                                   1, 0, NULL, 0, mode == SYNC), 0);

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_UNLOAD_KEY, &keyid,
                                   1, 0, NULL, 0, mode == SYNC), 0);

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_DELETE_KEY, &keyid,
                                   1, 0, NULL, 0, mode == SYNC), 0);

  crypto_free(&g_keymgmt_ctx);
  return 0;
}

static void test_keymgmt_sync(void **state)
{
  assert_int_equal(keymanagement_test(SYNC), 0);
}

static void test_keymgmt_async(void **state)
{
  assert_int_equal(keymanagement_test(ASYNC), 0);
}

static int aescbc_crypt(FAR crypto_context_t *ctx,
                        FAR const char *key, size_t klen,
                        FAR const char *iv, FAR const char *in,
                        FAR unsigned char *out, size_t len, int encrypt)
{
  struct session_op session;
  struct crypt_op cryp;

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CBC;
  session.key = (caddr_t)key;
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  session.flags |= SOP_F_KEYID;
  session.keylen = klen;
  assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session), 0);

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.olen = len;
  cryp.ivlen = 16;
  cryp.src = (caddr_t)in;
  cryp.dst = (caddr_t)out;
  cryp.iv = (caddr_t)iv;
  cryp.mac = 0;
  assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cryp), 0);
  assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses), 0);
  return 0;
}

static void test_keymgmt_aescbc(void **state)
{
  unsigned char buf[64];
  int keyid = -1;

  assert_int_equal(crypto_init(&g_keymgmt_ctx), 0);

  /* step 1: allocate one usable keyid */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_ALLOCATE_KEY, &keyid,
                                   0, 1, NULL, 0, TRUE), 0);

  /* step 2: import key */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_IMPORT_KEY, &keyid,
                                   2, 0, (unsigned char *)g_aescbc_key,
                                   16, TRUE), 0);

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_SAVE_KEY, &keyid,
                                   1, 0, NULL, 0, TRUE), 0);

  /* step 3: calculate out */

  assert_int_equal(aescbc_crypt(&g_keymgmt_ctx, (char *)&keyid,
                                sizeof(uint32_t), g_aescbc_iv,
                                g_aescbc_plain, buf, 16, 1), 0);

  assert_int_equal(memcmp(buf, g_aescbc_cipher, 16), 0);

  /* step 4: remove key */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_DELETE_KEY, &keyid,
                                   1, 0, NULL, 0, TRUE), 0);

  crypto_free(&g_keymgmt_ctx);
}

static int aescmac_crypt(FAR crypto_context_t *ctx,
                         FAR const unsigned char *key, size_t klen,
                         FAR const unsigned char *message, size_t len,
                         FAR unsigned char *output, size_t outlen)
{
  struct session_op session;
  struct crypt_op cryp;

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CMAC;
  session.key = (caddr_t)key;
  session.keylen = klen;
  session.mac = CRYPTO_AES_128_CMAC;
  session.mackey = (caddr_t)key;
  session.mackeylen = klen;
  session.flags |= SOP_F_KEYID;

  assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session), 0);

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;

  if (len > 0)
    {
      cryp.flags |= COP_FLAG_UPDATE;
      cryp.len = len;
      cryp.src = (caddr_t)message;
      assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cryp), 0);
    }

  cryp.flags = 0;
  cryp.len = 0;
  cryp.olen = outlen;
  cryp.src = NULL;
  cryp.mac = (caddr_t)output;

  assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cryp), 0);
  assert_int_equal(ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses), 0);
  return 0;
}

static void test_keymgmt_aescmac(void **state)
{
  unsigned char buf[64];
  int keyid = -1;

  assert_int_equal(crypto_init(&g_keymgmt_ctx), 0);

  /* step 1: allocate one usable keyid */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_ALLOCATE_KEY, &keyid,
                                   0, 1, NULL, 0, TRUE), 0);

  /* step 2: import key */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_IMPORT_KEY, &keyid,
                                   2, 0, g_aescmac_key, 16, TRUE), 0);

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_SAVE_KEY, &keyid,
                                   1, 0, NULL, 0, TRUE), 0);

  /* step 3: calculate out */

  assert_int_equal(aescmac_crypt(&g_keymgmt_ctx, (unsigned char *)&keyid,
                                 sizeof(uint32_t), g_aescmac_message,
                                 64, buf, 16), 0);

  assert_int_equal(memcmp(buf, g_aescmac_mac, 16), 0);

  /* step 4: remove key */

  assert_int_equal(keyid_operation(&g_keymgmt_ctx, CRK_DELETE_KEY, &keyid,
                                   1, 0, NULL, 0, TRUE), 0);

  crypto_free(&g_keymgmt_ctx);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest keymgmt_tests[] =
    {
      cmocka_unit_test(test_keymgmt_sync),
      cmocka_unit_test(test_keymgmt_async),
      cmocka_unit_test(test_keymgmt_aescbc),
      cmocka_unit_test(test_keymgmt_aescmac),
    };

  return cmocka_run_group_tests(keymgmt_tests, NULL, NULL);
}
