/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/ec.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_EC_H
#define OPENSSL_MBEDTLS_WRAPPER_EC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OPENSSL_EC_NAMED_CURVE 1

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* point_conversion_form_t enumerates forms, as defined in X9.62 (ECDSA), for
 * the encoding of a elliptic curve point (x,y)
 */

typedef enum
{
  /* POINT_CONVERSION_COMPRESSED indicates that the point is encoded as z||x,
   * where the octet z specifies which solution of the quadratic equation y
   * is.
   */

  POINT_CONVERSION_COMPRESSED = 2,

  /* POINT_CONVERSION_UNCOMPRESSED indicates that the point is encoded as
   * z||x||y, where z is the octet 0x04.
   */

  POINT_CONVERSION_UNCOMPRESSED = 4,

  /* POINT_CONVERSION_HYBRID indicates that the point is encoded as z||x||y,
   * where z specifies which solution of the quadratic equation y is. This is
   * not supported by the code and has never been observed in use.
   *
   * TODO(agl): remove once node.js no longer references this.
   */

  POINT_CONVERSION_HYBRID = 6,
} point_conversion_form_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void EC_GROUP_free(EC_GROUP *a);

void EC_KEY_free(EC_KEY *a);

void EC_POINT_free(EC_POINT *a);

int EC_GROUP_get_curve_name(const EC_GROUP *group);

EC_GROUP *EC_GROUP_new_by_curve_name(int nid);

int EC_GROUP_get_order(const EC_GROUP *group, BIGNUM *order, BN_CTX *ctx);

int EC_POINT_get_affine_coordinates_GFp(const EC_GROUP *group,
                                        const EC_POINT *point, BIGNUM *x,
                                        BIGNUM *y, BN_CTX *ctx);

/* EC_GROUP_set_point_conversion_form aborts the process if |form| is not
 * |POINT_CONVERSION_UNCOMPRESSED| and otherwise does nothing.
 */

void EC_GROUP_set_point_conversion_form(EC_GROUP *group,
                                        point_conversion_form_t form);

void EC_GROUP_set_asn1_flag(EC_GROUP *group, int flag);

EC_POINT *EC_POINT_new(const EC_GROUP *group);

int EC_POINT_oct2point(const EC_GROUP *group, EC_POINT *point,
                       const uint8_t *buf, size_t len,
                       BN_CTX *ctx);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_EC_H */
