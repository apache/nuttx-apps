/****************************************************************************
 * apps/include/nxmodbus/nxmb_raw.h
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

#ifndef __APPS_INCLUDE_NXMODBUS_NXMB_RAW_H
#define __APPS_INCLUDE_NXMODBUS_NXMB_RAW_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>
#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <nxmodbus/nxmodbus.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXMB_RAW_HDR_SIZE 8

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: nxmb_raw_submit_rx
 *
 * Description:
 *   Submit a received raw ADU into the NxModbus protocol core.
 *
 * Input Parameters:
 *   h   - The NxModbus instance receiving the frame.
 *   adu - The received raw ADU container.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_raw_submit_rx(nxmb_handle_t h, FAR const struct nxmb_adu_s *adu);

/****************************************************************************
 * Name: nxmb_raw_put_header
 *
 * Description:
 *   Serialize the 8-byte MBAP+FC header for an ADU.
 *
 * Input Parameters:
 *   adu - The ADU providing the header fields.
 *   hdr - The destination buffer for the serialized header.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void nxmb_raw_put_header(FAR const struct nxmb_adu_s *adu, FAR uint8_t *hdr);

/****************************************************************************
 * Name: nxmb_raw_get_header
 *
 * Description:
 *   Parse the 8-byte MBAP+FC header into an ADU structure.
 *
 * Input Parameters:
 *   adu - The destination ADU structure.
 *   hdr - The source buffer containing the serialized header.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void nxmb_raw_get_header(FAR struct nxmb_adu_s *adu, FAR const uint8_t *hdr);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NXMODBUS_NXMB_RAW_H */
