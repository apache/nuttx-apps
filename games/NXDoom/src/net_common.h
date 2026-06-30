/****************************************************************************
 * apps/games/NXDoom/src/net_common.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Common code shared between the client and server
 *
 ****************************************************************************/

#ifndef NET_COMMON_H
#define NET_COMMON_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>

#include "d_mode.h"
#include "net_defs.h"
#include "net_packet.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_RETRIES 5

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum
{
  /* Client has sent a SYN, is waiting for a SYN in response. */

  NET_CONN_STATE_CONNECTING,

  /* Successfully connected. */

  NET_CONN_STATE_CONNECTED,

  /* Sent a DISCONNECT packet, waiting for a DISCONNECT_ACK reply */

  NET_CONN_STATE_DISCONNECTING,

  /* Client successfully disconnected */

  NET_CONN_STATE_DISCONNECTED,

  /* We are disconnected, but in a sleep state, waiting for several
   * seconds.  This is in case the DISCONNECT_ACK we sent failed
   * to arrive, and we need to send another one.  We keep this as
   * a valid connection for a few seconds until we are sure that
   * the other end has successfully disconnected as well.
   */

  NET_CONN_STATE_DISCONNECTED_SLEEP,
} net_connstate_t;

/* Reason a connection was terminated */

typedef enum
{
  /* As the result of a local disconnect request */

  NET_DISCONNECT_LOCAL,

  /* As the result of a remote disconnect request */

  NET_DISCONNECT_REMOTE,

  /* Timeout (no data received in a long time) */

  NET_DISCONNECT_TIMEOUT,
} net_disconnect_reason_t;

typedef struct net_reliable_packet_s net_reliable_packet_t;

typedef struct
{
  net_connstate_t state;
  net_disconnect_reason_t disconnect_reason;
  net_addr_t *addr;
  net_protocol_t protocol;
  int last_send_time;
  int num_retries;
  int keepalive_send_time;
  int keepalive_recv_time;
  net_reliable_packet_t *reliable_packets;
  int reliable_send_seq;
  int reliable_recv_seq;
} net_connection_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void net_conn_send_packet(net_connection_t *conn, net_packet_t *packet);
void net_conn_init_client(net_connection_t *conn, net_addr_t *addr,
                          net_protocol_t protocol);
void net_conn_init_server(net_connection_t *conn, net_addr_t *addr,
                         net_protocol_t protocol);
boolean net_conn_packet(net_connection_t *conn, net_packet_t *packet,
                        unsigned int *packet_type);
void net_conn_disconnect(net_connection_t *conn);
void net_conn_run(net_connection_t *conn);
net_packet_t *net_conn_new_reliable(net_connection_t *conn, int packet_type);

/* Other miscellaneous common functions */

unsigned int net_expand_tic_num(unsigned int relative, unsigned int b);
boolean net_valid_game_settings(game_mode_t mode, gamemission_t mission,
                              net_gamesettings_t *settings);

/* Conditional logging */

#ifdef CONFIG_GAMES_NXDOOM_NET_LOGS
#define net_log_info(fmt, ...) syslog(LOG_USER | LOG_INFO, fmt, ##__VA_ARGS__)
#define net_log_warn(fmt, ...)                                               \
  syslog(LOG_USER | LOG_WARNING, fmt, ##__VA_ARGS__)
#define net_log_err(fmt, ...) syslog(LOG_USER | LOG_ERR, fmt, ##__VA_ARGS__)

void net_log_packet(net_packet_t *packet);
#else
#define net_log_info(fmt, ...)
#define net_log_warn(fmt, ...)
#define net_log_err(fmt, ...)

#define net_log_packet(pkt)
#endif /* CONFIG_GAMES_NXDOOM_NET_LOGS */

#endif /* NET_COMMON_H */
