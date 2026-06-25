/****************************************************************************
 * apps/fsutils/passwd/passwd_pbkdf2.h
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
 *
 ****************************************************************************/

#ifndef __APPS_FSUTILS_PASSWD_PASSWD_PBKDF2_H
#define __APPS_FSUTILS_PASSWD_PASSWD_PBKDF2_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>

#include <sys/types.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int passwd_pbkdf2_hmac_sha256(FAR const uint8_t *pass, size_t passlen,
                              FAR const uint8_t *salt, size_t saltlen,
                              uint32_t iterations,
                              FAR uint8_t *out, size_t outlen);

#endif /* __APPS_FSUTILS_PASSWD_PASSWD_PBKDF2_H */
