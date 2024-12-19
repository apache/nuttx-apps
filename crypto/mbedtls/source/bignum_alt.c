/****************************************************************************
 * apps/crypto/mbedtls/source/bignum_alt.c
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
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <nuttx/math/math_ioctl.h>
#include <nuttx/math/mpi.h>
#include "mbedtls/bignum.h"
#include "mbedtls/platform.h"

#define MBEDTLS_ROUNDUP(v, size) (((v) + (size - 1)) & ~(size - 1))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline
void mbedtls_mpi_to_mpiparam(FAR struct mpiparam *a,
                             FAR const mbedtls_mpi *A)
{
  a->n = A->n * sizeof(mbedtls_mpi_uint);
  a->s = A->s;
  a->p = (FAR uint8_t *)A->p;
}

static inline
void mpiparam_to_mbedtls_mpi(FAR mbedtls_mpi *A,
                             FAR const struct mpiparam *a)
{
  A->n = a->n / sizeof(mbedtls_mpi_uint);
  A->s = a->s;
  A->p = (FAR mbedtls_mpi_uint *)a->p;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_mpi_add_mpi(FAR mbedtls_mpi *X, FAR const mbedtls_mpi *A,
                        FAR const mbedtls_mpi *B)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_ADD;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], B);

  mbedtls_mpi_grow(X, MBEDTLS_ROUNDUP(MAX(A->n, B->n) + 1,
                                              sizeof(mbedtls_mpi_uint)));
  mbedtls_mpi_to_mpiparam(&mpi.param[2], X);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(X, &mpi.param[2]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_sub_mpi(FAR mbedtls_mpi *X, FAR const mbedtls_mpi *A,
                        FAR const mbedtls_mpi *B)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_SUB;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], B);

  mbedtls_mpi_grow(X, MBEDTLS_ROUNDUP(MAX(A->n, B->n) + 1,
                                              sizeof(mbedtls_mpi_uint)));
  mbedtls_mpi_to_mpiparam(&mpi.param[2], X);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(X, &mpi.param[2]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_mul_mpi(FAR mbedtls_mpi *X, FAR const mbedtls_mpi *A,
                        FAR const mbedtls_mpi *B)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_MUL;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], B);

  mbedtls_mpi_grow(X, MBEDTLS_ROUNDUP(A->n + B->n,
                                              sizeof(mbedtls_mpi_uint)));
  mbedtls_mpi_to_mpiparam(&mpi.param[2], X);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(X, &mpi.param[2]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_div_mpi(FAR mbedtls_mpi *Q, FAR mbedtls_mpi *R,
                        FAR const mbedtls_mpi *A, FAR const mbedtls_mpi *B)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_DIV;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], B);
  mbedtls_mpi_grow(Q, A->n);
  mbedtls_mpi_grow(R, B->n);
  mbedtls_mpi_to_mpiparam(&mpi.param[2], Q);
  mbedtls_mpi_to_mpiparam(&mpi.param[3], R);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(Q, &mpi.param[2]);
      mpiparam_to_mbedtls_mpi(R, &mpi.param[3]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_mod_mpi(FAR mbedtls_mpi *R, FAR const mbedtls_mpi *A,
                        FAR const mbedtls_mpi *B)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_MOD;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], B);
  mbedtls_mpi_grow(R, B->n);
  mbedtls_mpi_to_mpiparam(&mpi.param[2], R);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(R, &mpi.param[2]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_exp_mod(FAR mbedtls_mpi *X, FAR const mbedtls_mpi *A,
                        FAR const mbedtls_mpi *E, FAR const mbedtls_mpi *N,
                        FAR mbedtls_mpi *)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_EXP_MOD;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], E);
  mbedtls_mpi_to_mpiparam(&mpi.param[2], N);
  mbedtls_mpi_grow(X, N->n);
  mbedtls_mpi_to_mpiparam(&mpi.param[3], X);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(X, &mpi.param[3]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_gcd(FAR mbedtls_mpi *G, FAR const mbedtls_mpi *A,
                    FAR const mbedtls_mpi *B)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_GCD;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], B);
  mbedtls_mpi_grow(G, MIN(A->n, B->n));
  mbedtls_mpi_to_mpiparam(&mpi.param[2], G);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(G, &mpi.param[2]);
    }

  close(fd);
  return ret;
}

int mbedtls_mpi_inv_mod(FAR mbedtls_mpi *X, FAR const mbedtls_mpi *A,
                        FAR const mbedtls_mpi *N)
{
  int ret;
  int fd;
  struct mpi_calc_s mpi;

  fd = open("/dev/mpi0", O_RDWR);
  if (fd < 0)
    {
      return -errno;
    }

  mpi.op = MPI_CALC_FUNC_INV_MOD;
  mbedtls_mpi_to_mpiparam(&mpi.param[0], A);
  mbedtls_mpi_to_mpiparam(&mpi.param[1], N);
  mbedtls_mpi_grow(X, N->n);
  mbedtls_mpi_to_mpiparam(&mpi.param[2], X);
  ret = ioctl(fd, MATHIOC_MPI_CALC, (unsigned long)((uintptr_t)&mpi));
  if (ret >= 0)
    {
      mpiparam_to_mbedtls_mpi(X, &mpi.param[2]);
    }

  close(fd);
  return ret;
}
