/****************************************************************************
 * apps/include/nxmodbus/nxmb_client.h
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

#ifndef __APPS_INCLUDE_NXMODBUS_NXMB_CLIENT_H
#define __APPS_INCLUDE_NXMODBUS_NXMB_CLIENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>
#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <nxmodbus/nxmodbus.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: nxmb_read_coils
 *
 * Description:
 *   Read coil values from a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The first coil address to read.
 *   count - The number of coils to read.
 *   buf   - The destination buffer for the returned coil values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_read_coils(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                    uint16_t count, FAR uint8_t *buf);

/****************************************************************************
 * Name: nxmb_read_discrete
 *
 * Description:
 *   Read discrete input values from a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The first discrete input address to read.
 *   count - The number of discrete inputs to read.
 *   buf   - The destination buffer for the returned values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_read_discrete(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                       uint16_t count, FAR uint8_t *buf);

/****************************************************************************
 * Name: nxmb_read_input
 *
 * Description:
 *   Read input registers from a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The first input register address to read.
 *   count - The number of registers to read.
 *   buf   - The destination buffer for the returned register values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_read_input(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                    uint16_t count, FAR uint16_t *buf);

/****************************************************************************
 * Name: nxmb_read_holding
 *
 * Description:
 *   Read holding registers from a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The first holding register address to read.
 *   count - The number of registers to read.
 *   buf   - The destination buffer for the returned register values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_read_holding(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                      uint16_t count, FAR uint16_t *buf);

/****************************************************************************
 * Name: nxmb_write_coil
 *
 * Description:
 *   Write a single coil on a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The coil address to update.
 *   value - The coil state to write.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_write_coil(nxmb_handle_t h, uint8_t uid, uint16_t addr, bool value);

/****************************************************************************
 * Name: nxmb_write_holding
 *
 * Description:
 *   Write a single holding register on a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The holding register address to update.
 *   value - The register value to write.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_write_holding(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                       uint16_t value);

/****************************************************************************
 * Name: nxmb_write_coils
 *
 * Description:
 *   Write multiple coils on a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The first coil address to update.
 *   count - The number of coils to write.
 *   buf   - The source buffer containing coil values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_write_coils(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                     uint16_t count, FAR const uint8_t *buf);

/****************************************************************************
 * Name: nxmb_write_holdings
 *
 * Description:
 *   Write multiple holding registers on a remote unit.
 *
 * Input Parameters:
 *   h     - The NxModbus client instance.
 *   uid   - The remote unit identifier.
 *   addr  - The first holding register address to update.
 *   count - The number of registers to write.
 *   buf   - The source buffer containing register values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_write_holdings(nxmb_handle_t h, uint8_t uid, uint16_t addr,
                        uint16_t count, FAR const uint16_t *buf);

/****************************************************************************
 * Name: nxmb_readwrite_holdings
 *
 * Description:
 *   Perform a combined read/write of holding registers on a remote unit
 *   using FC23 (0x17). The write is performed before the read on the
 *   server side.
 *
 * Input Parameters:
 *   h        - The NxModbus client instance.
 *   uid      - The remote unit identifier.
 *   rd_addr  - The first holding register address to read.
 *   rd_count - The number of registers to read (1-125).
 *   rd_buf   - The destination buffer for the returned register values.
 *   wr_addr  - The first holding register address to write.
 *   wr_count - The number of registers to write (1-121).
 *   wr_buf   - The source buffer containing register values to write.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_readwrite_holdings(nxmb_handle_t h, uint8_t uid, uint16_t rd_addr,
                            uint16_t rd_count, FAR uint16_t *rd_buf,
                            uint16_t wr_addr, uint16_t wr_count,
                            FAR const uint16_t *wr_buf);

/****************************************************************************
 * Name: nxmb_set_timeout
 *
 * Description:
 *   Set the client-side response timeout.
 *
 * Input Parameters:
 *   h          - The NxModbus client instance.
 *   timeout_ms - The timeout in milliseconds.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_set_timeout(nxmb_handle_t h, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NXMODBUS_NXMB_CLIENT_H */
