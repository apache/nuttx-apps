/****************************************************************************
 * apps/industry/nxmodbus/nxmb_internal.h
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

#ifndef __APPS_INDUSTRY_NXMODBUS_NXMB_INTERNAL_H
#define __APPS_INDUSTRY_NXMODBUS_NXMB_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>
#include <nuttx/config.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <nxmodbus/nxmodbus.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Pre-processor Definitions */

#define NXMB_ADDRESS_BROADCAST 0

/* Modbus function codes */

#define NXMB_FC_READ_COILS         0x01
#define NXMB_FC_READ_DISCRETE      0x02
#define NXMB_FC_READ_HOLDING       0x03
#define NXMB_FC_READ_INPUT         0x04
#define NXMB_FC_WRITE_COIL         0x05
#define NXMB_FC_WRITE_HOLDING      0x06
#define NXMB_FC_DIAGNOSTICS        0x08
#define NXMB_FC_WRITE_COILS        0x0f
#define NXMB_FC_WRITE_HOLDINGS     0x10
#define NXMB_FC_REPORT_SERVER_ID   0x11
#define NXMB_FC_READWRITE_HOLDINGS 0x17

/* Transport operations interface.
 *
 * receive() must not block indefinitely. Implementations should use an
 * internal timeout (e.g. select()) and return 0 or -EAGAIN when no
 * complete frame is available yet. The client polling loop relies on
 * this to enforce its own response timeout.
 */

struct nxmb_transport_ops_s
{
  CODE int (*init)(nxmb_handle_t ctx);
  CODE int (*deinit)(nxmb_handle_t ctx);
  CODE int (*send)(nxmb_handle_t ctx);
  CODE int (*receive)(nxmb_handle_t ctx);
};

/* Custom function code handler */

struct nxmb_custom_fc_s
{
  FAR struct nxmb_custom_fc_s *next;
  nxmb_custom_fc_handler_t     handler;
  uint8_t                      fc;
};

/* NxModbus instance context */

struct nxmb_context_s
{
  FAR const struct nxmb_callbacks_s     *callbacks;
  FAR const struct nxmb_transport_ops_s *transport_ops;
  enum nxmb_mode_e                       mode;
  pthread_mutex_t                        lock;
  uint8_t                                unit_id;
  bool                                   is_client;
  bool                                   enabled;

  /* Transport configuration */

  union nxmb_transport_config_u transport_cfg;

  /* Single ADU container reused for both RX and TX. Modbus is half-duplex,
   * so request and response never coexist: handlers mutate the request
   * in place to form the response.
   */

  struct nxmb_adu_s adu;

  /* Report Server ID (FC17) data */

  uint8_t  server_id_buf[CONFIG_NXMODBUS_REP_SERVER_ID_BUF];
  uint16_t server_id_len;

  /* Custom handlers */

