/****************************************************************************
 * apps/fsutils/passwd/passwd_base64.h
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

#ifndef __APPS_FSUTILS_PASSWD_PASSWD_BASE64_H
#define __APPS_FSUTILS_PASSWD_PASSWD_BASE64_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int passwd_base64url_encode(FAR const uint8_t *in, size_t inlen,
                            FAR char *out, size_t outlen);
int passwd_base64url_decode(FAR const char *in,
                            FAR uint8_t *out, size_t outmax,
                            FAR size_t *outlen);

#endif /* __APPS_FSUTILS_PASSWD_PASSWD_BASE64_H */
