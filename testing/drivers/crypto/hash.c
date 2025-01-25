/****************************************************************************
 * apps/testing/drivers/crypto/hash.c
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
#include <crypto/md5.h>
#include <crypto/sha1.h>
#include <crypto/sha2.h>

#ifdef CONFIG_TESTING_CRYPTO_HASH_HUGE_BLOCK
#  define HASH_HUGE_BLOCK_SIZE (600 * 1024) /* 600k */
#endif

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
}
tb;

tb md5_testcase[] =
{
    {
      "",
      0,
    },
    {
      "a",
      1,
    },
    {
      "abc",
      3,
    },
    {
      "message digest",
      14,
    },
    {
      "abcdefghijklmnopqrstuvwxyz",
      26,
    },
    {
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
      62,
    },
    {
      "123456789012345678901234567890123456789"
      "01234567890123456789012345678901234567890",
      80,
    }
};

tb sha_testcase[] =
{
    {
      "abc",
      3,
    },
    {
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
      56,
    },
    {
      "",
      1000,
    }
};

tb sha512_testcase[] =
{
    {
      "abc",
      3,
    },
    {
      "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnh"
      "ijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
      112,
    },
    {
      "",
      1000,
    }
};

/* RFC 1321 test vectors */

static const unsigned char md5_result[7][16] =
{
  { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e },
  { 0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
    0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 },
  { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 },
  { 0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
    0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0 },
  { 0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
    0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b },
  { 0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
    0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f },
  { 0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
    0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a }
};

/* FIPS-180-1 test vectors */

static const unsigned char sha1_result[3][20] =
{
  { 0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
    0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d },
  { 0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e, 0xba, 0xae,
    0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1 },
  { 0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4, 0xf6, 0x1e,
    0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6f }
};

/* FIPS-180-2 test vectors */

static const unsigned char sha256_result[3][32] =
{
  { 0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad },
  { 0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
    0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
    0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
    0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1 },
  { 0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92,
    0x81, 0xa1, 0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67,
    0xf1, 0x80, 0x9a, 0x48, 0xa4, 0x97, 0x20, 0x0e,
    0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, 0x2c, 0xd0 }
};

/* FIPS-180-2 test vectors */

static const unsigned char sha512_result[3][64] =
{
  { 0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
    0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
    0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
    0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
    0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
    0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
    0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
    0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f },
  { 0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda,
    0x8c, 0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f,
    0x8f, 0x77, 0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1,
    0x72, 0x99, 0xae, 0xad, 0xb6, 0x88, 0x90, 0x18,
    0x50, 0x1d, 0x28, 0x9e, 0x49, 0x00, 0xf7, 0xe4,
    0x33, 0x1b, 0x99, 0xde, 0xc4, 0xb5, 0x43, 0x3a,
    0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd, 0x26, 0x54,
    0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9, 0x09 },
  { 0xe7, 0x18, 0x48, 0x3d, 0x0c, 0xe7, 0x69, 0x64,
    0x4e, 0x2e, 0x42, 0xc7, 0xbc, 0x15, 0xb4, 0x63,
    0x8e, 0x1f, 0x98, 0xb1, 0x3b, 0x20, 0x44, 0x28,
    0x56, 0x32, 0xa8, 0x03, 0xaf, 0xa9, 0x73, 0xeb,
    0xde, 0x0f, 0xf2, 0x44, 0x87, 0x7e, 0xa6, 0x0a,
    0x4c, 0xb0, 0x43, 0x2c, 0xe5, 0x77, 0xc3, 0x1b,
    0xeb, 0x00, 0x9c, 0x5c, 0x2c, 0x49, 0xaa, 0x2e,
    0x4e, 0xad, 0xb2, 0x17, 0xad, 0x8c, 0xc0, 0x9b }
};

#ifdef CONFIG_TESTING_CRYPTO_HASH_HUGE_BLOCK
static const unsigned char md5_huge_block_result[16] =
{
  0xef, 0x6d, 0xcc, 0xc8, 0xe1, 0xcc, 0x7f, 0x08,
  0xc2, 0x71, 0xd4, 0xc4, 0xe0, 0x13, 0xa3, 0x9b
};

static const unsigned char sha1_huge_block_result[20] =
{
  0xf2, 0x35, 0x65, 0x81, 0x79, 0x4d, 0xac, 0x20, 0x79, 0x7b,
  0xff, 0x38, 0xee, 0x2b, 0xdc, 0x44, 0x24, 0xd3, 0xf0, 0x4a
};

static const unsigned char sha256_huge_block_result[32] =
{
  0x79, 0xb1, 0xf2, 0x65, 0x7e, 0x33, 0x25, 0xff,
  0x16, 0xdb, 0x5d, 0x3c, 0x65, 0xa4, 0x7b, 0x78,
  0x0d, 0xd5, 0xa1, 0x45, 0xb5, 0x30, 0xe0, 0x91,
  0x54, 0x01, 0x40, 0x0c, 0xff, 0x35, 0x1d, 0xd3
};

