/****************************************************************************
 * apps/examples/unity_usrsock/defines.h
 * Common defines for all testcases
 *
 *   Copyright (C) 2015 Haltian Ltd. All rights reserved.
 *   Author: Roman Saveljev <roman.saveljev@haltian.com>
 *           Jussi Kivilinna <jussi.kivilinna@haltian.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#ifndef __EXAMPLES_UNITY_USRSOCK_DEFINES_H
#define __EXAMPLES_UNITY_USRSOCK_DEFINES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define USRSOCK_NODE "/dev/usrsock"

#define USRSOCKTEST_DAEMON_CONF_DEFAULTS \
  { \
    .max_sockets = UINT_MAX, \
    .supported_domain = AF_INET, \
    .supported_type = SOCK_STREAM, \
    .supported_protocol = 0, \
    .delay_all_responses = false, \
    .endpoint_addr = "127.0.0.1", \
    .endpoint_port = 255, \
    .endpoint_block_connect = false, \
    .endpoint_block_send = false, \
    .endpoint_recv_avail_from_start = true, \
    .endpoint_recv_avail = 4, \
  }

/****************************************************************************
 * Public Types
 ****************************************************************************/
struct usrsocktest_daemon_conf_s
{
  unsigned int max_sockets;
  int8_t supported_domain:8;
  int8_t supported_type:8;
  int8_t supported_protocol:8;
  bool delay_all_responses:1;
  bool endpoint_block_connect:1;
  bool endpoint_block_send:1;
  bool endpoint_recv_avail_from_start:1;
  uint8_t endpoint_recv_avail:8;
  const char *endpoint_addr;
  uint16_t endpoint_port;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern int usrsocktest_endp_malloc_cnt;
extern int usrsocktest_dcmd_malloc_cnt;
extern const struct usrsocktest_daemon_conf_s usrsocktest_daemon_defconf;
extern struct usrsocktest_daemon_conf_s usrsocktest_daemon_config;

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
int usrsocktest_daemon_start(const struct usrsocktest_daemon_conf_s *conf);

int usrsocktest_daemon_stop(void);

int usrsocktest_daemon_get_num_active_sockets(void);

int usrsocktest_daemon_get_num_connected_sockets(void);

int usrsocktest_daemon_get_num_waiting_connect_sockets(void);

int usrsocktest_daemon_get_num_recv_empty_sockets(void);

ssize_t usrsocktest_daemon_get_send_bytes(void);

ssize_t usrsocktest_daemon_get_recv_bytes(void);

int usrsocktest_daemon_get_num_unreachable_sockets(void);

int usrsocktest_daemon_get_num_remote_disconnected_sockets(void);

bool usrsocktest_daemon_establish_waiting_connections(void);

bool usrsocktest_send_delayed_command(char cmd, unsigned int delay_msec);

int usrsocktest_daemon_pause_usrsock_handling(bool pause);

#endif /* __EXAMPLES_UNITY_USRSOCK_DEFINES_H */