  FAR struct nxmb_custom_fc_s *custom_fc_list;
  FAR void                    *transport_state;
  FAR void                    *client_state;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_NXMODBUS_RTU
extern const struct nxmb_transport_ops_s g_nxmb_serial_ops;
#endif

#ifdef CONFIG_NXMODBUS_ASCII
extern const struct nxmb_transport_ops_s g_nxmb_ascii_ops;
#endif

#ifdef CONFIG_NXMODBUS_RAW_ADU
extern const struct nxmb_transport_ops_s g_nxmb_raw_ops;
#endif

#ifdef CONFIG_NXMODBUS_TCP
extern const struct nxmb_transport_ops_s g_nxmb_tcp_ops;
#endif

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_cb_ret_to_exception
 *
 * Description:
 *   Map a callback return value to a Modbus exception code.
 *
 *   -ENOENT  -> address not supported by the application
 *   anything else -> generic device failure
 *
 ****************************************************************************/

static inline enum nxmb_exception_e nxmb_cb_ret_to_exception(int ret)
{
  if (ret == -ENOENT)
    {
      return NXMB_EX_ILLEGAL_DATA_ADDRESS;
    }

  return NXMB_EX_DEVICE_FAILURE;
}

/****************************************************************************
 * Name: nxmb_util_get_u16_be
 *
 * Description:
 *   Extract a big-endian 16-bit value from a buffer.
 *
 ****************************************************************************/

static inline uint16_t nxmb_util_get_u16_be(FAR const uint8_t *buf)
{
  return (uint16_t)((buf[0] << 8) | buf[1]);
}

/****************************************************************************
 * Name: nxmb_util_put_u16_be
 *
 * Description:
 *   Store a 16-bit value as big-endian in a buffer.
 *
 ****************************************************************************/

static inline void nxmb_util_put_u16_be(FAR uint8_t *buf, uint16_t value)
{
  buf[0] = (uint8_t)(value >> 8);
  buf[1] = (uint8_t)(value & 0xff);
}

/****************************************************************************
 * Name: nxmb_util_clock_ms
 *
 * Description:
 *   Return the current monotonic clock value in milliseconds.
 *
 ****************************************************************************/

static inline uint64_t nxmb_util_clock_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_SERIAL_TERMIOS
/****************************************************************************
 * Name: nxmb_serial_configure
 *
 * Description:
 *   Configure serial port with termios settings for Modbus communication.
 *   Shared by RTU and ASCII transports.
 *
 * Input Parameters:
 *   fd       - File descriptor
 *   baudrate - Baud rate
 *   parity   - Parity setting
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_serial_configure(int fd, uint32_t baudrate,
                          enum nxmb_parity_e parity);
#endif

/****************************************************************************
 *  Name: nxmb_util_set_bits
 *
 *  Description:
 *    Set up to 8 bits in a byte buffer at the given bit offset.
 *
 *  Input Parameters:
 *    buf        - Byte buffer
 *    bit_offset - Bit offset from start of buffer
 *    nbits      - Number of bits to set (max 8)
 *    value      - Bit values to write
 *
 ****************************************************************************/

void nxmb_util_set_bits(FAR uint8_t *buf, uint16_t bit_offset, uint8_t nbits,
                        uint8_t value);

/****************************************************************************
 * Name: nxmb_util_get_bits
 *
 * Description:
 *   Extract up to 8 bits from a byte buffer at the given bit offset.
 *
 * Input Parameters:
 *   buf        - Byte buffer
 *   bit_offset - Bit offset from start of buffer
 *   nbits      - Number of bits to extract (max 8)
 *
 * Returned Value:
 *   Extracted bit values
 *
 ****************************************************************************/

uint8_t nxmb_util_get_bits(FAR const uint8_t *buf, uint16_t bit_offset,
                           uint8_t nbits);

#ifdef CONFIG_NXMODBUS_RTU
/****************************************************************************
 * Name: nxmb_crc16
 *
 * Description:
 *   Calculate Modbus RTU CRC16 checksum.
 *
 * Input Parameters:
 *   buf - Pointer to data buffer
 *   len - Length of data in bytes
 *
 * Returned Value:
 *   CRC16 value
 *
 ****************************************************************************/

uint16_t nxmb_crc16(FAR const uint8_t *buf, uint16_t len);
#endif

#ifdef CONFIG_NXMODBUS_ASCII
/****************************************************************************
 * Name: nxmb_lrc
 *
 * Description:
 *   Calculate Modbus ASCII LRC checksum.
 *
 * Input Parameters:
 *   buf - Pointer to data buffer
 *   len - Length of data in bytes
 *
 * Returned Value:
 *   LRC value (8-bit)
 *
 ****************************************************************************/

uint8_t nxmb_lrc(FAR const uint8_t *buf, uint16_t len);
#endif

#ifdef CONFIG_NXMODBUS_CLIENT
/****************************************************************************
 * Name: nxmb_client_init
 *
 * Description:
 *   Initialize client state for master mode.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_client_init(nxmb_handle_t ctx);

/****************************************************************************
 * Name: nxmb_client_deinit
 *
 * Description:
 *   Deinitialize client state.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_client_deinit(nxmb_handle_t ctx);
#endif

/****************************************************************************
 * Name: nxmb_dispatch_function
 *
 * Description:
 *   Dispatch a Modbus function code to the appropriate handler. Operates
 *   in place on ctx->adu.
 *
 * Input Parameters:
 *   ctx - Instance context
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure
 *
 ****************************************************************************/

int nxmb_dispatch_function(nxmb_handle_t ctx);

/* Function-code handlers operate on ctx->adu in place: they read the
 * request from adu.fc/adu.data[]/adu.length and overwrite those fields
 * with the response.
 */

enum nxmb_exception_e nxmb_fc01_read_coils(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc02_read_discrete(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc05_write_coil(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc15_write_coils(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc03_read_holding(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc04_read_input(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc06_write_holding(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc16_write_holdings(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc23_readwrite_holding(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc08_diagnostics(nxmb_handle_t ctx);
enum nxmb_exception_e nxmb_fc17_report_server_id(nxmb_handle_t ctx);

#endif /* __APPS_INDUSTRY_NXMODBUS_NXMB_INTERNAL_H */
