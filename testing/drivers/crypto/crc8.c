/****************************************************************************
 * apps/testing/drivers/crypto/crc8.c
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
 *
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

/* Session key of the crc8 xform: the parameter block itself */

typedef struct crc8_params
{
  uint8_t polynomial;
  uint8_t initial;
  uint8_t xorout;
  bool reflect_in;
  bool reflect_out;
}
crc8_params;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const tb g_crc8_testcases[] =
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

static const crc8_params g_crc8_testparams[] =
{
    /* polynomial 0xa6, reflected input */

    {
      0xa6, 0x00, 0x00, true, false,
    },

    /* polynomial 0x07, reflected input */

    {
      0x07, 0x00, 0x00, true, false,
    },

    /* CRC-8/SAE J1850 */

    {
      0x1d, 0xff, 0xff, false, false,
    },

    /* CRC-8/AUTOSAR */

    {
      0x2f, 0xff, 0xff, false, false,
    },

    /* CRC-8/ROHC */

    {
      0x07, 0xff, 0x00, true, true,
    },
};

static const uint8_t g_crc8_testresults[][8] =
{
    /* polynomial 0xa6, reflected input */

    {
      0x00, 0xa4, 0xa6, 0x86,
      0x68, 0x96, 0x50, 0x50,
    },

    /* polynomial 0x07, reflected input */

    {
      0x00, 0x9b, 0x0f, 0x0b,
      0xc6, 0x0e, 0x69, 0x69,
    },

    /* CRC-8/SAE J1850 */

    {
      0x00, 0xb2, 0x9a, 0x2d,
      0xd2, 0x1f, 0x92, 0x92,
    },

    /* CRC-8/AUTOSAR */

    {
      0x00, 0x07, 0x41, 0xc1,
      0xb4, 0x20, 0x0c, 0x0c,
    },

    /* CRC-8/ROHC */

    {
      0xff, 0x16, 0x24, 0xdc,
      0x39, 0x91, 0x2b, 0x2b,
    },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void syscrc8_free(FAR crypto_context *ctx)
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

static int syscrc8_init(FAR crypto_context *ctx)
{
  memset(ctx, 0, sizeof(crypto_context));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("CRIOGET");
      syscrc8_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->crypto_fd) == -1)
    {
      warn("CRIOGET");
      syscrc8_free(ctx);
      return 1;
    }

  return 0;
}

static int syscrc8_start(FAR crypto_context *ctx,
                         FAR const crc8_params *key)
{
  ctx->session.mac = CRYPTO_CRC8;
  ctx->session.mackey = (caddr_t)key;
  ctx->session.mackeylen = sizeof(crc8_params);
  if (ioctl(ctx->crypto_fd, CIOCGSESSION, &ctx->session) == -1)
    {
      warn("CIOCGSESSION");
      syscrc8_free(ctx);
      return 1;
    }

  ctx->cryp.ses = ctx->session.ses;
  return 0;
}

static int syscrc8_update(FAR crypto_context *ctx, FAR const char *s,
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

static int syscrc8_finish(FAR crypto_context *ctx, FAR uint8_t *out)
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

static int syscrc8_case(FAR const crc8_params *params,
                       FAR const uint8_t *result, int number)
{
  crypto_context ctx;
  uint8_t output;
  int i;

  if (syscrc8_init(&ctx) != 0)
    {
      printf("crc8 parameter case %d: init failed\n", number);
      return 1;
    }

  /* testcase 1-7: test crc8 vector */

  for (i = 0; i < (int)(sizeof(g_crc8_testcases) / sizeof(tb)) - 1; i++)
    {
      if (syscrc8_start(&ctx, params) != 0 ||
          syscrc8_update(&ctx, g_crc8_testcases[i].data,
                               g_crc8_testcases[i].datalen) != 0 ||
          syscrc8_finish(&ctx, &output) != 0)
        {
          printf("crc8 parameter case %d: io failed\n", number);
          syscrc8_free(&ctx);
          return 1;
        }

      if (output != result[i])
        {
          printf("crc8 parameter case %d test case %d failed: "
                 "%02x != %02x\n", number, i + 1, output, result[i]);
          syscrc8_free(&ctx);
          return 1;
        }

      printf("crc8 parameter case %d test case %d success\n",
             number, i + 1);
    }

  /* testcase 8: test segmented computing capabilities in crc8 mode */

  if (syscrc8_start(&ctx, params) != 0)
    {
      printf("crc8 parameter case %d: io failed\n", number);
      syscrc8_free(&ctx);
      return 1;
    }

  for (i = 0; i < 8; i++)
    {
      if (syscrc8_update(&ctx, g_crc8_testcases[7].data,
                               g_crc8_testcases[7].datalen) != 0)
        {
          printf("crc8 parameter case %d: io failed\n", number);
          syscrc8_free(&ctx);
          return 1;
        }
    }

  if (syscrc8_finish(&ctx, &output) != 0)
    {
      printf("crc8 parameter case %d: io failed\n", number);
      syscrc8_free(&ctx);
      return 1;
    }

  if (output != result[7])
    {
      printf("crc8 parameter case %d test case 8 failed: "
             "%02x != %02x\n", number, output, result[7]);
      syscrc8_free(&ctx);
      return 1;
    }

  printf("crc8 parameter case %d test case 8 success\n", number);

  syscrc8_free(&ctx);
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

  for (i = 0; i < 5; i++)
    {
      fails += syscrc8_case(&g_crc8_testparams[i],
                            g_crc8_testresults[i], i + 1);
    }

  return fails;
}
