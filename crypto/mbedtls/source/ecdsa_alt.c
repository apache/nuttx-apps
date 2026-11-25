/****************************************************************************
 * apps/crypto/mbedtls/source/ecdsa_alt.c
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

#include <errno.h>
#include <string.h>

#include "dev_alt.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/error.h"
#include "mbedtls/platform_util.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ECDSA_SECP256R1_KEYLEN 32

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* cryptodev ECC integers are little-endian; SHA256 output is big-endian */

static void ecdsa_hash_to_le(FAR unsigned char *dst,
                             FAR const unsigned char *src)
{
  size_t i;

  for (i = 0; i < ECDSA_SECP256R1_KEYLEN; i++)
    {
      dst[i] = src[ECDSA_SECP256R1_KEYLEN - 1 - i];
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_ecdsa_sign(FAR mbedtls_ecp_group *grp, FAR mbedtls_mpi *r,
                       FAR mbedtls_mpi *s, FAR const mbedtls_mpi *d,
                       FAR const unsigned char *buf, size_t blen,
                       FAR int (*f_rng)(void *, unsigned char *, size_t),
                       FAR void *p_rng)
{
  unsigned char dbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char hbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char rbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char sbuf[ECDSA_SECP256R1_KEYLEN];
  cryptodev_context_t ctx;
  int ret;

  if (grp->id != MBEDTLS_ECP_DP_SECP256R1)
    {
      return MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
    }

  if (blen != sizeof(hbuf))
    {
      return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    }

  ret = mbedtls_mpi_write_binary_le(d, dbuf, sizeof(dbuf));
  if (ret != 0)
    {
      return ret;
    }

  ecdsa_hash_to_le(hbuf, buf);

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_ECP_ALLOC_FAILED;
      goto cleanup;
    }

  ctx.cryptk.crk_op = CRK_ECDSA_SECP256R1_SIGN;
  ctx.cryptk.crk_optype = CRYPTO_OP_SIGN;
  ctx.cryptk.crk_keytype = CRYPTO_KEY_TYPE_PRIVATE;
  ctx.cryptk.crk_iparams = 2;
  ctx.cryptk.crk_oparams = 2;
  ctx.cryptk.crk_param[0].crp_p = (caddr_t)dbuf;
  ctx.cryptk.crk_param[0].crp_nbits = sizeof(dbuf) * 8;
  ctx.cryptk.crk_param[1].crp_p = (caddr_t)hbuf;
  ctx.cryptk.crk_param[1].crp_nbits = sizeof(hbuf) * 8;
  ctx.cryptk.crk_param[2].crp_p = (caddr_t)rbuf;
  ctx.cryptk.crk_param[2].crp_nbits = sizeof(rbuf) * 8;
  ctx.cryptk.crk_param[3].crp_p = (caddr_t)sbuf;
  ctx.cryptk.crk_param[3].crp_nbits = sizeof(sbuf) * 8;

  ret = cryptodev_cryptk(&ctx);
  cryptodev_free(&ctx);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
      goto cleanup;
    }

  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(r, rbuf, sizeof(rbuf)));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(s, sbuf, sizeof(sbuf)));

cleanup:
  mbedtls_platform_zeroize(dbuf, sizeof(dbuf));
  return ret;
}

