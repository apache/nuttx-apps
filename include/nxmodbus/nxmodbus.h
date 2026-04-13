/****************************************************************************
 * apps/include/nxmodbus/nxmodbus.h
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

#ifndef __APPS_INCLUDE_NXMODBUS_NXMODBUS_H
#define __APPS_INCLUDE_NXMODBUS_NXMODBUS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>
#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXMB_TCP_PORT_DEFAULT 502

/* Maximum number of PDU data bytes (everything after the FC byte).
 * Configured via Kconfig (CONFIG_NXMODBUS_ADU_DATA_MAX).
 */

#define NXMB_ADU_DATA_MAX CONFIG_NXMODBUS_ADU_DATA_MAX

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Opaque handle to an NxModbus instance. */

typedef FAR struct nxmb_context_s *nxmb_handle_t;

/* Custom frame handler */

typedef CODE int (*nxmb_custom_fc_handler_t)(nxmb_handle_t ctx);

/* Modbus Application Data Unit container.
 *
 * Single canonical ADU representation shared by the protocol core and all
 * transports. Transports parse incoming wire frames into this struct and
 * serialize this struct back out on transmit.
 *
 *   trans_id, proto_id, length - MBAP header fields (TCP-only).
 *   unit_id                    - Modbus address byte.
 *   fc                         - Modbus function code byte.
 *   data[]                     - PDU payload after the function code.
 *   crc                        - RTU/ASCII checksum (serial-only).
 *
 * The MBAP `length` field counts the bytes following it: unit_id + fc +
 * data_count. Transports that don't carry MBAP still populate this field
 * with the same value so that consumers can derive data_count uniformly:
 *     data_count = adu.length - 2
 */

struct nxmb_adu_s
{
  uint16_t trans_id;
  uint16_t proto_id;
  uint16_t length;
  uint8_t  unit_id;
  uint8_t  fc;
  uint8_t  data[NXMB_ADU_DATA_MAX];
  uint16_t crc;
};

/* Callback used by raw ADU mode to emit a fully-formed frame. */

typedef int (*nxmb_raw_tx_cb_t)(FAR const struct nxmb_adu_s *adu,
                                FAR void *user_data);

/* Supported transport modes. */

enum nxmb_mode_e
{
  NXMB_MODE_INVAL = 0,
  NXMB_MODE_RTU,                /* Modbus RTU */
  NXMB_MODE_ASCII,              /* Modbus ASCII */
  NXMB_MODE_TCP,                /* Modbus TCP */
  NXMB_MODE_RAW                 /* Custom ADU transport */
};

/* Serial parity configuration. */

enum nxmb_parity_e
{
  NXMB_PAR_NONE = 0,
  NXMB_PAR_EVEN,
  NXMB_PAR_ODD
};

/* Register access direction passed to callbacks. */

enum nxmb_regmode_e
{
  NXMB_REG_READ = 0,
  NXMB_REG_WRITE
};

/* Standard Modbus exception codes. */

enum nxmb_exception_e
{
  NXMB_EX_NONE                 = 0x00,
  NXMB_EX_ILLEGAL_FUNCTION     = 0x01,
  NXMB_EX_ILLEGAL_DATA_ADDRESS = 0x02,
  NXMB_EX_ILLEGAL_DATA_VALUE   = 0x03,
  NXMB_EX_DEVICE_FAILURE       = 0x04,
  NXMB_EX_ACKNOWLEDGE          = 0x05,
  NXMB_EX_DEVICE_BUSY          = 0x06,
  NXMB_EX_MEMORY_PARITY_ERROR  = 0x08,
  NXMB_EX_GATEWAY_PATH_FAILED  = 0x0a,
  NXMB_EX_GATEWAY_TGT_FAILED   = 0x0b
};

/* Serial transport configuration. */

struct nxmb_serial_config_s
{
  FAR const char     *devpath;
  enum nxmb_parity_e  parity;
  uint32_t            baudrate;
};

/* TCP transport configuration.
 *
 * For slave mode, bindaddr selects the local address and port selects the
 * listening port. For client mode, host selects the remote endpoint and
 * port selects the remote port.
 */

struct nxmb_tcp_config_s
{
  FAR const char *host;
  FAR const char *bindaddr;
  uint16_t        port;
};

/* Raw ADU transport configuration. */

struct nxmb_raw_config_s
{
  FAR void         *user_data;
  nxmb_raw_tx_cb_t  tx_cb;
};

union nxmb_transport_config_u
{
  struct nxmb_serial_config_s serial;
  struct nxmb_tcp_config_s    tcp;
  struct nxmb_raw_config_s    raw;
};

/* Generic NxModbus instance configuration. */

