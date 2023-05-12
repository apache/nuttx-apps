/****************************************************************************
 * apps/crypto/controlse/mbedtls_extension.h
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
 *
 ****************************************************************************/

/* Copyright 2023 NXP */

#ifndef __INCLUDE_APPS_CRYPTO_CONTROLSE_MBEDTLS_EXTENSION_H_
#define __INCLUDE_APPS_CRYPTO_CONTROLSE_MBEDTLS_EXTENSION_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <mbedtls/x509_crt.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int mbedtls_x509write_crt_der_se05x(mbedtls_x509write_cert *ctx,
                                    unsigned char *buf, size_t size,
                                    int se05x_fd, uint32_t private_key_id);

#endif /* __INCLUDE_APPS_CRYPTO_CONTROLSE_MBEDTLS_EXTENSION_H_ */
