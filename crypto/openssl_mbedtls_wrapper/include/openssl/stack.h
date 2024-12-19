/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/include/openssl/stack.h
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

#ifndef OPENSSL_MBEDTLS_WRAPPER_STACK_H
#define OPENSSL_MBEDTLS_WRAPPER_STACK_H

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

struct stack_st
{
    char **data;
    int num_alloc;
    OPENSSL_sk_compfunc c;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

OPENSSL_STACK *OPENSSL_sk_new(OPENSSL_sk_compfunc c);
OPENSSL_STACK *OPENSSL_sk_new_null(void);
void OPENSSL_sk_free(OPENSSL_STACK *stack);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_MBEDTLS_WRAPPER_STACK_H */
