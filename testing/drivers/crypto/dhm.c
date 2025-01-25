/****************************************************************************
 * apps/testing/drivers/crypto/dhm.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <err.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RFC 5114 section 2.1:
 * 1024-bit MODP Group with 160-bit Prime Order Subgroup
 */

#define DHM_RFC5114_MODP_1024_P_BIN {                   \
        0xB1, 0x0B, 0x8F, 0x96, 0xA0, 0x80, 0xE0, 0x1D, \
        0xDE, 0x92, 0xDE, 0x5E, 0xAE, 0x5D, 0x54, 0xEC, \
        0x52, 0xC9, 0x9F, 0xBC, 0xFB, 0x06, 0xA3, 0xC6, \
        0x9A, 0x6A, 0x9D, 0xCA, 0x52, 0xD2, 0x3B, 0x61, \
        0x60, 0x73, 0xE2, 0x86, 0x75, 0xA2, 0x3D, 0x18, \
        0x98, 0x38, 0xEF, 0x1E, 0x2E, 0xE6, 0x52, 0xC0, \
        0x13, 0xEC, 0xB4, 0xAE, 0xA9, 0x06, 0x11, 0x23, \
        0x24, 0x97, 0x5C, 0x3C, 0xD4, 0x9B, 0x83, 0xBF, \
        0xAC, 0xCB, 0xDD, 0x7D, 0x90, 0xC4, 0xBD, 0x70, \
        0x98, 0x48, 0x8E, 0x9C, 0x21, 0x9A, 0x73, 0x72, \
        0x4E, 0xFF, 0xD6, 0xFA, 0xE5, 0x64, 0x47, 0x38, \
        0xFA, 0xA3, 0x1A, 0x4F, 0xF5, 0x5B, 0xCC, 0xC0, \
        0xA1, 0x51, 0xAF, 0x5F, 0x0D, 0xC8, 0xB4, 0xBD, \
        0x45, 0xBF, 0x37, 0xDF, 0x36, 0x5C, 0x1A, 0x65, \
        0xE6, 0x8C, 0xFD, 0xA7, 0x6D, 0x4D, 0xA7, 0x08, \
        0xDF, 0x1F, 0xB2, 0xBC, 0x2E, 0x4A, 0x43, 0x71 }

#define DHM_RFC5114_MODP_1024_G_BIN {                   \
        0xA4, 0xD1, 0xCB, 0xD5, 0xC3, 0xFD, 0x34, 0x12, \
        0x67, 0x65, 0xA4, 0x42, 0xEF, 0xB9, 0x99, 0x05, \
        0xF8, 0x10, 0x4D, 0xD2, 0x58, 0xAC, 0x50, 0x7F, \
        0xD6, 0x40, 0x6C, 0xFF, 0x14, 0x26, 0x6D, 0x31, \
        0x26, 0x6F, 0xEA, 0x1E, 0x5C, 0x41, 0x56, 0x4B, \
        0x77, 0x7E, 0x69, 0x0F, 0x55, 0x04, 0xF2, 0x13, \
        0x16, 0x02, 0x17, 0xB4, 0xB0, 0x1B, 0x88, 0x6A, \
        0x5E, 0x91, 0x54, 0x7F, 0x9E, 0x27, 0x49, 0xF4, \
        0xD7, 0xFB, 0xD7, 0xD3, 0xB9, 0xA9, 0x2E, 0xE1, \
        0x90, 0x9D, 0x0D, 0x22, 0x63, 0xF8, 0x0A, 0x76, \
        0xA6, 0xA2, 0x4C, 0x08, 0x7A, 0x09, 0x1F, 0x53, \
        0x1D, 0xBF, 0x0A, 0x01, 0x69, 0xB6, 0xA2, 0x8A, \
        0xD6, 0x62, 0xA4, 0xD1, 0x8E, 0x73, 0xAF, 0xA3, \
        0x2D, 0x77, 0x9D, 0x59, 0x18, 0xD0, 0x8B, 0xC8, \
        0x85, 0x8F, 0x4D, 0xCE, 0xF9, 0x7C, 0x2A, 0x24, \
        0x85, 0x5E, 0x6E, 0xEB, 0x22, 0xB3, 0xB2, 0xE5 }

#define DHM_SIZE 128

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

static int match(FAR unsigned char *a, FAR unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  perror("dhm mismatch");

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
  memset(ctx, 0, sizeof(cryptodev_context));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      perror("cryptodev_init open /dev/crypto failed");
      cryptodev_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->cryptodev_fd) == -1)
    {
      perror("cryptodev_init CRIOGET failed");
      cryptodev_free(ctx);
      return 1;
    }

  return 0;
}

