/****************************************************************************
 * apps/testing/drivers/crypto/crc32.c
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
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

typedef struct crypto_context
{
  int fd;
  int crypto_fd;
  struct session_op session;
  struct crypt_op cryp;
}
crypto_context;

typedef struct tb
{
  FAR char *data;
  int datalen;
  uint32_t result;
}
tb;

static tb g_crc32_testcase[] =
{
    /* testcase 1-7: Individual testing */

    {
      "",
      0,
      0,
    },
    {
      "a",
      1,
      0xe8b7be43,
    },
    {
      "abc",
      3,
      0x352441c2,
    },
    {
      "message digest",
      14,
      0x20159d7f,
    },
    {
      "abcdefghijklmnopqrstuvwxyz",
      26,
      0x4c2750bd,
    },
    {
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
      62,
      0x1fc2e6d2,
    },
    {
      "123456789012345678901234567890123456789"
      "01234567890123456789012345678901234567890",
      80,
      0x7ca94a72,
    },

    /* testcase 8: test case 7 is divided into 8 parts */

    {
      "1234567890",
      10,
      0x7ca94a72,
    }
};

static void syscrc32_free(FAR crypto_context *ctx)
{
  if (ctx->crypto_fd != 0)
    {
      close(ctx->crypto_fd);
      ctx->crypto_fd = 0;
    }

  if (ctx->fd != 0)
    {
      close(ctx->fd);
      ctx->fd = 0;
    }
}

static int syscrc32_init(FAR crypto_context *ctx)
{
  memset(ctx, 0, sizeof(crypto_context));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("CRIOGET");
      syscrc32_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->crypto_fd) == -1)
    {
      warn("CRIOGET");
      syscrc32_free(ctx);
      return 1;
    }

  return 0;
}

static int syscrc32_start(FAR crypto_context *ctx, uint32_t *key)
{
  ctx->session.mac = CRYPTO_CRC32;
  ctx->session.mackey = (caddr_t) key;
  ctx->session.mackeylen = sizeof(uint32_t);
  if (ioctl(ctx->crypto_fd, CIOCGSESSION, &ctx->session) == -1)
    {
      warn("CIOCGSESSION");
      syscrc32_free(ctx);
      return 1;
    }

  ctx->cryp.ses = ctx->session.ses;
  return 0;
}

static int syscrc32_update(FAR crypto_context *ctx, FAR const char *s,
                           size_t len)
{
  ctx->cryp.op = COP_ENCRYPT;
  ctx->cryp.flags |= COP_FLAG_UPDATE;
  ctx->cryp.src = (caddr_t) s;
  ctx->cryp.len = len;
  if (ioctl(ctx->crypto_fd, CIOCCRYPT, &ctx->cryp) == -1)
    {
      warn("CIOCCRYPT");
      return 1;
    }

  return 0;
}

static int syscrc32_finish(FAR crypto_context *ctx, FAR uint32_t *out)
{
  ctx->cryp.flags = 0;
  ctx->cryp.mac = (caddr_t) out;

  if (ioctl(ctx->crypto_fd, CIOCCRYPT, &ctx->cryp) == -1)
    {
      warn("CIOCCRYPT");
      return 1;
    }

  if (ioctl(ctx->crypto_fd, CIOCFSESSION, &ctx->session.ses) == -1)
    {
      warn("CIOCFSESSION");
      return 1;
    };

  return 0;
}

static int match(const uint32_t a, const uint32_t b)
{
  if (a == b)
    {
      return 0;
    }

  warnx("crc32 mismatch");

  printf("%02lx", a);
  printf("%02lx", b);
  printf("\n");

  return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  crypto_context crc32_ctx;
  uint32_t output;
  uint32_t startval = 0;
  int ret = 0;
  int i;

  ret = syscrc32_init(&crc32_ctx);
  if (ret != 0)
    {
      printf("syscrc32 init failed\n");
    }

  /* testcase 1-7: test crc32 vector */

  for (i = 0; i < sizeof(g_crc32_testcase) / sizeof(tb) - 1; i++)
    {
      ret = syscrc32_start(&crc32_ctx, &startval);
      if (ret != 0)
        {
          printf("syscrc32 start failed\n");
          goto err;
        }

      ret = syscrc32_update(&crc32_ctx, g_crc32_testcase[i].data,
                            g_crc32_testcase[i].datalen);
      if (ret)
        {
          printf("syscrc32 update failed\n");
          goto err;
        }

      ret = syscrc32_finish(&crc32_ctx, &output);
      if (ret)
        {
          printf("syscrc32 finish failed\n");
          goto err;
        }

      ret = match(g_crc32_testcase[i].result, output);
      if (ret)
        {
          printf("match crc32 test case %d failed\n", i + 1);
          goto err;
        }
      else
        {
          printf("crc32 test case %d success\n", i + 1);
        }
    }

  /* testcase 8: test segmented computing capabilities in crc32 mode */

  for (i = 0; i < 8; i++)
    {
      ret = syscrc32_start(&crc32_ctx, &startval);
      if (ret != 0)
        {
          printf("syscrc32 start failed\n");
          goto err;
        }

      ret = syscrc32_update(&crc32_ctx, g_crc32_testcase[7].data,
                            g_crc32_testcase[7].datalen);
      if (ret)
        {
          printf("syscrc32 update failed\n");
          goto err;
        }

      ret = syscrc32_finish(&crc32_ctx, &startval);
      if (ret)
        {
          printf("syscrc32 finish failed\n");
          goto err;
        }
    }

  ret = match(g_crc32_testcase[7].result, startval);
  if (ret)
    {
      printf("match crc32 testcase 8 failed\n");
      goto err;
    }
  else
    {
      printf("crc32 test case 8 success\n");
    }

err:
  syscrc32_free(&crc32_ctx);
  return 0;
}
