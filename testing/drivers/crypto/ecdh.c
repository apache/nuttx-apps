/****************************************************************************
 * apps/testing/drivers/crypto/ecdh.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache Software Foundation (ASF) under one or more
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
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct cryptodev_context
{
  int fd;
  int cryptodev_fd;
}
cryptodev_context;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int match(FAR const unsigned char *a, FAR const unsigned char *b,
                 size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  warnx("ecdh mismatch");

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

static void cryptodev_free(FAR cryptodev_context *ctx)
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

static int cryptodev_init(FAR cryptodev_context *ctx)
{
  ctx->fd = 0;
  ctx->cryptodev_fd = 0;

  ctx->fd = open("/dev/crypto", O_RDWR, 0);
  if (ctx->fd < 0)
    {
      perror("cryptodev_init open /dev/crypto failed");
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->cryptodev_fd) == -1)
    {
      perror("cryptodev_init CRIOGET failed");
      close(ctx->fd);
      return 1;
    }

  return 0;
}

/****************************************************************************
 * Name: test_ecdh_compute_key
 ****************************************************************************/

/* ECDH over SECP256R1 through CRK_ECDH_GEN_PUBLIC and
 * CRK_ECDH_COMPUTE_SHARED.  The integers cross the cryptodev boundary
 * little-endian.
 */

#define ECDH_SIZE 32

/* Fixed SECP256R1 vector, generated independently with Python and
 * cross-checked with the cryptography package:
 *
 *   dA   = b82785170e4b4947fe61a51c4dec4cf3298c2648b3266b63bbb8951e320e439e
 *   dB   = b610f27deceaa3a98ab537bf6261c7b21c406c02294d76b0fb6b4f9687e4e41f
 *   QA.x = ab800151c970b4a313006198fc39034ebaa0ab4b1c4b259bc3947b096a9396bc
 *   QA.y = 6f21465ee2963e14d7e03a9c353ee0697d75f82dcef50f2f45a5aca312847b85
 *   QB.x = eb70f7f40c06af48981d49e6bac592123fad983520c3f3b3a3642ab8ddca12de
 *   QB.y = 40ad2bff4f3a55a93c1e640740aa4c810b4e3f7fa311432790e1c1d80448481f
 *   Z    = 13ac3690538790d009b640046160d1d483b56615a3c110adac31f36b0f92e090
 *
 * The arrays below hold the little-endian byte order of the ioctl ABI.
 */

static const unsigned char g_ecdh_da[ECDH_SIZE] =
{
  0x9e, 0x43, 0x0e, 0x32, 0x1e, 0x95, 0xb8, 0xbb,
  0x63, 0x6b, 0x26, 0xb3, 0x48, 0x26, 0x8c, 0x29,
  0xf3, 0x4c, 0xec, 0x4d, 0x1c, 0xa5, 0x61, 0xfe,
  0x47, 0x49, 0x4b, 0x0e, 0x17, 0x85, 0x27, 0xb8
};

static const unsigned char g_ecdh_db[ECDH_SIZE] =
{
  0x1f, 0xe4, 0xe4, 0x87, 0x96, 0x4f, 0x6b, 0xfb,
  0xb0, 0x76, 0x4d, 0x29, 0x02, 0x6c, 0x40, 0x1c,
  0xb2, 0xc7, 0x61, 0x62, 0xbf, 0x37, 0xb5, 0x8a,
  0xa9, 0xa3, 0xea, 0xec, 0x7d, 0xf2, 0x10, 0xb6
};

static const unsigned char g_ecdh_qa_x[ECDH_SIZE] =
{
  0xbc, 0x96, 0x93, 0x6a, 0x09, 0x7b, 0x94, 0xc3,
  0x9b, 0x25, 0x4b, 0x1c, 0x4b, 0xab, 0xa0, 0xba,
  0x4e, 0x03, 0x39, 0xfc, 0x98, 0x61, 0x00, 0x13,
  0xa3, 0xb4, 0x70, 0xc9, 0x51, 0x01, 0x80, 0xab
};

static const unsigned char g_ecdh_qa_y[ECDH_SIZE] =
{
  0x85, 0x7b, 0x84, 0x12, 0xa3, 0xac, 0xa5, 0x45,
  0x2f, 0x0f, 0xf5, 0xce, 0x2d, 0xf8, 0x75, 0x7d,
  0x69, 0xe0, 0x3e, 0x35, 0x9c, 0x3a, 0xe0, 0xd7,
  0x14, 0x3e, 0x96, 0xe2, 0x5e, 0x46, 0x21, 0x6f
};

static const unsigned char g_ecdh_qb_x[ECDH_SIZE] =
{
  0xde, 0x12, 0xca, 0xdd, 0xb8, 0x2a, 0x64, 0xa3,
  0xb3, 0xf3, 0xc3, 0x20, 0x35, 0x98, 0xad, 0x3f,
  0x12, 0x92, 0xc5, 0xba, 0xe6, 0x49, 0x1d, 0x98,
  0x48, 0xaf, 0x06, 0x0c, 0xf4, 0xf7, 0x70, 0xeb
};

static const unsigned char g_ecdh_qb_y[ECDH_SIZE] =
{
  0x1f, 0x48, 0x48, 0x04, 0xd8, 0xc1, 0xe1, 0x90,
  0x27, 0x43, 0x11, 0xa3, 0x7f, 0x3f, 0x4e, 0x0b,
  0x81, 0x4c, 0xaa, 0x40, 0x07, 0x64, 0x1e, 0x3c,
  0xa9, 0x55, 0x3a, 0x4f, 0xff, 0x2b, 0xad, 0x40
};

