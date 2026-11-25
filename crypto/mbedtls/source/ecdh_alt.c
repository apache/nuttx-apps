/****************************************************************************
 * apps/crypto/mbedtls/source/ecdh_alt.c
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

#include "dev_alt.h"
#include "mbedtls/bignum.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/error.h"
#include "mbedtls/platform_util.h"

#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ECDH_SECP256R1_KEYLEN 32

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_ecdh_gen_public(FAR mbedtls_ecp_group *grp,
                            FAR mbedtls_mpi *d,
                            FAR mbedtls_ecp_point *Q,
                            FAR int (*f_rng)(void *, unsigned char *,
                                             size_t),
                            FAR void *p_rng)
{
  uint8_t d_buf[ECDH_SECP256R1_KEYLEN];
  uint8_t x_buf[ECDH_SECP256R1_KEYLEN];
  uint8_t y_buf[ECDH_SECP256R1_KEYLEN];
  cryptodev_context_t ctx;
  int ret;

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      return MBEDTLS_ERR_ECP_ALLOC_FAILED;
    }

  switch (grp->id)
    {
      case MBEDTLS_ECP_DP_SECP256R1:
        ctx.cryptk.crk_op = CRK_ECDH_GEN_PUBLIC;
        break;
      default:
        ret = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

  ctx.cryptk.crk_iparams = 0;
  ctx.cryptk.crk_oparams = 4;
  ctx.cryptk.crk_param[0].crp_p = (caddr_t)d_buf;
  ctx.cryptk.crk_param[0].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;
  ctx.cryptk.crk_param[1].crp_p = (caddr_t)x_buf;
  ctx.cryptk.crk_param[1].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;
  ctx.cryptk.crk_param[2].crp_p = (caddr_t)y_buf;
  ctx.cryptk.crk_param[2].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;
  ctx.cryptk.crk_param[3].crp_p = NULL;
  ctx.cryptk.crk_param[3].crp_nbits = 0;

  ret = cryptodev_cryptk(&ctx);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
      goto cleanup;
    }

  /* cryptodev ECC integers are little-endian */

  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(d, d_buf,
                                             ECDH_SECP256R1_KEYLEN));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(&Q->MBEDTLS_PRIVATE(X),
                                             x_buf,
                                             ECDH_SECP256R1_KEYLEN));
  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(&Q->MBEDTLS_PRIVATE(Y),
                                             y_buf,
                                             ECDH_SECP256R1_KEYLEN));
  MBEDTLS_MPI_CHK(mbedtls_mpi_lset(&Q->MBEDTLS_PRIVATE(Z), 1));

cleanup:
  cryptodev_free(&ctx);
  mbedtls_platform_zeroize(d_buf, sizeof(d_buf));
  return ret;
}

int mbedtls_ecdh_compute_shared(FAR mbedtls_ecp_group *grp,
                                FAR mbedtls_mpi *z,
                                FAR const mbedtls_ecp_point *Q,
                                FAR const mbedtls_mpi *d,
                                FAR int (*f_rng)(void *, unsigned char *,
                                                 size_t),
                                FAR void *p_rng)
{
  uint8_t x_buf[ECDH_SECP256R1_KEYLEN];
  uint8_t y_buf[ECDH_SECP256R1_KEYLEN];
  uint8_t d_buf[ECDH_SECP256R1_KEYLEN];
  uint8_t z_buf[ECDH_SECP256R1_KEYLEN];
  cryptodev_context_t ctx;
  int ret;

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      return MBEDTLS_ERR_ECP_ALLOC_FAILED;
    }

  switch (grp->id)
    {
      case MBEDTLS_ECP_DP_SECP256R1:
        ctx.cryptk.crk_op = CRK_ECDH_COMPUTE_SHARED;
        break;
      default:
        ret = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

  MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary_le(&Q->MBEDTLS_PRIVATE(X),
                                              x_buf,
                                              ECDH_SECP256R1_KEYLEN));
  MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary_le(&Q->MBEDTLS_PRIVATE(Y),
                                              y_buf,
                                              ECDH_SECP256R1_KEYLEN));
  MBEDTLS_MPI_CHK(mbedtls_mpi_write_binary_le(d, d_buf,
                                              ECDH_SECP256R1_KEYLEN));

  ctx.cryptk.crk_iparams = 3;
  ctx.cryptk.crk_oparams = 1;
  ctx.cryptk.crk_param[0].crp_p = (caddr_t)d_buf;
  ctx.cryptk.crk_param[0].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;
  ctx.cryptk.crk_param[1].crp_p = (caddr_t)x_buf;
  ctx.cryptk.crk_param[1].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;
  ctx.cryptk.crk_param[2].crp_p = (caddr_t)y_buf;
  ctx.cryptk.crk_param[2].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;
  ctx.cryptk.crk_param[3].crp_p = (caddr_t)z_buf;
  ctx.cryptk.crk_param[3].crp_nbits = ECDH_SECP256R1_KEYLEN * 8;

  ret = cryptodev_cryptk(&ctx);
  if (ret != 0)
    {
      ret = MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
      goto cleanup;
    }

  MBEDTLS_MPI_CHK(mbedtls_mpi_read_binary_le(z, z_buf,
                                             ECDH_SECP256R1_KEYLEN));

cleanup:
  cryptodev_free(&ctx);
  mbedtls_platform_zeroize(d_buf, sizeof(d_buf));
  mbedtls_platform_zeroize(z_buf, sizeof(z_buf));
  return ret;
}