struct nxmb_config_s
{
  union nxmb_transport_config_u transport;
  enum nxmb_mode_e              mode;
  uint8_t                       unit_id;
  bool                          is_client;
};

/* Application callback table for Modbus data model access.
 *
 * The protocol stack passes buffers covering a contiguous address range and
 * the application fills or consumes them depending on the register mode.
 */

struct nxmb_callbacks_s
{
  CODE int (*coil_cb)(FAR uint8_t *buf, uint16_t addr, uint16_t ncoils,
                      enum nxmb_regmode_e mode, FAR void *priv);
  CODE int (*discrete_cb)(FAR uint8_t *buf, uint16_t addr,
                          uint16_t ndiscrete, FAR void *priv);
  CODE int (*input_cb)(FAR uint8_t *buf, uint16_t addr, uint16_t nregs,
                       FAR void *priv);
  CODE int (*holding_cb)(FAR uint8_t *buf, uint16_t addr, uint16_t nregs,
                         enum nxmb_regmode_e mode, FAR void *priv);
  FAR void *priv;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: nxmb_create
 *
 * Description:
 *   Create and initialize an NxModbus instance. This function allocates one
 *   entry from the static NxModbus instance pool and initializes it for the
 *   selected transport mode.
 *
 * Input Parameters:
 *   handle - A location to receive the opaque NxModbus handle.
 *   config - The transport and role configuration for the new instance.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_create(FAR nxmb_handle_t *handle,
                FAR const struct nxmb_config_s *config);

/****************************************************************************
 * Name: nxmb_destroy
 *
 * Description:
 *   Destroy a previously created NxModbus instance. This function releases
 *   a handle previously obtained from nxmb_create().
 *
 * Input Parameters:
 *   handle - The NxModbus instance to destroy.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_destroy(nxmb_handle_t handle);

/****************************************************************************
 * Name: nxmb_set_callbacks
 *
 * Description:
 *   Register the application callback table for an instance. The registered
 *   callback table defines how the application exposes coils, discrete
 *   inputs, input registers, and holding registers to the protocol stack.
 *
 * Input Parameters:
 *   handle - The NxModbus instance to update.
 *   cbs    - The callback table to associate with the instance.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_set_callbacks(nxmb_handle_t handle,
                       FAR const struct nxmb_callbacks_s *cbs);

/****************************************************************************
 * Name: nxmb_enable
 *
 * Description:
 *   Enable the configured transport and protocol handling. This function
 *   prepares the configured transport backend so the instance can start
 *   sending or receiving Modbus ADUs.
 *
 * Input Parameters:
 *   handle - The NxModbus instance to enable.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_enable(nxmb_handle_t handle);

/****************************************************************************
 * Name: nxmb_disable
 *
 * Description:
 *   Disable an active NxModbus instance. This function stops frame
 *   processing and releases any transport state held by an enabled instance.
 *
 * Input Parameters:
 *   handle - The NxModbus instance to disable.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_disable(nxmb_handle_t handle);

/****************************************************************************
 * Name: nxmb_poll
 *
 * Description:
 *   Execute one server-side polling iteration. This function performs a
 *   single poll of the transport backend and processes one server-side
 *   Modbus transaction when data is available.
 *
 * Input Parameters:
 *   handle - The NxModbus instance to poll.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_poll(nxmb_handle_t handle);

/****************************************************************************
 * Name: nxmb_set_server_id
 *
 * Description:
 *   Configure the data returned by FC17 (Report Server ID). The response
 *   contains the server ID byte, a run indicator, and optional additional
 *   data supplied by the application.
 *
 * Input Parameters:
 *   handle     - The NxModbus instance to update.
 *   id         - The server ID byte.
 *   is_running - True if the device is in a running state.
 *   additional - Optional additional data bytes (may be NULL).
 *   addlen     - Length of additional data.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_set_server_id(nxmb_handle_t handle, uint8_t id, bool is_running,
                       FAR const uint8_t *additional, uint16_t addlen);

#ifdef CONFIG_NXMODBUS_CUSTOM_FC
/****************************************************************************
 * Name: nxmb_register_custom_fc
 *
 * Description:
 *   Register a custom Modbus function code handler for an instance.
 *   Custom handlers are checked after standard handlers, so standard
 *   function codes cannot be overridden.
 *
 * Input Parameters:
 *   h       - The NxModbus instance handle.
 *   fc      - The custom function code to register.
 *   handler - The handler function. Receives the instance handle and
 *             operates on ctx->adu in place: the request fields
 *             (adu.fc, adu.data[], adu.length) are overwritten with the
 *             response. Returns zero on success or a negated errno value
 *             on failure.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int nxmb_register_custom_fc(nxmb_handle_t h, uint8_t fc,
                            nxmb_custom_fc_handler_t handler);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NXMODBUS_NXMODBUS_H */
