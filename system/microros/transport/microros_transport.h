/****************************************************************************
 * apps/system/microros/transport/microros_transport.h
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

#ifndef __APPS_SYSTEM_MICROROS_TRANSPORT_MICROROS_TRANSPORT_H
#define __APPS_SYSTEM_MICROROS_TRANSPORT_MICROROS_TRANSPORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <uxr/client/profile/transport/custom/custom_transport.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Backend callbacks.  Exposed for testing only. */

bool   microros_udp_open(struct uxrCustomTransport *transport);
bool   microros_udp_close(struct uxrCustomTransport *transport);
size_t microros_udp_write(struct uxrCustomTransport *transport,
                          const uint8_t *buf, size_t len, uint8_t *err);
size_t microros_udp_read(struct uxrCustomTransport *transport,
                         uint8_t *buf, size_t len,
                         int timeout_ms, uint8_t *err);

bool   microros_serial_open(struct uxrCustomTransport *transport);
bool   microros_serial_close(struct uxrCustomTransport *transport);
size_t microros_serial_write(struct uxrCustomTransport *transport,
                             const uint8_t *buf, size_t len, uint8_t *err);
size_t microros_serial_read(struct uxrCustomTransport *transport,
                            uint8_t *buf, size_t len,
                            int timeout_ms, uint8_t *err);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_SYSTEM_MICROROS_TRANSPORT_MICROROS_TRANSPORT_H */