static const unsigned char sha512_huge_block_result[64] =
{
  0xa4, 0x3a, 0x66, 0xe8, 0xf7, 0x59, 0x95, 0x6d,
  0x09, 0x55, 0xdd, 0xad, 0x84, 0x7c, 0xd5, 0xe7,
  0xd2, 0xbe, 0xac, 0x49, 0xa9, 0x4b, 0xa3, 0x72,
  0xe1, 0x92, 0xa0, 0x70, 0x83, 0x17, 0x85, 0x5e,
  0x90, 0x9e, 0x1d, 0x91, 0x6a, 0x93, 0xd9, 0xae,
  0xb8, 0x1a, 0x43, 0xb5, 0x51, 0x53, 0x10, 0xf1,
  0xce, 0x3a, 0xcf, 0xb6, 0x9c, 0x8b, 0x6e, 0x07,
  0x13, 0xca, 0xe1, 0x94, 0x3c, 0x41, 0x50, 0xcc
};
#endif

static void syshash_free(FAR crypto_context *ctx)
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

static int syshash_init(FAR crypto_context *ctx)
{
  memset(ctx, 0, sizeof(crypto_context));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("CRIOGET");
      syshash_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->crypto_fd) == -1)
    {
      warn("CRIOGET");
      syshash_free(ctx);
      return 1;
    }

  return 0;
}

static int syshash_start(FAR crypto_context *ctx, int mac)
{
  ctx->session.mac = mac;
  if (ioctl(ctx->crypto_fd, CIOCGSESSION, &ctx->session) == -1)
    {
      warn("CIOCGSESSION");
      syshash_free(ctx);
      return 1;
    }

  ctx->cryp.ses = ctx->session.ses;
  return 0;
}

