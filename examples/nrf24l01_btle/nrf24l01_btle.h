/****************************************************************************
 * apps/examples/nrf24l01_btle/nrf24l01_btle.h
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

#ifndef __EXAMPLES_NRF24L01_BTLE_H
#define __EXAMPLES_NRF24L01_BTLE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Service UUIDs used on the nRF8001 and nRF51822 platforms */

#define NRF_TEMPERATURE_SERVICE_UUID		0x1809
#define NRF_ENVIRONMENTAL_SERVICE_UUID      0x181A

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* helper struct for sending temperature as BT service data */

struct nrf_service_data
  {
    int16_t   service_uuid;
    uint8_t   value;
  };

/* advertisement PDU */

struct btle_adv_pdu
  {
    /* PDU type, most of it 0x42  */

    uint8_t pdu_type;

    /* payload size */

    uint8_t pl_size;

    /* MAC address */

    uint8_t mac[6];

    /* payload (including 3 bytes for CRC) */

    uint8_t payload[24];
  };

/* payload chunk in advertisement PDU payload */

struct btle_pdu_chunk
  {
    uint8_t size;
    uint8_t type;
    uint8_t data[];
  };

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* helper macro to access chunk at specific offset */

#define chunk(x, y) ((struct btle_pdu_chunk *)(x.payload+y))

int nrf24_cfg(int fd);

int nrf24_open(void);

int nrf24_send(int wl_fd, uint8_t * buf, uint8_t len);

#endif /* __EXAMPLES_NRF24L01_BTLE_H  */
