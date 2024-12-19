/****************************************************************************
 * apps/crypto/mbedtls/include/dev_alt.h
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

#ifndef __APPS_CRYPTO_MBEDTLS_INCLUDE_DEV_ALT_H
#define __APPS_CRYPTO_MBEDTLS_INCLUDE_DEV_ALT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <crypto/cryptodev.h>

typedef struct cryptodev_context_s
{
  int fd;
  struct session_op session;
  struct crypt_op crypt;
}
cryptodev_context_t;

int cryptodev_init(FAR cryptodev_context_t *ctx);
int cryptodev_clone(FAR cryptodev_context_t *dst,
                    FAR const cryptodev_context_t *src);
void cryptodev_free(FAR cryptodev_context_t *ctx);
int cryptodev_get_session(FAR cryptodev_context_t *ctx);
void cryptodev_free_session(FAR cryptodev_context_t *ctx);
int cryptodev_crypt(FAR cryptodev_context_t *ctx);

#endif /* __APPS_CRYPTO_MBEDTLS_INCLUDE_DEV_ALT_H */
