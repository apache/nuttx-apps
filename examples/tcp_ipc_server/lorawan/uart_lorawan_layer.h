/****************************************************************************
 * apps/examples/tcp_ipc_server/lorawan/uart_lorawan_layer.h
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

#ifndef __APPS_EXAMPLES_TCP_IPC_SERVER_LORAWAN_H
#define __APPS_EXAMPLES_TCP_IPC_SERVER_LORAWAN_H

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define APP_SESSION_KEY_SIZE            60
#define NW_SESSION_KEY_SIZE             60
#define APP_EUI_SIZE                    30
#define DEVICE_ADDRESS_SIZE             15
#define CHANNEL_MASK_SIZE               35
#define ERR_AT_BUSY_ERROR               -1

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
    char application_session_key[APP_SESSION_KEY_SIZE];
    char network_session_key[NW_SESSION_KEY_SIZE];
    char application_eui[APP_EUI_SIZE];
    char device_address[DEVICE_ADDRESS_SIZE];
    char channel_mask[CHANNEL_MASK_SIZE];
} config_lorawan_radioenge_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void lorawan_radioenge_init(config_lorawan_radioenge_t config_lorawan);
int lorawan_radioenge_send_msg(unsigned char * pt_payload_uplink_hexstring,
                               int size_uplink,
                               unsigned char * pt_payload_downlink_hexstring,
                               int max_size_downlink,
                               int time_to_wait_ms);

#endif /* __APPS_EXAMPLES_TCP_IPC_SERVER_LORAWAN_H */
