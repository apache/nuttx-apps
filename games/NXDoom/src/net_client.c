/****************************************************************************
 * apps/games/NXDoom/src/net_client.c
 *
 * SPDX-License-Identifier: GPLv2
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
 * Network client code
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "d_loop.h"
#include "deh_main.h"
#include "deh_str.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_timer.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_fixed.h"
#include "m_misc.h"
#include "net_client.h"
#include "net_common.h"
#include "net_defs.h"
#include "net_gui.h"
#include "net_io.h"
#include "net_packet.h"
#include "net_query.h"
#include "net_server.h"
#include "net_structrw.h"
#include "w_checksum.h"
#include "w_wad.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define net_cl_expand_ticnum(b) net_expand_tic_num(recvwindow_start, (b))

#define KP 0.1
#define KI 0.01
#define KD 0.02

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  /* waiting for the game to launch */

  CLIENT_STATE_WAITING_LAUNCH,

  /* waiting for the game to start */

  CLIENT_STATE_WAITING_START,

  /* in game */

  CLIENT_STATE_IN_GAME,
} net_clientstate_t;

/* Type of structure used in the receive window */

typedef struct
{
  /* Whether this tic has been received yet */

  boolean active;

  /* Last time we sent a resend request for this tic */

  unsigned int resend_time;

  /* Tic data from server */

  net_full_ticcmd_t cmd;
} net_server_recv_t;

/* Type of structure used in the send window */

typedef struct
{
  /* Whether this slot is active yet */

  boolean active;

  /* The tic number */

  unsigned int seq;

  /* Time the command was generated */

  unsigned int time;

  /* Ticcmd diff */

  net_ticdiff_t cmd;
} net_server_send_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static net_connection_t client_connection;
static net_clientstate_t client_state;
static net_addr_t *server_addr;
static net_context_t *client_context;

/* game settings, as received from the server when the game started */

static net_gamesettings_t settings;

/* The last ticcmd constructed */

static ticcmd_t last_ticcmd;

/* Buffer of ticcmd diffs being sent to the server */