static const unsigned char g_ecdh_z[ECDH_SIZE] =
{
  0x90, 0xe0, 0x92, 0x0f, 0x6b, 0xf3, 0x31, 0xac,
  0xad, 0x10, 0xc1, 0xa3, 0x15, 0x66, 0xb5, 0x83,
  0xd4, 0xd1, 0x60, 0x61, 0x04, 0x40, 0xb6, 0x09,
  0xd0, 0x90, 0x87, 0x53, 0x90, 0x36, 0xac, 0x13
};

static int ecdh_gen_public(FAR cryptodev_context *ctx,
                           FAR unsigned char *d, FAR unsigned char *x,
                           FAR unsigned char *y, FAR unsigned char *z)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = CRK_ECDH_GEN_PUBLIC;
  cryptk.crk_iparams = 0;
  cryptk.crk_oparams = 4;

  /* outputs: d x y z, z unused */

  cryptk.crk_param[0].crp_p = (caddr_t)d;
  cryptk.crk_param[0].crp_nbits = ECDH_SIZE * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)x;
  cryptk.crk_param[1].crp_nbits = ECDH_SIZE * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)y;
  cryptk.crk_param[2].crp_nbits = ECDH_SIZE * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)z;
  cryptk.crk_param[3].crp_nbits = ECDH_SIZE * 8;
  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("ecdh_gen_public failed");
      return 1;
    }

  return cryptk.crk_status;
}

static int ecdh_compute_shared(FAR cryptodev_context *ctx,
                               FAR const unsigned char *privkey,
                               FAR const unsigned char *peer_x,
                               FAR const unsigned char *peer_y,
                               FAR unsigned char *k)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = CRK_ECDH_COMPUTE_SHARED;
  cryptk.crk_iparams = 3;
  cryptk.crk_oparams = 1;

  /* inputs: priv_key pub_x pub_y; output: share key */

  cryptk.crk_param[0].crp_p = (caddr_t)privkey;
  cryptk.crk_param[0].crp_nbits = ECDH_SIZE * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)peer_x;
  cryptk.crk_param[1].crp_nbits = ECDH_SIZE * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)peer_y;
  cryptk.crk_param[2].crp_nbits = ECDH_SIZE * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)k;
  cryptk.crk_param[3].crp_nbits = ECDH_SIZE * 8;
  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("ecdh_compute_shared failed");
      return 1;
    }

  return cryptk.crk_status;
}

static int test_ecdh_compute_key(void)
{
  unsigned char d1[ECDH_SIZE];
  unsigned char x1[ECDH_SIZE];
  unsigned char y1[ECDH_SIZE];
  unsigned char z1[ECDH_SIZE];
  unsigned char d2[ECDH_SIZE];
  unsigned char x2[ECDH_SIZE];
  unsigned char y2[ECDH_SIZE];
  unsigned char z2[ECDH_SIZE];
  unsigned char k1[ECDH_SIZE];
  unsigned char k2[ECDH_SIZE];
  cryptodev_context ctx;
  int ret;

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      goto free;
    }

  /* Fixed vector: both sides must derive the known shared secret */

  ret = ecdh_compute_shared(&ctx, g_ecdh_da, g_ecdh_qb_x, g_ecdh_qb_y, k1);
  if (ret != 0)
    {
      printf("ECDH compute Alice's share key failed\n");
      goto free;
    }

  ret = ecdh_compute_shared(&ctx, g_ecdh_db, g_ecdh_qa_x, g_ecdh_qa_y, k2);
  if (ret != 0)
    {
      printf("ECDH compute Bob's share key failed\n");
      goto free;
    }

  ret = match(k1, g_ecdh_z, ECDH_SIZE);
  if (ret != 0)
    {
      printf("ECDH vector mismatch\n");
      goto free;
    }

  ret = match(k2, g_ecdh_z, ECDH_SIZE);
  if (ret != 0)
    {
      printf("ECDH vector mismatch\n");
      goto free;
    }

  /* Ephemeral keypairs: both sides must agree on the shared secret */

  memset(d1, 0, ECDH_SIZE);
  memset(x1, 0, ECDH_SIZE);
  memset(y1, 0, ECDH_SIZE);
  memset(z1, 0, ECDH_SIZE);
  memset(d2, 0, ECDH_SIZE);
  memset(x2, 0, ECDH_SIZE);
  memset(y2, 0, ECDH_SIZE);
  memset(z2, 0, ECDH_SIZE);

  ret = ecdh_gen_public(&ctx, d1, x1, y1, z1);
  if (ret != 0)
    {
      printf("ECDH make Alice's public key failed\n");
      goto free;
    }

  ret = ecdh_gen_public(&ctx, d2, x2, y2, z2);
  if (ret != 0)
    {
      printf("ECDH make Bob's public failed\n");
      goto free;
    }

  ret = ecdh_compute_shared(&ctx, d1, x2, y2, k1);
  if (ret != 0)
    {
      printf("ECDH compute Alice's share key failed\n");
      goto free;
    }

  ret = ecdh_compute_shared(&ctx, d2, x1, y1, k2);
  if (ret != 0)
    {
      printf("ECDH compute Bob's share key failed\n");
      goto free;
    }

  ret = match(k1, k2, ECDH_SIZE);
  if (ret != 0)
    {
      printf("ECDH mismatch share key\n");
      goto free;
    }

free:
  cryptodev_free(&ctx);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  int ret;

  setbuf(stdout, NULL);

  ret = test_ecdh_compute_key();
  if (ret != 0)
    {
      printf("test ecdh compute key failed\n");
    }
  else
    {
      printf("test ecdh compute key ok\n");
    }

  return ret;
}