static int dh_make_public(FAR cryptodev_context *ctx,
                          FAR const unsigned char *p, size_t p_size,
                          FAR const unsigned char *g, size_t g_size,
                          FAR unsigned char *x,  size_t x_size,
                          FAR unsigned char *gx, size_t gx_size)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = CRK_DH_MAKE_PUBLIC;
  cryptk.crk_iparams = 2;
  cryptk.crk_oparams = 2;

  /* inputs: g p
   * outputs: x gx
   */

  cryptk.crk_param[0].crp_p = (caddr_t) g;
  cryptk.crk_param[0].crp_nbits = g_size * 8;
  cryptk.crk_param[1].crp_p = (caddr_t) p;
  cryptk.crk_param[1].crp_nbits = p_size * 8;
  cryptk.crk_param[2].crp_p = (caddr_t) x;
  cryptk.crk_param[2].crp_nbits = x_size * 8;
  cryptk.crk_param[3].crp_p = (caddr_t) gx;
  cryptk.crk_param[3].crp_nbits = gx_size * 8;
  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("dh_make_public failed");
      return 1;
    }

  return 0;
}

static int dh_compute_key(FAR cryptodev_context *ctx,
                          FAR const unsigned char *gy, size_t gy_size,
                          FAR const unsigned char *x, size_t x_size,
                          FAR const unsigned char *p, size_t p_size,
                          FAR unsigned char *k, size_t k_size)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = CRK_DH_COMPUTE_KEY;
  cryptk.crk_iparams = 3;
  cryptk.crk_oparams = 1;

  /* inputs: pub_key priv_key p */

  cryptk.crk_param[0].crp_p = (caddr_t) gy;
  cryptk.crk_param[0].crp_nbits = gy_size * 8;
  cryptk.crk_param[1].crp_p = (caddr_t) x;
  cryptk.crk_param[1].crp_nbits = x_size * 8;
  cryptk.crk_param[2].crp_p = (caddr_t) p;
  cryptk.crk_param[2].crp_nbits = p_size * 8;
  cryptk.crk_param[3].crp_p = (caddr_t) k;
  cryptk.crk_param[3].crp_nbits = k_size * 8;
  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("dh_compute_key failed");
      return 1;
    }

  return 0;
}

static int test_dh_compute_key(void)
{
  int ret;
  cryptodev_context ctx;
  static const unsigned char dhm_p_1024[] =
      DHM_RFC5114_MODP_1024_P_BIN;
  static const unsigned char dhm_g_1024[] =
      DHM_RFC5114_MODP_1024_G_BIN;
  unsigned char x1[DHM_SIZE];
  unsigned char gx1[DHM_SIZE];
  unsigned char k1[DHM_SIZE];
  unsigned char x2[DHM_SIZE];
  unsigned char gx2[DHM_SIZE];
  unsigned char k2[DHM_SIZE];

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      goto free;
    }

  /* X-private key  GX-pulic key  K-share key */

  memset(x1, 0, DHM_SIZE);
  memset(gx1, 0, DHM_SIZE);
  memset(k1, 0, DHM_SIZE);
  memset(x2, 0, DHM_SIZE);
  memset(gx2, 0, DHM_SIZE);
  memset(k2, 0, DHM_SIZE);

  ret = dh_make_public(&ctx, dhm_p_1024, DHM_SIZE, dhm_g_1024,
                       DHM_SIZE, x1, DHM_SIZE, gx1, DHM_SIZE);
  if (ret != 0)
    {
      printf("DH make Alice's public key failed\n");
      goto free;
    }

  ret = dh_make_public(&ctx, dhm_p_1024, DHM_SIZE, dhm_g_1024,
                       DHM_SIZE, x2, DHM_SIZE, gx2, DHM_SIZE);
  if (ret != 0)
    {
      printf("DH make Bob's public failed\n");
      goto free;
    }

  ret = dh_compute_key(&ctx, gx2, DHM_SIZE, x1, DHM_SIZE,
                       dhm_p_1024, DHM_SIZE, k1, DHM_SIZE);
  if (ret != 0)
    {
      printf("DH compute Alice's share key failed\n");
      goto free;
    }

  ret = dh_compute_key(&ctx, gx1, DHM_SIZE, x2, DHM_SIZE,
                       dhm_p_1024, DHM_SIZE, k2, DHM_SIZE);
  if (ret != 0)
    {
      printf("DH compute Bob's share key failed\n");
      goto free;
    }

  ret = match(k1, k2, DHM_SIZE);
  if (ret != 0)
    {
      printf("mismatch share key\n");
    }

free:
  cryptodev_free(&ctx);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  if (test_dh_compute_key() != 0)
    {
      printf("test dh compute key failed\n");
    }
  else
    {
      printf("test dh compute key ok\n");
    }

  return 0;
}
