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
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
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

/* Session key of the crc32 xform: the parameter block itself */

typedef struct crc32_params
{
  uint32_t polynomial;
  uint32_t initial;
  uint32_t xorout;
  bool reflect_in;
  bool reflect_out;
}
crc32_params;

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

/* Parameter cases: the session key carries the parameter block */

static const crc32_params g_crc32_testparams[] =
{
    /* polynomial 0xedb88320, reflected input */

    {
      0xedb88320, 0x00000000, 0x00000000, true, false,
    },

    /* CRC-32/IEEE-802.3 */

    {
      0x04c11db7, 0xffffffff, 0xffffffff, true, true,
    },

    /* polynomial 0xf4acfb13, reflected input and output */

    {
      0xf4acfb13, 0xffffffff, 0xffffffff, true, true,
    },

    /* CRC-32C */

    {
      0x1edc6f41, 0xffffffff, 0xffffffff, true, true,
    },
};

static const uint32_t g_crc32_testresults[][8] =
{
    /* polynomial 0xedb88320, reflected input */

    {
      0x00000000, 0xb2364bc0, 0x75ce58c0, 0xbb741ae0,
      0xa24cfcc0, 0x32344b80, 0x121cfd80, 0x121cfd80,
    },

    /* CRC-32/IEEE-802.3 */

    {
      0x00000000, 0xe8b7be43, 0x352441c2, 0x20159d7f,
      0x4c2750bd, 0x1fc2e6d2, 0x7ca94a72, 0x7ca94a72,
    },

    /* polynomial 0xf4acfb13, reflected input and output */

    {
      0x00000000, 0xaa7b2b08, 0xebe963c8, 0x572300c2,
      0x9bed5706, 0xadd7278e, 0x226aba31, 0x226aba31,
    },

    /* CRC-32C */

    {
      0x00000000, 0xc1d04330, 0x364b3fb7, 0x02bd79d0,
      0x9ee6ef25, 0xa245d57d, 0x477a6781, 0x477a6781,
    },
};

/* CRC-32/MPEG-2 via the three-word {polynomial, initial, xorout} key */

static const uint32_t g_crc32_mpeg2_key[3] =
{
  0x04c11db7, 0xffffffff, 0x00000000
};

static const uint32_t g_crc32_mpeg2_results[8] =
{
  0xffffffff, 0xe66c6494, 0x9b73448c, 0x4036fca8,
  0x88406c69, 0x5f8bde77, 0xe9f78669, 0xe9f78669,
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

static int syscrc32_start(FAR crypto_context *ctx, FAR const void *key,
                          int keylen)
{
  ctx->session.mac = CRYPTO_CRC32;
  ctx->session.mackey = (caddr_t) key;
  ctx->session.mackeylen = keylen;
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

static int syscrc32_case(FAR const void *key, int keylen,
                         FAR const uint32_t *result, int number)
{
  crypto_context ctx;
  uint32_t output;
  int i;

  if (syscrc32_init(&ctx) != 0)
    {
      printf("crc32 parameter case %d: init failed\n", number);
      return 1;
    }

  /* testcase 1-7: test crc32 vector */

  for (i = 0; i < (int)(sizeof(g_crc32_testcase) / sizeof(tb)) - 1; i++)
    {
      if (syscrc32_start(&ctx, key, keylen) != 0 ||
          syscrc32_update(&ctx, g_crc32_testcase[i].data,
                          g_crc32_testcase[i].datalen) != 0 ||
          syscrc32_finish(&ctx, &output) != 0)
        {
          printf("crc32 parameter case %d: io failed\n", number);
          syscrc32_free(&ctx);
          return 1;
        }

      if (output != result[i])
        {
          printf("crc32 parameter case %d test case %d failed: "
                 "%08" PRIx32 " != %08" PRIx32 "\n",
                 number, i + 1, output, result[i]);
          syscrc32_free(&ctx);
          return 1;
        }

      printf("crc32 parameter case %d test case %d success\n",
             number, i + 1);
    }

  /* testcase 8: test segmented computing capabilities in crc32 mode */

  if (syscrc32_start(&ctx, key, keylen) != 0)
    {
      printf("crc32 parameter case %d: io failed\n", number);
      syscrc32_free(&ctx);
      return 1;
    }

  for (i = 0; i < 8; i++)
    {
      if (syscrc32_update(&ctx, g_crc32_testcase[7].data,
                          g_crc32_testcase[7].datalen) != 0)
        {
          printf("crc32 parameter case %d: io failed\n", number);
          syscrc32_free(&ctx);
          return 1;
        }
    }

  if (syscrc32_finish(&ctx, &output) != 0)
    {
      printf("crc32 parameter case %d: io failed\n", number);
      syscrc32_free(&ctx);
      return 1;
    }

  if (output != result[7])
    {
      printf("crc32 parameter case %d test case 8 failed: "
             "%08" PRIx32 " != %08" PRIx32 "\n", number, output, result[7]);
      syscrc32_free(&ctx);
      return 1;
    }

  printf("crc32 parameter case %d test case 8 success\n", number);

  syscrc32_free(&ctx);
  return 0;
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

  setbuf(stdout, NULL);

  ret = syscrc32_init(&crc32_ctx);
  if (ret != 0)
    {
      printf("syscrc32 init failed\n");
    }

  /* testcase 1-7: test crc32 vector */

  for (i = 0; i < sizeof(g_crc32_testcase) / sizeof(tb) - 1; i++)
    {
      ret = syscrc32_start(&crc32_ctx, &startval, sizeof(uint32_t));
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

  ret = syscrc32_start(&crc32_ctx, &startval, sizeof(uint32_t));
  if (ret != 0)
    {
      printf("syscrc32 start failed\n");
      goto err;
    }

  for (i = 0; i < 8; i++)
    {
      ret = syscrc32_update(&crc32_ctx, g_crc32_testcase[7].data,
                            g_crc32_testcase[7].datalen);
      if (ret)
        {
          printf("syscrc32 update failed\n");
          goto err;
        }
    }

  ret = syscrc32_finish(&crc32_ctx, &startval);
  if (ret)
    {
      printf("syscrc32 finish failed\n");
      goto err;
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

  /* parameter cases: the session key carries the parameter block */

  for (i = 0; i < 4; i++)
    {
      ret |= syscrc32_case(&g_crc32_testparams[i], sizeof(crc32_params),
                           g_crc32_testresults[i], i + 1);
    }

  /* CRC-32/MPEG-2 via the three-word {polynomial, initial, xorout} key */

  ret |= syscrc32_case(g_crc32_mpeg2_key, sizeof(g_crc32_mpeg2_key),
                       g_crc32_mpeg2_results, 5);

err:
  syscrc32_free(&crc32_ctx);
  return ret;
}
