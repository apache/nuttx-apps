/****************************************************************************
 * apps/wireless/lora_pkt_fwd/lora_pkt_fwd.h
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

#ifndef __APPS_WIRELESS_LORA_PKT_FWD_LORA_PKT_FWD_H
#define __APPS_WIRELESS_LORA_PKT_FWD_LORA_PKT_FWD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LORA_FWD_HOSTLEN   64
#define LORA_FWD_EUILEN    8
#define LORA_FWD_IFNAME    "eth0"

/* Semtech UDP protocol version 2 packet types.  Note that the numbering is
 * not sequential: PULL_RESP comes before PULL_ACK.  Swapping those two makes
 * a gateway answer INVALID_JSON to every keepalive.
 */

#define LORA_PKT_PUSH_DATA 0x00
#define LORA_PKT_PUSH_ACK  0x01
#define LORA_PKT_PULL_DATA 0x02
#define LORA_PKT_PULL_RESP 0x03
#define LORA_PKT_PULL_ACK  0x04
#define LORA_PKT_TX_ACK    0x05

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Runtime configuration of the forwarder ***********************************/

struct lora_fwd_config_s
{
  char     server[LORA_FWD_HOSTLEN]; /* Host name or address of the server */
  uint16_t port_up;
  uint16_t port_down;
  uint8_t  eui[LORA_FWD_EUILEN];     /* Gateway identifier */
  int      keepalive_sec;
  int      stat_sec;
};

/* Counters of the forwarder itself.  The radio side counters come from the
 * driver with WLIOC_GW_GETSTATUS.
 */

struct lora_fwd_stats_s
{
  uint32_t rx_fwd;      /* Packets forwarded upstream */
  uint32_t rx_drop;     /* Packets dropped before forwarding */
  uint32_t tx_req;      /* Downlink requests received */
  uint32_t tx_ok;       /* Downlinks handed to the concentrator */
  uint32_t push_sent;
  uint32_t push_ack;
  uint32_t pull_sent;
  uint32_t pull_ack;
  uint32_t uptime_sec;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Daemon control, lora_pkt_fwd.c *******************************************/

void lora_fwd_init(void);
int  lora_fwd_start(void);
int  lora_fwd_stop(void);
bool lora_fwd_isrunning(void);

/* Configuration and statistics.  The daemon runs as its own task and shares
 * this state with the "lora" command through the flat address space, which
 * is why the counters are only visible in a flat build.
 */

FAR struct lora_fwd_config_s *lora_fwd_config(void);
void lora_fwd_getstats(FAR struct lora_fwd_stats_s *stats);
int  lora_fwd_setserver(FAR const char *host, int port_up, int port_down);
int  lora_fwd_parse_eui(FAR const char *str, FAR uint8_t *eui);

#endif /* __APPS_WIRELESS_LORA_PKT_FWD_LORA_PKT_FWD_H */