static net_server_send_t send_queue[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

/* Receive window */

static ticcmd_t recvwindow_cmd_base[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];
static int recvwindow_start;
static net_server_recv_t recvwindow[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

/* Whether we need to send an acknowledgement and
 * when gamedata was last received.
 */

static boolean need_to_acknowledge;
static unsigned int gamedata_recv_time;

/* The latency (time between when we sent our command and we got all
 * the other players' commands from the server) for the last tic we
 * received. We include this latency in tics we send to the server so
 * that they can adjust to us.
 */

static int last_latency;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Why did the server reject us? */

char *net_client_reject_reason = NULL;

/* true if the client code is in use */

boolean net_client_connected;

/* true if we have received waiting data from the server,
 * and the wait data that was received.
 */

boolean net_client_received_wait_data;
net_waitdata_t net_client_wait_data;

/* Waiting at the initial wait screen for the game to be launched? */

boolean net_waiting_for_launch = false;

/* Name that we send to the server */

char *net_player_name = NULL;

/* Connected but not participating in the game (observer) */

boolean drone = false;

/* Hash checksums of our wad directory and dehacked data. */

sha1_digest_t net_local_wad_sha1sum;
sha1_digest_t net_local_deh_sha1sum;

/* Are we playing with the freedoom IWAD? */

unsigned int net_local_is_freedoom;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Called when we become disconnected from the server */

static void net_cl_disconnected(void)
{
  d_receive_tic(NULL, NULL);
}

/* Called when a packet is received from the server containing game
 * data. This updates the clock synchronization variable (offsetms)
 * using a PID filter that keeps client clocks in sync.
 */

static void update_clock_sync(unsigned int seq, unsigned int remote_latency)
{
  static int last_error;
  static int cumul_error;
  int latency;
  int error;

  if (seq == send_queue[seq % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].seq)
    {
      latency = i_get_time_ms() -
                send_queue[seq % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].time;
    }
  else if (seq > send_queue[seq % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].seq)
    {
      /* We have received the ticcmd from the server before we have
       * even sent ours
       */

      latency = 0;
    }
  else
    {
      return;
    }

  /* PID filter. These are manually trained parameters. */

  /* How does our latency compare to the worst other player? */

  error = latency - remote_latency;
  cumul_error += error;

  offsetms = KP * (FRACUNIT * error) - KI * (FRACUNIT * cumul_error) +
             (KD * FRACUNIT) * (last_error - error);

  last_error = error;
  last_latency = latency;

  net_log_info(
    "client: latency %d, remote %d -> offset=%dms, cumul_error=%d",
    latency, remote_latency, offsetms / FRACUNIT, cumul_error);
}

/* Expand a net_full_ticcmd_t, applying the diffs in cmd->cmds as
 * patches against recvwindow_cmd_base.  Place the results into
 * the d_net.c structures (netcmds/nettics) and save the new ticcmd
 * back into recvwindow_cmd_base.
 */

static void net_cl_expand_full_ticcmd(net_full_ticcmd_t *cmd,
                                      unsigned int seq, ticcmd_t *ticcmds)
{
  int i;

  /* Expand tic diffs for all players */

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (i == settings.consoleplayer && !drone)
        {
          continue;
        }

      if (cmd->playeringame[i])
        {
          net_ticdiff_t *diff;

          diff = &cmd->cmds[i];

          /* Use the ticcmd diff to patch the previous ticcmd to
           * the new ticcmd
           */

          net_ticcmd_patch(&recvwindow_cmd_base[i], diff, &ticcmds[i]);

          /* Store a copy for next time */

          recvwindow_cmd_base[i] = ticcmds[i];
        }
    }
}

/* Advance the receive window */

static void net_cl_advance_window(void)
{
  ticcmd_t ticcmds[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];

  while (recvwindow[0].active)
    {
      /* Expand tic diff data into d_net.c structures */

      net_cl_expand_full_ticcmd(&recvwindow[0].cmd, recvwindow_start,
                                ticcmds);
      d_receive_tic(ticcmds, recvwindow[0].cmd.playeringame);

      /* Advance the window */

      memmove(recvwindow, recvwindow + 1,
              sizeof(net_server_recv_t) *
                  (CONFIG_GAMES_NXDOOM_NET_BACKUPTICS - 1));
      memset(&recvwindow[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS - 1], 0,
             sizeof(net_server_recv_t));

      ++recvwindow_start;

      net_log_info("client: advanced receive window to %d",
              recvwindow_start);
    }
}

/* Shut down the client code, etc.  Invoked after a disconnect. */

static void net_cl_shutdown(void)
{
  if (net_client_connected)
    {
      net_client_connected = false;

      net_release_address(server_addr);

      /* Shut down network module, etc.  To do. */
    }
}

static void net_cl_send_game_data_ack(void)
{
  net_packet_t *packet;

  packet = net_new_packet(10);

  net_write_int16(packet, NET_PACKET_TYPE_GAMEDATA_ACK);
  net_write_int8(packet, recvwindow_start & 0xff);

  net_conn_send_packet(&client_connection, packet);

  net_free_packet(packet);

  need_to_acknowledge = false;
}

static void net_cl_send_tics(int start, int end)
{
  net_packet_t *packet;
  int i;

  if (!net_client_connected)
    {
      /* Disconnected from server */

      return;
    }

  if (start < 0) start = 0;

  /* Build a new packet to send to the server */

  packet = net_new_packet(512);
  net_write_int16(packet, NET_PACKET_TYPE_GAMEDATA);

  /* Write the start tic and number of tics.  Send only the low byte
   * of start - it can be inferred by the server.
   */

  net_write_int8(packet, recvwindow_start & 0xff);
  net_write_int8(packet, start & 0xff);
  net_write_int8(packet, end - start + 1);

  /* Add the tics. */

  for (i = start; i <= end; ++i)
    {
      net_server_send_t *sendobj;

      sendobj = &send_queue[i % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

      net_write_int16(packet, last_latency);

      net_write_ticcmd_diff(packet, &sendobj->cmd, settings.lowres_turn);
    }

  /* Send the packet */

  net_conn_send_packet(&client_connection, packet);

  /* All done! */

  net_free_packet(packet);

  /* Acknowledgement has been sent as part of the packet */

  need_to_acknowledge = false;
}

/* Parse a SYN packet received back from the server indicating a successful
 * connection attempt.
 */

static void net_cl_parse_syn(net_packet_t *packet)
{
  net_protocol_t protocol;
  char *server_version;

  net_log_info("client: processing SYN response");

  server_version = net_read_safe_string(packet);
  if (server_version == NULL)
    {
      net_log_err("client: failed to read server version");
      return;
    }

  protocol = net_read_protocol(packet);
  if (protocol == NET_PROTOCOL_UNKNOWN)
    {
      net_log_err("client: can't find a common protocol");
      return;
    }

  /* We are now successfully connected. */

  net_log_info("client: connected to server");
  client_connection.state = NET_CONN_STATE_CONNECTED;
  client_connection.protocol = protocol;

  /* Even though we have negotiated a compatible protocol, the game may still
   * desync. Chocolate Doom's philosophy makes this unlikely, but if we're
   * playing with a forked version, or even against a different version that
   * fixes a compatibility issue, we may still have problems.
   */

  if (strcmp(server_version, PACKAGE_STRING) != 0)
    {
      fprintf(stderr,
              "NET_CL_ParseSYN: This is '%s', but the server is "
              "'%s'. It is possible that this mismatch may cause the game "
              "to desync.\n",
              PACKAGE_STRING, server_version);
    }
}

static void set_reject_reason(const char *s)
{
  free(net_client_reject_reason);
  if (s != NULL)
    {
      net_client_reject_reason = strdup(s);
    }
  else
    {
      net_client_reject_reason = NULL;
    }
}

static void net_cl_parse_reject(net_packet_t *packet)
{
  char *msg;

  msg = net_read_safe_string(packet);
  if (msg == NULL)
    {
      return;
    }

  if (client_connection.state == NET_CONN_STATE_CONNECTING)
    {
      client_connection.state = NET_CONN_STATE_DISCONNECTED;
      client_connection.disconnect_reason = NET_DISCONNECT_REMOTE;
      set_reject_reason(msg);
    }
}

/* data received while we are waiting for the game to start */

static void net_cl_parse_waiting_data(net_packet_t *packet)
{
  net_waitdata_t wait_data;

  if (!net_read_wait_data(packet, &wait_data))
    {
      /* Invalid packet? */

      return;
    }

  if (wait_data.num_players > wait_data.max_players ||
      wait_data.ready_players > wait_data.num_players ||
      wait_data.max_players > CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS)
    {
      /* insane data */

      return;
    }

  if ((wait_data.consoleplayer >= 0 && drone) ||
      (wait_data.consoleplayer < 0 && !drone) ||
      (wait_data.consoleplayer >= wait_data.num_players))
    {
      /* Invalid player number */

      return;
    }

  memcpy(&net_client_wait_data, &wait_data, sizeof(net_waitdata_t));
  net_client_received_wait_data = true;
}

static void net_cl_parse_launch(net_packet_t *packet)
{
  unsigned int num_players;

  net_log_info("client: processing launch packet");

  if (client_state != CLIENT_STATE_WAITING_LAUNCH)
    {
      net_log_err("client: not in waiting launch state, client_state=%d",
                  client_state);
      return;
    }

  /* The launch packet contains the number of players that will be
   * in the game when it starts, so that we can do the startup
   * progress indicator (the wait data is unreliable).
   */

  if (!net_read_int8(packet, &num_players))
    {
      net_log_err("client: failed to read number of players");
      return;
    }

  net_client_wait_data.num_players = num_players;
  client_state = CLIENT_STATE_WAITING_START;
  net_log_err("client: now waiting for game start");
}

static void net_cl_parse_game_start(net_packet_t *packet)
{
  net_log_info("client: processing game start packet");

  if (!net_read_settings(packet, &settings))
    {
      net_log_err("client: failed to read settings");
      return;
    }

  if (client_state != CLIENT_STATE_WAITING_START)
    {
      net_log_err("client: not in waiting start state, client_state=%d",
                  client_state);
      return;
    }

  if (settings.num_players > CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS ||
      settings.consoleplayer >= (signed int)settings.num_players)
    {
      /* insane values */

      net_log_err("client: bad settings, num_players=%d, consoleplayer=%d",
                  settings.num_players, settings.consoleplayer);
      return;
    }

  if ((drone && settings.consoleplayer >= 0) ||
      (!drone && settings.consoleplayer < 0))
    {
      /* Invalid player number: must be positive for real players,
       * negative for drones
       */

      net_log_err("client: mismatch: drone=%d, consoleplayer=%d", drone,
                  settings.consoleplayer);
      return;
    }

  net_log_info("client: beginning game state");
  client_state = CLIENT_STATE_IN_GAME;

  /* Clear the receive window */

  memset(recvwindow, 0, sizeof(recvwindow));
  recvwindow_start = 0;
  memset(&recvwindow_cmd_base, 0, sizeof(recvwindow_cmd_base));

  /* Clear the send queue */

  memset(&send_queue, 0x00, sizeof(send_queue));
}

static void net_cl_send_resend_request(int start, int end)
{
  net_packet_t *packet;
  unsigned int nowtime;
  int i;

  packet = net_new_packet(64);
  net_write_int16(packet, NET_PACKET_TYPE_GAMEDATA_RESEND);
  net_write_int32(packet, start);
  net_write_int8(packet, end - start + 1);
  net_conn_send_packet(&client_connection, packet);
  net_free_packet(packet);

  nowtime = i_get_time_ms();

  /* Save the time we sent the resend request */

  for (i = start; i <= end; ++i)
    {
      int index;

      index = i - recvwindow_start;

      if (index < 0 || index >= CONFIG_GAMES_NXDOOM_NET_BACKUPTICS) continue;

      recvwindow[index].resend_time = nowtime;
    }
}

/* Check for expired resend requests */

static void net_cl_check_resends(void)
{
  int i;
  int resend_start;
  int resend_end;
  unsigned int nowtime;
  boolean maybe_deadlocked;

  nowtime = i_get_time_ms();
  maybe_deadlocked = nowtime - gamedata_recv_time > 1000;

  resend_start = -1;
  resend_end = -1;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_BACKUPTICS; ++i)
    {
      net_server_recv_t *recvobj;
      boolean need_resend;

      recvobj = &recvwindow[i];

      /* if need_resend is true, this tic needs another retransmit
       * request (300ms timeout)
       */

      need_resend = !recvobj->active && recvobj->resend_time != 0 &&
                    nowtime > recvobj->resend_time + 300;

      /* if no game data has been received in a long time, we may be in
       * a deadlock scenario where tics from the server have been lost, so
       * we've stopped generating any more, so the server isn't sending us
       * any, so we don't get any to trigger a resend request. So force the
       * first few tics in the receive window to be requested.
       */

      if (i == 0 && !recvobj->active && recvobj->resend_time == 0 &&
          maybe_deadlocked)
        {
          need_resend = true;
        }

      if (need_resend)
        {
          /* Start a new run of resend tics? */

          if (resend_start < 0)
            {
              resend_start = i;
            }

          resend_end = i;
        }
      else if (resend_start >= 0)
        {
          /* End of a run of resend tics */

          net_log_info("client: resend request timed out for %d-%d (%d)",
                       recvwindow_start + resend_start,
                       recvwindow_start + resend_end,
                       recvwindow[resend_start].resend_time);
          net_cl_send_resend_request(recvwindow_start + resend_start,
                                     recvwindow_start + resend_end);
          resend_start = -1;
        }
    }

  if (resend_start >= 0)
    {
      net_log_info("client: resend request timed out for %d-%d (%d)",
                   recvwindow_start + resend_start,
                   recvwindow_start + resend_end,
                   recvwindow[resend_start].resend_time);
      net_cl_send_resend_request(recvwindow_start + resend_start,
                                 recvwindow_start + resend_end);
    }

  /* We have received some data from the server and not acknowledged
   * it yet.  Normally this gets acknowledged when we send our game
   * data, but if the client is a drone we need to do this.
   */

  if (need_to_acknowledge && nowtime - gamedata_recv_time > 200)
    {
      net_log_info("client: no game data received since %d: triggering ack",
                   gamedata_recv_time);
      net_cl_send_game_data_ack();
    }
}

/* Parsing of NET_PACKET_TYPE_GAMEDATA packets
 * (packets containing the actual ticcmd data)
 */

static void net_cl_parse_game_data(net_packet_t *packet)
{
  net_server_recv_t *recvobj;
  unsigned int seq;
  unsigned int num_tics;
  unsigned int nowtime;
  int resend_start;
  int resend_end;
  size_t i;
  int index;

  net_log_info("client: processing game data packet");

  /* Read header */

  if (!net_read_int8(packet, &seq) || !net_read_int8(packet, &num_tics))
    {
      net_log_info("client: error: failed to read header");
      return;
    }

  nowtime = i_get_time_ms();

  /* Whatever happens, we now need to send an acknowledgement of our
   * current receive point.
   */

  if (!need_to_acknowledge)
    {
      need_to_acknowledge = true;
      gamedata_recv_time = nowtime;
    }

  /* Expand byte value into the full tic number */

  seq = net_cl_expand_ticnum(seq);
  net_log_info("client: got game data, seq=%d, num_tics=%d", seq, num_tics);

  for (i = 0; i < num_tics; ++i)
    {
      net_full_ticcmd_t cmd;

      index = seq - recvwindow_start + i;

      if (!net_read_full_ticcmd(packet, &cmd, settings.lowres_turn))
        {
          net_log_err("client: failed to read ticcmd %zu", i);
          return;
        }

      if (index < 0 || index >= CONFIG_GAMES_NXDOOM_NET_BACKUPTICS)
        {
          /* Out of range of the recv window */

          continue;
        }

      /* Store in the receive window */

      recvobj = &recvwindow[index];

      recvobj->active = true;
      recvobj->cmd = cmd;
      net_log_info("client: stored tic %zu in receive window", seq + i);

      /* If a packet is lost or arrives out of order, we might get
       * the tic in the next packet instead (because of extratic).
       * If that's the case then the latency for receiving that tic
       * now will be bogus. So we only use the last tic in the packet
       * to trigger a clock sync update.
       */

      if (i == num_tics - 1)
        {
          update_clock_sync(seq + i, cmd.latency);
        }
    }

  /* Has this been received out of sequence, ie. have we not received
   * all tics before the first tic in this packet?  If so, send a
   * resend request.
   */

  resend_end = seq - recvwindow_start;

  if (resend_end <= 0) return;

  if (resend_end >= CONFIG_GAMES_NXDOOM_NET_BACKUPTICS)
    {
      resend_end = CONFIG_GAMES_NXDOOM_NET_BACKUPTICS - 1;
    }

  index = resend_end - 1;
  resend_start = resend_end;

  while (index >= 0)
    {
      recvobj = &recvwindow[index];

      if (recvobj->active)
        {
          /* ended our run of unreceived tics */

          break;
        }

      if (recvobj->resend_time != 0)
        {
          /* Already sent a resend request for this tic */

          break;
        }

      resend_start = index;
      --index;
    }

  /* Possibly send a resend request */

  if (resend_start < resend_end)
    {
      net_log_info("client: request resend for %d-%d before %d",
                   recvwindow_start + resend_start,
                   recvwindow_start + resend_end - 1, seq);
      net_cl_send_resend_request(recvwindow_start + resend_start,
                                 recvwindow_start + resend_end - 1);
    }
}

/* Parse a resend request from the server due to a dropped packet */

static void net_cl_parse_resend_request(net_packet_t *packet)
{
  static unsigned int start;
  static unsigned int end;
  static unsigned int num_tics;

  net_log_info("client: processing resend request");

  if (drone)
    {
      /* Drones don't send gamedata. */

      net_log_err("client: resend request but we're a drone?");
      return;
    }

  if (!net_read_int32(packet, &start) || !net_read_int8(packet, &num_tics))
    {
      net_log_err("client: couldn't read start and num_tics");
      return;
    }

  end = start + num_tics - 1;

  net_log_info("client: resend request: start=%d, num_tics=%d", start,
               num_tics);

  /* Check we have the tics being requested.  If not, reduce the
   * window of tics to only what we have.
   */

  while (
      start <= end &&
      (!send_queue[start % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].active ||
       send_queue[start % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].seq != start))
    {
      ++start;
    }

  while (start <= end &&
         (!send_queue[end % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].active ||
          send_queue[end % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].seq != end))
    {
      --end;
    }

  /* Resend those tics */

  if (start <= end)
    {
      net_log_info("client: resending %d-%d", start, end);
      net_cl_send_tics(start, end);
    }
  else
    {
      net_log_info("client: don't have the tics to resend");
    }
}

/* Console message that the server wants the client to print */

static void net_cl_parse_console_message(net_packet_t *packet)
{
  char *msg;

  msg = net_read_safe_string(packet);

  if (msg == NULL)
    {
      return;
    }

  printf("Message from server:\n%s\n", msg);
}

/* parse a received packet */

static void net_cl_parse_packet(net_packet_t *packet)
{
  unsigned int packet_type;

  if (!net_read_int16(packet, &packet_type))
    {
      return;
    }

  net_log_info("client: packet from server, type %d",
               packet_type & ~NET_RELIABLE_PACKET);
  net_log_packet(packet);

  if (net_conn_packet(&client_connection, packet, &packet_type))
    {
      /* Packet eaten by the common connection code */
    }
  else
    {
      switch (packet_type)
        {
        case NET_PACKET_TYPE_SYN:
          net_cl_parse_syn(packet);
          break;

        case NET_PACKET_TYPE_REJECTED:
          net_cl_parse_reject(packet);
          break;

        case NET_PACKET_TYPE_WAITING_DATA:
          net_cl_parse_waiting_data(packet);
          break;

        case NET_PACKET_TYPE_LAUNCH:
          net_cl_parse_launch(packet);
          break;

        case NET_PACKET_TYPE_GAMESTART:
          net_cl_parse_game_start(packet);
          break;

        case NET_PACKET_TYPE_GAMEDATA:
          net_cl_parse_game_data(packet);
          break;

        case NET_PACKET_TYPE_GAMEDATA_RESEND:
          net_cl_parse_resend_request(packet);
          break;

        case NET_PACKET_TYPE_CONSOLE_MESSAGE:
          net_cl_parse_console_message(packet);
          break;

        default:
          break;
        }
    }
}

static void net_cl_send_syn(net_connect_data_t *data)
{
  net_packet_t *packet;

  net_log_info("client: sending SYN");

  packet = net_new_packet(10);
  net_write_int16(packet, NET_PACKET_TYPE_SYN);
  net_write_int32(packet, NET_MAGIC_NUMBER);
  net_write_string(packet, PACKAGE_STRING);
  net_write_protocol_list(packet);
  net_write_connect_data(packet, data);
  net_write_string(packet, net_player_name);
  net_conn_send_packet(&client_connection, packet);
  net_free_packet(packet);
}

static void net_cl_init(void)
{
  /* Try to set from the USER and USERNAME environment variables
   * Otherwise, fallback to "Player"
   */

  if (net_player_name == NULL)
    {
      net_player_name = "Player";
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void net_cl_launch_game(void)
{
  net_conn_new_reliable(&client_connection, NET_PACKET_TYPE_LAUNCH);
}

void net_cl_start_game(net_gamesettings_t *p_settings)
{
  net_packet_t *packet;

  /* Start from a ticcmd of all zeros */

  memset(&last_ticcmd, 0, sizeof(ticcmd_t));

  /* Send packet */

  packet =
      net_conn_new_reliable(&client_connection, NET_PACKET_TYPE_GAMESTART);

  net_write_settings(packet, p_settings);
}

/* Add a new ticcmd to the send queue */

void net_cl_send_ticcmd(ticcmd_t *ticcmd, int maketic)
{
  net_ticdiff_t diff;
  net_server_send_t *sendobj;
  int starttic;
  int endtic;

  /* Calculate the difference to the last ticcmd */

  net_ticcmd_diff(&last_ticcmd, ticcmd, &diff);

  /* Store in the send queue */

  sendobj = &send_queue[maketic % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];
  sendobj->active = true;
  sendobj->seq = maketic;
  sendobj->time = i_get_time_ms();
  sendobj->cmd = diff;

  last_ticcmd = *ticcmd;

  /* Send to server. */

  starttic = maketic - settings.extratics;
  endtic = maketic;

  if (starttic < 0) starttic = 0;

  net_log_info("client: generated tic %d, sending %d-%d", maketic, starttic,
               endtic);
  net_cl_send_tics(starttic, endtic);
}

/* "Run" the client code: check for new packets, send packets as
 * needed
 */

void net_cl_run(void)
{
  net_addr_t *addr;
  net_packet_t *packet;

  if (!net_client_connected)
    {
      return;
    }

  while (net_recv_packet(client_context, &addr, &packet))
    {
      /* only accept packets from the server */

      if (addr == server_addr)
        {
          net_cl_parse_packet(packet);
        }

      net_free_packet(packet);
      net_release_address(addr);
    }

  /* Run the common connection code to send any packets as needed */

  net_conn_run(&client_connection);

  if (client_connection.state == NET_CONN_STATE_DISCONNECTED ||
      client_connection.state == NET_CONN_STATE_DISCONNECTED_SLEEP)
    {
      net_cl_disconnected();

      net_cl_shutdown();
    }

  net_waiting_for_launch =
      client_connection.state == NET_CONN_STATE_CONNECTED &&
      client_state == CLIENT_STATE_WAITING_LAUNCH;

  if (client_state == CLIENT_STATE_IN_GAME)
    {
      /* Possibly advance the receive window */

      net_cl_advance_window();

      /* Check if our resend requests have timed out */

      net_cl_check_resends();
    }
}

/* Connect to a server */

boolean net_cl_connect(net_addr_t *addr, net_connect_data_t *data)
{
  int start_time;
  int last_send_time;
  boolean sent_hole_punch;

  server_addr = addr;
  net_reference_address(addr);

  memcpy(net_local_wad_sha1sum, data->wad_sha1sum, sizeof(sha1_digest_t));
  memcpy(net_local_deh_sha1sum, data->deh_sha1sum, sizeof(sha1_digest_t));
  net_local_is_freedoom = data->is_freedoom;

  /* create a new network I/O context and add just the necessary module */

  client_context = net_new_context();

  /* initialize module for client mode */

  if (!addr->module->init_client())
    {
      set_reject_reason("Failed to initialize client module");
      return false;
    }

  net_add_module(client_context, addr->module);

  net_client_connected = true;
  net_client_received_wait_data = false;
  sent_hole_punch = false;

  net_conn_init_client(&client_connection, addr, NET_PROTOCOL_UNKNOWN);

  /* try to connect */

  start_time = i_get_time_ms();
  last_send_time = -1;
  set_reject_reason("Unknown reason");

  while (client_connection.state == NET_CONN_STATE_CONNECTING)
    {
      int nowtime = i_get_time_ms();

      /* Send a SYN packet every second. */

      if (nowtime - last_send_time > 1000 || last_send_time < 0)
        {
          net_cl_send_syn(data);
          last_send_time = nowtime;
        }

      /* time out after 5 seconds */

      if (nowtime - start_time > 5000)
        {
          set_reject_reason("No response from server");
          break;
        }

      if (!sent_hole_punch && nowtime - start_time > 2000)
        {
          net_log_warn("client: no response to SYN, requesting hole punch");
          net_request_hole_punch(client_context, addr);
          sent_hole_punch = true;
        }

      /* run client code */

      net_cl_run();

      /* run the server, just in case we are doing a loopback connect */

      net_sv_run();

      /* Don't hog the CPU */

      usleep(1000);
    }

  if (client_connection.state == NET_CONN_STATE_CONNECTED)
    {
      /* connected ok! */

      net_log_info("client: connected successfully");
      set_reject_reason(NULL);
      client_state = CLIENT_STATE_WAITING_LAUNCH;
      drone = data->drone;

      return true;
    }
  else
    {
      /* failed to connect */

      net_log_err("client: failed to connect");
      net_cl_shutdown();

      return false;
    }
}

/* read game settings received from server */

boolean net_cl_get_settings(net_gamesettings_t *_settings)
{
  if (client_state != CLIENT_STATE_IN_GAME)
    {
      return false;
    }

  memcpy(_settings, &settings, sizeof(net_gamesettings_t));

  return true;
}

/* disconnect from the server */

void net_cl_disconnect(void)
{
  int start_time;

  if (!net_client_connected)
    {
      return;
    }

  net_log_info("client: beginning disconnect");
  net_conn_disconnect(&client_connection);

  start_time = i_get_time_ms();

  while (client_connection.state != NET_CONN_STATE_DISCONNECTED &&
         client_connection.state != NET_CONN_STATE_DISCONNECTED_SLEEP)
    {
      if (i_get_time_ms() - start_time > 5000)
        {
          /* time out after 5 seconds */

          net_log_err("client: no acknowledgement of disconnect received");
          client_state = CLIENT_STATE_WAITING_START;

          fprintf(stderr, "net_cl_disconnect: Timeout while disconnecting "
                          "from server\n");
          break;
        }

      net_cl_run();
      net_sv_run();

      usleep(1000);
    }

  /* Finished sending disconnect packets, etc. */

  net_log_info("client: disconnect complete");
  net_cl_shutdown();
}

void net_init(void)
{
  net_cl_init();
}

void net_bind_variables(void)
{
  m_bind_string_variable("player_name", &net_player_name);
}
