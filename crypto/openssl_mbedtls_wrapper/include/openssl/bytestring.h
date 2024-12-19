/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/bytestring.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_BYTESTRING_H
#define OPENSSL_MBEDTLS_WRAPPER_BYTESTRING_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/base.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct cbb_buffer_st
{
  uint8_t * buf;

  /* len is the number of valid bytes in |buf|. */

  size_t len;

  /* cap is the size of |buf|. */

  size_t cap;

  /* can_resize is one iff |buf| is owned by this object. If not then |buf|
   * cannot be resized.
   */

  unsigned can_resize : 1;

  /* error is one if there was an error writing to this CBB. All future
   * operations will fail.
   */

  unsigned error : 1;
};

struct cbb_child_st
{
  /* base is a pointer to the buffer this |CBB| writes to. */

  struct cbb_buffer_st * base;

  /* offset is the number of bytes from the start of
   * |base->buf| to this |CBB|'s
   * pending length prefix.
   */

  size_t offset;

  /* pending_len_len contains the number of bytes in this |CBB|'s pending
   * length-prefix, or zero if no length-prefix is pending.
   */

  uint8_t pending_len_len;
  unsigned pending_is_asn1 : 1;
};

struct cbb_st
{
  /* child points to a child CBB if a length-prefix is pending.
   * CBB* child;
   * is_child is one if this is a child |CBB| and zero if it is a top-level
   * |CBB|. This determines which arm of the union is valid.
   */

  char is_child;
  union
  {
    struct cbb_buffer_st base;
    struct cbb_child_st child;
  } u;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int CBB_init(CBB *cbb, size_t initial_capacity);

int CBB_finish(CBB *cbb, uint8_t **out_data, size_t *out_len);

void CBB_cleanup(CBB *cbb);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_BYTESTRING_H */
