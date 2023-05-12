/****************************************************************************
 * apps/crypto/controlse/x509_utils.h
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

#ifndef __INCLUDE_APPS_CRYPTO_CONTROLSE_X509_UTILS_H_
#define __INCLUDE_APPS_CRYPTO_CONTROLSE_X509_UTILS_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int convert_public_key_raw_to_pem(FAR char *pem_buf, size_t pem_buf_size,
                                  FAR uint8_t *key_buf, size_t key_buf_size);
int convert_public_key_pem_to_raw(FAR uint8_t *key_buf, size_t key_buf_size,
                                  FAR size_t *key_size, char *pem_buf);
int convert_pem_certificate_or_csr_to_der(FAR uint8_t *der_buf,
                                          size_t der_buf_size,
                                          FAR size_t *der_content_size,
                                          FAR char *pem_buf,
                                          size_t pem_buf_size);
int convert_der_certificate_or_csr_to_pem(FAR char *pem_buf,
                                          size_t pem_buf_size,
                                          FAR size_t *pem_content_size,
                                          FAR uint8_t *der_buf,
                                          size_t der_buf_size);
int sign_csr(int se05x_fd, uint32_t private_key_id, FAR char *crt_pem_buf,
             size_t crt_pem_buf_size, FAR char *csr_pem_buf,
             size_t csr_pem_buf_content_size);

#endif /* __INCLUDE_APPS_CRYPTO_CONTROLSE_X509_UTILS_H_ */