int mbedtls_ecdsa_verify(FAR mbedtls_ecp_group *grp,
                         FAR const unsigned char *buf, size_t blen,
                         FAR const mbedtls_ecp_point *Q,
                         FAR const mbedtls_mpi *r,
                         FAR const mbedtls_mpi *s)
{
  unsigned char xbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char ybuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char rbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char sbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char hbuf[ECDSA_SECP256R1_KEYLEN];
  cryptodev_context_t ctx;
  int ret;

  if (grp->id != MBEDTLS_ECP_DP_SECP256R1)
    {
      return MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
    }

  if (blen != sizeof(hbuf))
    {
      return MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    }

  MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary_le(&Q->MBEDTLS_PRIVATE(X),
                                              xbuf, sizeof(xbuf)));
  MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary_le(&Q->MBEDTLS_PRIVATE(Y),
                                              ybuf, sizeof(ybuf)));

  /* An r or s wider than the curve cannot be a valid signature */

  if (mbedtls_mpi_write_binary_le(r, rbuf, sizeof(rbuf)) != 0 ||
      mbedtls_mpi_write_binary_le(s, sbuf, sizeof(sbuf)) != 0)
    {
      return MBEDTLS_ERR_ECP_VERIFY_FAILED;
    }

  ecdsa_hash_to_le(hbuf, buf);

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      return MBEDTLS_ERR_ECP_ALLOC_FAILED;
    }

  ctx.cryptk.crk_op = CRK_ECDSA_SECP256R1_VERIFY;
  ctx.cryptk.crk_optype = CRYPTO_OP_VERIFY;
  ctx.cryptk.crk_keytype = CRYPTO_KEY_TYPE_PUBLIC;
  ctx.cryptk.crk_iparams = 6;
  ctx.cryptk.crk_oparams = 0;
  ctx.cryptk.crk_param[0].crp_p = (caddr_t)xbuf;
  ctx.cryptk.crk_param[0].crp_nbits = sizeof(xbuf) * 8;
  ctx.cryptk.crk_param[1].crp_p = (caddr_t)ybuf;
  ctx.cryptk.crk_param[1].crp_nbits = sizeof(ybuf) * 8;
  ctx.cryptk.crk_param[2].crp_p = NULL;
  ctx.cryptk.crk_param[2].crp_nbits = 0;
  ctx.cryptk.crk_param[3].crp_p = (caddr_t)rbuf;
  ctx.cryptk.crk_param[3].crp_nbits = sizeof(rbuf) * 8;
  ctx.cryptk.crk_param[4].crp_p = (caddr_t)sbuf;
  ctx.cryptk.crk_param[4].crp_nbits = sizeof(sbuf) * 8;
  ctx.cryptk.crk_param[5].crp_p = (caddr_t)hbuf;
  ctx.cryptk.crk_param[5].crp_nbits = sizeof(hbuf) * 8;

  ret = cryptodev_cryptk(&ctx);
  cryptodev_free(&ctx);
  if (ret == -EBADMSG)
    {
      return MBEDTLS_ERR_ECP_VERIFY_FAILED;
    }

  if (ret != 0)
    {
      return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

cleanup:
  return ret;
}

int mbedtls_ecdsa_genkey(FAR mbedtls_ecdsa_context *ctx,
                         mbedtls_ecp_group_id gid,
                         FAR int (*f_rng)(void *, unsigned char *, size_t),
                         FAR void *p_rng)
{
  unsigned char dbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char xbuf[ECDSA_SECP256R1_KEYLEN];
  unsigned char ybuf[ECDSA_SECP256R1_KEYLEN];
  cryptodev_context_t dev;
  int ret;

  if (gid != MBEDTLS_ECP_DP_SECP256R1)
    {
      return MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
    }

  ret = mbedtls_ecp_group_load(&ctx->MBEDTLS_PRIVATE(grp), gid);
  if (ret != 0)
    {
      return ret;
    }

  ret = cryptodev_init(&dev);
  if (ret != 0)
    {
      return MBEDTLS_ERR_ECP_ALLOC_FAILED;
    }

  dev.cryptk.crk_op = CRK_ECDSA_SECP256R1_GENKEY;
  dev.cryptk.crk_optype = CRYPTO_OP_GENKEY;
  dev.cryptk.crk_iparams = 0;
  dev.cryptk.crk_oparams = 4;
  dev.cryptk.crk_param[0].crp_p = (caddr_t)dbuf;
  dev.cryptk.crk_param[0].crp_nbits = sizeof(dbuf) * 8;
  dev.cryptk.crk_param[1].crp_p = (caddr_t)xbuf;
  dev.cryptk.crk_param[1].crp_nbits = sizeof(xbuf) * 8;
  dev.cryptk.crk_param[2].crp_p = (caddr_t)ybuf;
  dev.cryptk.crk_param[2].crp_nbits = sizeof(ybuf) * 8;
  dev.cryptk.crk_param[3].crp_p = NULL;
  dev.cryptk.crk_param[3].crp_nbits = 0;

  ret = cryptodev_cryptk(&dev);
  cryptodev_free(&dev);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
      goto cleanup;
    }

  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(&ctx->MBEDTLS_PRIVATE(d),
                                             dbuf, sizeof(dbuf)));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(
                    &ctx->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X),
                    xbuf, sizeof(xbuf)));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(
                    &ctx->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y),
                    ybuf, sizeof(ybuf)));
  MBEDTLS_MPI_CHK(mbedtls_mpi_lset(
                    &ctx->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Z), 1));

cleanup:
  mbedtls_platform_zeroize(dbuf, sizeof(dbuf));
  return ret;
}