static int syshash_update(FAR crypto_context *ctx, FAR const char *s,
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

static int syshash_finish(FAR crypto_context *ctx, FAR unsigned char *out)
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

static int match(const unsigned char *a, const unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  warnx("hash mismatch");

  for (i = 0; i < len; i++)
    {
      printf("%02x", a[i]);
    }

  printf("\n");
  for (i = 0; i < len; i++)
    {
      printf("%02x", b[i]);
    }

  printf("\n");

  return 1;
}

#ifdef CONFIG_TESTING_CRYPTO_HASH_HUGE_BLOCK
static int testing_hash_huge_block(crypto_context *ctx, int op,
                    FAR const unsigned char *block, size_t len,
                    FAR const unsigned char *result, size_t reslen)
{
  int ret = 0;
  unsigned char output[64];

  ret = syshash_start(ctx, op);
  if (ret != 0)
    {
      return ret;
    }

  ret = syshash_update(ctx, (char *)block, len);
  if (ret != 0)
    {
      return ret;
    }

  ret = syshash_finish(ctx, output);
  if (ret != 0)
    {
      return ret;
    }

  return match(result, output, reslen);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  crypto_context md5_ctx;
  crypto_context sha1_ctx;
  crypto_context sha2_256_ctx;
  crypto_context sha2_512_ctx;
  unsigned char output[64];
  unsigned char buf[1024];
  int ret = 0;
  int i;
  int j;

  ret += syshash_init(&md5_ctx);
  ret += syshash_init(&sha1_ctx);
  ret += syshash_init(&sha2_256_ctx);
  ret += syshash_init(&sha2_512_ctx);
  if (ret != 0)
    {
      printf("syshash init failed\n");
    }

  for (i = 0; i < nitems(md5_testcase); i++)
    {
      ret = syshash_start(&md5_ctx, CRYPTO_MD5);
      if (ret != 0)
        {
          printf("syshash md5 start failed\n");
          goto err;
        }

      ret = syshash_update(&md5_ctx, md5_testcase[i].data,
                           md5_testcase[i].datalen);
      if (ret)
        {
          printf("syshash md5 update failed\n");
          goto err;
        }

      ret = syshash_finish(&md5_ctx, output);
      if (ret)
        {
          printf("syshash md5 finish failed\n");
          goto err;
        }

      ret = match(md5_result[i], output, MD5_DIGEST_LENGTH);
      if (ret)
        {
          printf("match md5 failed\n");
          goto err;
        }
      else
        {
          printf("hash md5 success\n");
        }
    }

  for (i = 0; i < nitems(sha_testcase); i++)
    {
      ret = syshash_start(&sha1_ctx, CRYPTO_SHA1);
      if (ret != 0)
        {
          printf("syshash sha1 start failed\n");
          goto err;
        }

      if (i == 2)
        {
          memset(buf, 'a', sha_testcase[i].datalen);
          for (j = 0; j < 1000; j++)
            {
              ret = syshash_update(&sha1_ctx, (char *)buf,
                                   sha_testcase[i].datalen);
              if (ret)
                {
                  break;
                }
            }
        }
      else
        {
          ret = syshash_update(&sha1_ctx, sha_testcase[i].data,
                               sha_testcase[i].datalen);
        }

      if (ret)
        {
          printf("sha1 update failed\n");
          goto err;
        }

      ret = syshash_finish(&sha1_ctx, output);
      if (ret)
        {
          printf("sha1 finish failed\n");
          goto err;
        }

      ret = match((unsigned char *)sha1_result[i],
                  (unsigned char *)output,
                  SHA1_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha1 failed\n");
          goto err;
        }
      else
        {
          printf("hash sha1 success\n");
        }
    }

  for (i = 0; i < nitems(sha_testcase); i++)
    {
      ret = syshash_start(&sha2_256_ctx, CRYPTO_SHA2_256);
      if (ret != 0)
        {
          printf("sha256 start failed\n");
          goto err;
        }

      if (i == 2)
        {
          memset(buf, 'a', sha_testcase[i].datalen);
          for (j = 0; j < 1000; j++)
            {
              ret = syshash_update(&sha2_256_ctx, (char *)buf,
                                   sha_testcase[i].datalen);
              if (ret)
                {
                  break;
                }
            }
        }
      else
        {
          ret = syshash_update(&sha2_256_ctx, sha_testcase[i].data,
                               sha_testcase[i].datalen);
        }

      if (ret)
        {
          printf("sha256 update failed\n");
          goto err;
        }

      ret = syshash_finish(&sha2_256_ctx, output);
      if (ret)
        {
          printf("sha256 finish failed\n");
        }

      ret = match((unsigned char *)sha256_result[i],
                  (unsigned char *)output,
                  SHA256_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha256 failed\n");
        }
      else
        {
          printf("hash sha256 success\n");
        }
    }

  for (i = 0; i < nitems(sha512_testcase); i++)
    {
      ret = syshash_start(&sha2_512_ctx, CRYPTO_SHA2_512);
      if (ret != 0)
        {
          printf("sha512 start failed\n");
          goto err;
        }

      if (i == 2)
        {
          memset(buf, 'a', sha512_testcase[i].datalen);
          for (j = 0; j < 1000; j++)
            {
              ret = syshash_update(&sha2_512_ctx, (char *)buf,
                                   sha512_testcase[i].datalen);
              if (ret)
                {
                  break;
                }
            }
        }
      else
        {
          ret = syshash_update(&sha2_512_ctx, sha512_testcase[i].data,
                               sha512_testcase[i].datalen);
        }

      if (ret)
        {
          printf("sha512 update failed\n");
          goto err;
        }

      ret = syshash_finish(&sha2_512_ctx, output);
      if (ret)
        {
          printf("sha512 finish failed\n");
          goto err;
        }

      ret = match((unsigned char *)sha512_result[i],
                  (unsigned char *)output,
                  SHA512_DIGEST_LENGTH);
      if (ret)
        {
          printf("match sha512 failed\n");
          goto err;
        }
      else
        {
          printf("hash sha512 success\n");
        }
    }

#ifdef CONFIG_TESTING_CRYPTO_HASH_HUGE_BLOCK
  unsigned char *huge_block;
  huge_block = (unsigned char *)malloc(HASH_HUGE_BLOCK_SIZE);
  if (huge_block == NULL)
    {
      printf("huge block test no memory\n");
      goto err;
    }

  memset(huge_block, 'a', HASH_HUGE_BLOCK_SIZE);
  ret = testing_hash_huge_block(&md5_ctx, CRYPTO_MD5,
                                huge_block, HASH_HUGE_BLOCK_SIZE,
                                md5_huge_block_result,
                                MD5_DIGEST_LENGTH);
  if (ret != 0)
    {
      printf("md5 huge block test failed\n");
    }
  else
    {
      printf("md5 huge block test success\n");
    }

  ret = testing_hash_huge_block(&sha1_ctx, CRYPTO_SHA1,
                                huge_block, HASH_HUGE_BLOCK_SIZE,
                                sha1_huge_block_result,
                                SHA1_DIGEST_LENGTH);
  if (ret != 0)
    {
      printf("sha1 huge block test failed\n");
    }
  else
    {
      printf("sha1 huge block test success\n");
    }

  ret = testing_hash_huge_block(&sha2_256_ctx, CRYPTO_SHA2_256,
                                huge_block, HASH_HUGE_BLOCK_SIZE,
                                sha256_huge_block_result,
                                SHA256_DIGEST_LENGTH);
  if (ret != 0)
    {
      printf("sha256 huge block test failed\n");
    }
  else
    {
      printf("sha256 huge block test success\n");
    }

  ret = testing_hash_huge_block(&sha2_512_ctx, CRYPTO_SHA2_512,
                                huge_block, HASH_HUGE_BLOCK_SIZE,
                                sha512_huge_block_result,
                                SHA512_DIGEST_LENGTH);
  if (ret != 0)
    {
      printf("sha512 huge block test failed\n");
    }
  else
    {
      printf("sha512 huge block test success\n");
    }

  free(huge_block);
#endif

err:
  syshash_free(&md5_ctx);
  syshash_free(&sha1_ctx);
  syshash_free(&sha2_256_ctx);
  syshash_free(&sha2_512_ctx);
  return 0;
}
