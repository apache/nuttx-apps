/****************************************************************************
 * apps/testing/drivers/crypto/crc16.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

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

/* Session key of the crc16 xform: the parameter block itself */

typedef struct crc16_params
{
  uint16_t polynomial;
  uint16_t initial;
  uint16_t xorout;
  bool reflect_in;
  bool reflect_out;
}
crc16_params;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const tb g_crc16_testcases[] =
{
    /* testcase 1-7: Individual testing */

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
      "1234567890123456789012345678901234567890"
      "1234567890123456789012345678901234567890",
      80,
    },

    /* testcase 8: test case 7 is divided into 8 parts */

    {
      "1234567890",
      10,
    },
};

static const crc16_params g_crc16_testparams[] =
{
    /* CRC-16/CCITT */

    {
      0x1021, 0x0000, 0x0000, true, true,
    },

    /* CRC-16/CCITT-FALSE */

    {
      0x1021, 0xffff, 0x0000, false, false,
    },

    /* CRC-16/IBM */

    {
      0x8005, 0x0000, 0x0000, true, true,
    },

    /* CRC-16/XMODEM */

    {
      0x1021, 0x0000, 0x0000, false, false,
    },
};

static const uint16_t g_crc16_testresults[][8] =
{
    /* CRC-16/CCITT */

    {
      0x0000, 0x728f, 0x58e9, 0x7bd5,
      0x80b0, 0xd7d5, 0x4375, 0x4375,
    },

    /* CRC-16/CCITT-FALSE */

    {
      0xffff, 0x9d77, 0x514a, 0x32cc,
      0x53e2, 0xb46b, 0xdadf, 0xdadf,
    },

    /* CRC-16/IBM */

    {
      0x0000, 0xe8c1, 0x9738, 0x3b44,
      0x9c1d, 0x57b4, 0x5ec7, 0x5ec7,
    },

    /* CRC-16/XMODEM */

    {
      0x0000, 0x7c87, 0x9dd6, 0x9ba6,
      0x63ac, 0x7db0, 0xe73a, 0xe73a,
    },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void syscrc16_free(FAR crypto_context *ctx)
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

static int syscrc16_init(FAR crypto_context *ctx)
{
  memset(ctx, 0, sizeof(crypto_context));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("CRIOGET");
      syscrc16_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->crypto_fd) == -1)
    {
      warn("CRIOGET");
      syscrc16_free(ctx);
      return 1;
    }

  return 0;
}

static int syscrc16_start(FAR crypto_context *ctx,
                          FAR const crc16_params *key)
{
  ctx->session.mac = CRYPTO_CRC16;
  ctx->session.mackey = (caddr_t)key;
  ctx->session.mackeylen = sizeof(crc16_params);
  if (ioctl(ctx->crypto_fd, CIOCGSESSION, &ctx->session) == -1)
    {
      warn("CIOCGSESSION");
      syscrc16_free(ctx);
      return 1;
    }

  ctx->cryp.ses = ctx->session.ses;
  return 0;
}

static int syscrc16_update(FAR crypto_context *ctx, FAR const char *s,
                           size_t len)
{
  ctx->cryp.op = COP_ENCRYPT;
  ctx->cryp.flags |= COP_FLAG_UPDATE;
  ctx->cryp.src = (caddr_t)s;
  ctx->cryp.len = len;
  if (ioctl(ctx->crypto_fd, CIOCCRYPT, &ctx->cryp) == -1)
    {
      warn("CIOCCRYPT");
      return 1;
    }

  return 0;
}

static int syscrc16_finish(FAR crypto_context *ctx, FAR uint16_t *out)
{
  ctx->cryp.flags = 0;
  ctx->cryp.mac = (caddr_t)out;

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

static int syscrc16_case(FAR const crc16_params *params,
                       FAR const uint16_t *result, int number)
{
  crypto_context ctx;
  uint16_t output;
  int i;

  if (syscrc16_init(&ctx) != 0)
    {
      printf("crc16 parameter case %d: init failed\n", number);
      return 1;
    }

  /* testcase 1-7: test crc16 vector */

  for (i = 0; i < (int)(sizeof(g_crc16_testcases) / sizeof(tb)) - 1; i++)
    {
      if (syscrc16_start(&ctx, params) != 0 ||
          syscrc16_update(&ctx, g_crc16_testcases[i].data,
                                g_crc16_testcases[i].datalen) != 0 ||
          syscrc16_finish(&ctx, &output) != 0)
        {
          printf("crc16 parameter case %d: io failed\n", number);
          syscrc16_free(&ctx);
          return 1;
        }

      if (output != result[i])
        {
          printf("crc16 parameter case %d test case %d failed: "
                 "%04x != %04x\n", number, i + 1, output, result[i]);
          syscrc16_free(&ctx);
          return 1;
        }

      printf("crc16 parameter case %d test case %d success\n",
             number, i + 1);
    }

  /* testcase 8: test segmented computing capabilities in crc16 mode */

  if (syscrc16_start(&ctx, params) != 0)
    {
      printf("crc16 parameter case %d: io failed\n", number);
      syscrc16_free(&ctx);
      return 1;
    }

  for (i = 0; i < 8; i++)
    {
      if (syscrc16_update(&ctx, g_crc16_testcases[7].data,
                                g_crc16_testcases[7].datalen) != 0)
        {
          printf("crc16 parameter case %d: io failed\n", number);
          syscrc16_free(&ctx);
          return 1;
        }
    }

  if (syscrc16_finish(&ctx, &output) != 0)
    {
      printf("crc16 parameter case %d: io failed\n", number);
      syscrc16_free(&ctx);
      return 1;
    }

  if (output != result[7])
    {
      printf("crc16 parameter case %d test case 8 failed: "
             "%04x != %04x\n", number, output, result[7]);
      syscrc16_free(&ctx);
      return 1;
    }

  printf("crc16 parameter case %d test case 8 success\n", number);

  syscrc16_free(&ctx);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  int fails = 0;
  int i;

  setbuf(stdout, NULL);

  for (i = 0; i < 4; i++)
    {
      fails += syscrc16_case(&g_crc16_testparams[i],
                             g_crc16_testresults[i], i + 1);
    }

  return fails;
}
