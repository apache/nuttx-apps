/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/statem.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_STATEM_H
#define OPENSSL_MBEDTLS_WRAPPER_STATEM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <openssl/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum
{
/* No handshake in progress */

  MSG_FLOW_UNINITED,

/* A permanent error with this connection */

  MSG_FLOW_ERROR,

/* We are about to renegotiate */

  MSG_FLOW_RENEGOTIATE,

/* We are reading messages */

  MSG_FLOW_READING,

/* We are writing messages */

  MSG_FLOW_WRITING,

/* Handshake has finished */

  MSG_FLOW_FINISHED
}
MSG_FLOW_STATE;

struct ossl_statem_st
{
  MSG_FLOW_STATE state;
  int hand_state;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int ossl_statem_in_error(const SSL *ssl);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_STACK_H */