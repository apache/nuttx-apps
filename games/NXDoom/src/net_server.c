/****************************************************************************
 * apps/games/NXDoom/src/net_server.c
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
 * Network server code
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "d_mode.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_timer.h"
#include "m_argv.h"
#include "m_misc.h"

#include "net_client.h"
#include "net_common.h"
#include "net_defs.h"
#include "net_io.h"
#include "net_loop.h"
#include "net_packet.h"
#include "net_query.h"
#include "net_sdl.h"
#include "net_server.h"
#include "net_structrw.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* How often to refresh our registration with the master server. */

#define MASTER_REFRESH_PERIOD 30 /* twice per minute */

/* How often to re-resolve the address of the master server? */

#define MASTER_RESOLVE_PERIOD 8 * 60 * 60 /* 8 hours */

#define net_sv_expand_ticnum(b) net_expand_tic_num(recvwindow_start, (b))

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  /* waiting for the game to be "launched" (key player to press the start
   * button)
   */

  SERVER_WAITING_LAUNCH,

  /* game has been launched, we are waiting for all players to be ready
   * so the game can start.
   */

  SERVER_WAITING_START,

  /* in a game */

  SERVER_IN_GAME,
} net_server_state_t;

typedef struct
{
  boolean active;
  int player_number;
  net_addr_t *addr;
  net_connection_t connection;
  int last_send_time;
  char *name;

  /* If true, the client has sent the NET_PACKET_TYPE_GAMESTART
   * message indicating that it is ready for the game to start.
   */

  boolean ready;

  /* Time that this client connected to the server.
   * This is used to determine the controller (oldest client).
   */

  unsigned int connect_time;

  /* Last time new gamedata was received from this client */

  int last_gamedata_time;

  /* recording a demo without -longtics */

  boolean recording_lowres;

  /* send queue: items to send to the client
   * this is a circular buffer
   */

  int sendseq;
  net_full_ticcmd_t sendqueue[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

  /* Latest acknowledged by the client */

  unsigned int acknowledged;

  /* Value of max_players specified by the client on connect. */

  int max_players;

  /* Observer: receives data but does not participate in the game. */

  boolean drone;

  /* SHA1 hash sums of the client's WAD directory and dehacked data */

  sha1_digest_t wad_sha1sum;
  sha1_digest_t deh_sha1sum;

  /* Is this client is playing with the Freedoom IWAD? */

  unsigned int is_freedoom;

  /* Player class (for Hexen) */

  int player_class;
} net_client_t;

/* structure used for the recv window */

typedef struct
{
  /* Whether this tic has been received yet */

  boolean active;

  /* Latency value received from the client */

  signed int latency;

  /* Last time we sent a resend request for this tic */

  unsigned int resend_time;

  /* Tic data itself */

  net_ticdiff_t diff;
} net_client_recv_t;

typedef enum
{
  RANGE_LOCALHOST, /* Same process or 127.x */
  RANGE_PRIVATE,   /* RFC 1918 */
  RANGE_PUBLIC,    /* The public Internet */
} ip_range_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static net_server_state_t server_state;
static boolean server_initialized = false;
static net_client_t clients[MAXNETNODES];
static net_client_t *sv_players[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];
static net_context_t *server_context;
static unsigned int sv_gamemode;
static unsigned int sv_gamemission;
static net_gamesettings_t sv_settings;

/* For registration with master server: */

static net_addr_t *master_server = NULL;
static unsigned int master_refresh_time;
static unsigned int master_resolve_time;

/* receive window */

static unsigned int recvwindow_start;
static net_client_recv_t recvwindow[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS]
                                   [CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void net_sv_send_console_message(net_client_t *client, const char *s,
                                        ...) PRINTF_ATTR(2, 3);
static void net_sv_broadcast_message(const char *s, ...) PRINTF_ATTR(1, 2);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void net_sv_disconnect_client(net_client_t *client)
{
  if (client->active)
    {
      net_conn_disconnect(&client->connection);
    }
}

static boolean client_connected(net_client_t *client)
{
  /* Check that the client is properly connected: ie. not in the
   * process of connecting or disconnecting
   */

  return client->active &&
         client->connection.state == NET_CONN_STATE_CONNECTED;
}

/* Send a message to be displayed on a client's console */

static void net_sv_send_console_message(net_client_t *client, const char *s,
                                        ...)
{
  char buf[1024];
  va_list args;
  net_packet_t *packet;

  va_start(args, s);
  vsnprintf(buf, sizeof(buf), s, args);
  va_end(args);

  packet = net_conn_new_reliable(&client->connection,
                                NET_PACKET_TYPE_CONSOLE_MESSAGE);

  net_write_string(packet, buf);
}

/* Send a message to all clients */

static void net_sv_broadcast_message(const char *s, ...)
{
  char buf[1024];
  va_list args;
  int i;

  va_start(args, s);
  vsnprintf(buf, sizeof(buf), s, args);
  va_end(args);

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]))
        {
          net_sv_send_console_message(&clients[i], "%s", buf);
        }
    }

  printf("%s\n", buf);
}

/* Assign player numbers to connected clients */

static void net_sv_assign_players(void)
{
  int i;
  int pl;

  pl = 0;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]))
        {
          if (!clients[i].drone)
            {
              sv_players[pl] = &clients[i];
              sv_players[pl]->player_number = pl;
              ++pl;
            }
          else
            {
              clients[i].player_number = -1;
            }
        }
    }

  for (; pl < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++pl)
    {
      sv_players[pl] = NULL;
    }
}

/* Returns the number of players currently connected. */

static int net_sv_num_players(void)
{
  int i;
  int result;

  result = 0;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (sv_players[i] != NULL && client_connected(sv_players[i]))
        {
          result += 1;
        }
    }

  return result;
}

/* Returns the number of players ready to start the game. */

static int net_sv_num_ready_players(void)
{
  int result = 0;
  int i;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]) && !clients[i].drone &&
          clients[i].ready)
        {
          ++result;
        }
    }

  return result;
}

/* Returns the maximum number of players that can play. */

static int net_sv_max_players(void)
{
  int i;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]))
        {
          return clients[i].max_players;
        }
    }

  return CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS;
}

/* Returns the number of drones currently connected. */

static int net_sv_num_drones(void)
{
  int i;
  int result;

  result = 0;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]) && clients[i].drone)
        {
          result += 1;
        }
    }

  return result;
}

/* returns the number of clients connected */

static int net_sv_num_clients(void)
{
  int count;
  int i;

  count = 0;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]))
        {
          ++count;
        }
    }

  return count;
}

/* returns a pointer to the client which controls the server */

static net_client_t *net_sv_controller(void)
{
  net_client_t *best;
  int i;

  /* Find the oldest client (first to connect). */

  best = NULL;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      /* Can't be controller? */

      if (!client_connected(&clients[i]) || clients[i].drone)
        {
          continue;
        }

      if (best == NULL || clients[i].connect_time < best->connect_time)
        {
          best = &clients[i];
        }
    }

  return best;
}

static ip_range_t client_address_range(const char *addr)
{
  if (!strcmp(addr, "local client") || m_string_starts_with(addr, "127."))
    {
      return RANGE_LOCALHOST;
    }

  if (m_string_starts_with(addr, "10.") ||
      m_string_starts_with(addr, "192.168."))
    {
      return RANGE_PRIVATE;
    }

  return RANGE_PUBLIC;
}

static void net_sv_send_waiting_data(net_client_t *client)
{
  net_waitdata_t wait_data;
  net_packet_t *packet;
  net_client_t *controller;
  ip_range_t client_range, player_range;
  const char *addr;
  int i;

  net_sv_assign_players();

  controller = net_sv_controller();

  wait_data.num_players = net_sv_num_players();
  wait_data.num_drones = net_sv_num_drones();
  wait_data.ready_players = net_sv_num_ready_players();
  wait_data.max_players = net_sv_max_players();
  wait_data.is_controller = (client == controller);
  wait_data.consoleplayer = client->player_number;

  /* Send the WAD and dehacked checksums of the controlling client.
   * If no controller found (?), send the details that the client
   * is expecting anyway.
   */

  if (controller == NULL)
    {
      controller = client;
    }

  memcpy(&wait_data.wad_sha1sum, &controller->wad_sha1sum,
         sizeof(sha1_digest_t));
  memcpy(&wait_data.deh_sha1sum, &controller->deh_sha1sum,
         sizeof(sha1_digest_t));
  wait_data.is_freedoom = controller->is_freedoom;

  /* We only send IP addresses to locally-connected clients (including
   * the 127.* loopback range):
   */

  addr = net_addr_to_string(client->connection.addr);
  client_range = client_address_range(addr);

  /* set name and address of each player: */

  for (i = 0; i < wait_data.num_players; ++i)
    {
      m_str_copy(wait_data.player_names[i], sv_players[i]->name,
                 CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME);

      /* For privacy, only local clients or those on a LAN get to see
       * addresses. Public clients only get to see their own address,
       * though we do reveal localhost addresses since they're harmless,
       * and we do reveal when a client is connected via LAN.
       */

      addr = net_addr_to_string(sv_players[i]->addr);
      player_range = client_address_range(addr);
      if (client_range == RANGE_LOCALHOST || client_range == RANGE_PRIVATE ||
          i == wait_data.consoleplayer || player_range == RANGE_LOCALHOST)
        {
          m_str_copy(wait_data.player_addrs[i], addr,
                     CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME);
        }
      else if (player_range == RANGE_PRIVATE)
        {
          snprintf(wait_data.player_addrs[i],
                   CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME,
                   "[LAN player]");
        }
      else
        {
          snprintf(wait_data.player_addrs[i],
                   CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME,
                   "[address hidden]");
        }
    }

  /* Construct packet: */

  packet = net_new_packet(10);
  net_write_int16(packet, NET_PACKET_TYPE_WAITING_DATA);
  net_write_wait_data(packet, &wait_data);

  /* Send packet to client and free */

  net_conn_send_packet(&client->connection, packet);
  net_free_packet(packet);
}

/* Find the latest tic which has been acknowledged as received by
 * all clients.
 */

static unsigned int net_sv_latest_acknowledged(void)
{
  unsigned int lowtic = UINT_MAX;
  int i;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]))
        {
          if (clients[i].acknowledged < lowtic)
            {
              lowtic = clients[i].acknowledged;
            }
        }
    }

  return lowtic;
}

/* Possibly advance the recv window if all connected clients have
 * used the data in the window
 */

static void net_sv_advance_window(void)
{
  unsigned int lowtic;
  int i;

  if (net_sv_num_players() <= 0)
    {
      return;
    }

  lowtic = net_sv_latest_acknowledged();

  /* Advance the recv window until it catches up with lowtic */

  while (recvwindow_start < lowtic)
    {
      boolean should_advance;

      /* Check we have tics from all players for first tic in
       * the recv window
       */

      should_advance = true;

      for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
        {
          if (sv_players[i] == NULL || !client_connected(sv_players[i]))
            {
              continue;
            }

          if (!recvwindow[0][i].active)
            {
              should_advance = false;
              break;
            }
        }

      if (!should_advance)
        {
          /* The first tic is not complete: ie. we have not
           * received tics from all connected players. This can
           * happen if only one player is in the game.
           */

          break;
        }

      /* Advance the window */

      memmove(recvwindow, recvwindow + 1,
              sizeof(*recvwindow) *
              (CONFIG_GAMES_NXDOOM_NET_BACKUPTICS - 1));
      memset(&recvwindow[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS - 1],
             0, sizeof(*recvwindow));
      ++recvwindow_start;
      net_log_info("server: advanced receive window to %d",
              recvwindow_start);
    }
}

/* Given an address, find the corresponding client */

static net_client_t *net_sv_find_client(net_addr_t *addr)
{
  int i;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (clients[i].active && clients[i].addr == addr)
        {
          /* found the client */

          return &clients[i];
        }
    }

  return NULL;
}

/* send a rejection packet to a client */

static void net_sv_send_reject(net_addr_t *addr, const char *msg)
{
  net_packet_t *packet;

  net_log_info("server: sending reject to %s", net_addr_to_string(addr));

  packet = net_new_packet(10);
  net_write_int16(packet, NET_PACKET_TYPE_REJECTED);
  net_write_string(packet, msg);
  net_send_packet(addr, packet);
  net_free_packet(packet);
}

static void net_sv_init_new_client(net_client_t *client, net_addr_t *addr,
                                   net_protocol_t protocol)
{
  client->active = true;
  client->connect_time = i_get_time_ms();
  net_conn_init_server(&client->connection, addr, protocol);
  client->addr = addr;
  net_reference_address(addr);
  client->last_send_time = -1;

  /* init the ticcmd send queue */

  client->sendseq = 0;
  client->acknowledged = 0;
  client->drone = false;
  client->ready = false;

  client->last_gamedata_time = 0;

  memset(client->sendqueue, 0xff, sizeof(client->sendqueue));

  net_log_info("server: initialized new client from %s",
               net_addr_to_string(addr));
}

/* parse a SYN from a client(initiating a connection) */

static void net_sv_parse_syn(net_packet_t *packet, net_client_t *client,
                             net_addr_t *addr)
{
  unsigned int magic;
  net_connect_data_t data;
  net_packet_t *reply;
  net_protocol_t protocol;
  char *player_name;
  char *client_version;
  int num_players;
  int i;

  net_log_info("server: processing SYN packet");

  /* Read the magic number and check it is the expected one. */

  if (!net_read_int32(packet, &magic))
    {
      net_log_err("server: no magic number for SYN");
      return;
    }

  switch (magic)
    {
    case NET_MAGIC_NUMBER:
      break;

    case NET_OLD_MAGIC_NUMBER:
      net_log_err("server: client using old magic number: %d", magic);
      net_sv_send_reject(
          addr,
          "You are using an old client version that is not supported by "
          "this server. This server is running " PACKAGE_STRING ".");
      return;

    default:
      net_log_err("server: wrong magic number: %d", magic);
      return;
    }

  /* Read the client version string. We actually now only use this when
   * sending a reject message, as we only reject if we can't negotiate a
   * common protocol (below).
   */

  client_version = net_read_string(packet);
  if (client_version == NULL)
    {
      net_log_err("server: no version from client");
      return;
    }

  /* Read the client's list of accepted protocols. Net play between forks
   * of Chocolate Doom is accepted provided that they can negotiate a
   * common accepted protocol.
   */

  protocol = net_read_protocol_list(packet);
  if (protocol == NET_PROTOCOL_UNKNOWN)
    {
      char reject_msg[256];

      snprintf(reject_msg, sizeof(reject_msg),
               "Version mismatch: server version is: " PACKAGE_STRING "; "
               "client is: %s. No common compatible protocol could be "
               "negotiated.",
               client_version);
      net_sv_send_reject(addr, reject_msg);
      net_log_err("server: no common protocol");
      return;
    }

  /* Read connect data, and check that the game mode/mission are valid
   * and the max_players value is in a sensible range.
   */

  if (!net_read_connect_data(packet, &data))
    {
      net_log_err("server: failed to read connect data");
      return;
    }

  if (!d_valid_game_mode(data.gamemission, data.gamemode) ||
      data.max_players > CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS)
    {
      net_log_err("server: invalid connect data, max_players=%d, "
                  "gamemission=%d, gamemode=%d",
                  data.max_players, data.gamemission, data.gamemode);
      return;
    }

  /* Read the player's name */

  player_name = net_read_string(packet);
  if (player_name == NULL)
    {
      net_log_err("server: failed to read player name");
      return;
    }

  /* At this point we have received a valid SYN. */

  /* Not accepting new connections? */

  if (server_state != SERVER_WAITING_LAUNCH)
    {
      net_log_err("server: not in waiting launch state, server_state=%d",
                  server_state);
      net_sv_send_reject(addr,
                         "Server is not currently accepting connections");
      return;
    }

  /* Before accepting a new client, check that there is a slot free. */

  net_sv_assign_players();
  num_players = net_sv_num_players();

  if ((!data.drone && num_players >= net_sv_max_players()) ||
      net_sv_num_clients() >= MAXNETNODES)
    {
      net_log_info("server: no more players, num_players=%d, max=%d",
                   num_players, net_sv_max_players());
      net_sv_send_reject(addr, "Server is full!");
      return;
    }

  /* TODO: Add server option to allow rejecting clients which set
   * lowres_turn. This is potentially desirable as the presence of such
   * clients affects turning resolution.
   */

  /* Adopt the game mode and mission of the first connecting client: */

  if (num_players == 0 && !data.drone)
    {
      sv_gamemode = data.gamemode;
      sv_gamemission = data.gamemission;
      net_log_info("server: new game, mode=%d, mission=%d", sv_gamemode,
                   sv_gamemission);
    }

  /* Check the connecting client is playing the same game as all
   * the other clients
   */

  if (data.gamemode != sv_gamemode || data.gamemission != sv_gamemission)
    {
      char msg[128];
      net_log_warn("server: wrong mode/mission, %d != %d || %d != %d",
                   data.gamemode, sv_gamemode, data.gamemission,
                   sv_gamemission);
      snprintf(msg, sizeof(msg),
               "Game mismatch: server is %s (%s), client is %s (%s)",
               d_game_mission_string(sv_gamemission),
               d_game_mode_string(sv_gamemode),
               d_game_mission_string(data.gamemission),
               d_game_mode_string(data.gamemode));

      net_sv_send_reject(addr, msg);
      return;
    }

  /* Allocate a client slot if there isn't one already */

  if (client == NULL)
    {
      /* find a slot, or return if none found */

      for (i = 0; i < MAXNETNODES; ++i)
        {
          if (!clients[i].active)
            {
              client = &clients[i];
              break;
            }
        }

      if (client == NULL)
        {
          return;
        }
    }
  else
    {
      /* If this is a recently-disconnected client, deactivate
       * to allow immediate reconnection
       */

      if (client->connection.state == NET_CONN_STATE_DISCONNECTED)
        {
          client->active = false;
        }
    }

  /* Client already connected? */

  if (client->active)
    {
      net_log_warn("server: client is already initialized (duplicate SYN?)");
      return;
    }

  /* Activate, initialize connection */

  net_sv_init_new_client(client, addr, protocol);

  /* Save the SHA1 checksums and other details. */

  memcpy(client->wad_sha1sum, data.wad_sha1sum, sizeof(sha1_digest_t));
  memcpy(client->deh_sha1sum, data.deh_sha1sum, sizeof(sha1_digest_t));
  client->is_freedoom = data.is_freedoom;
  client->max_players = data.max_players;
  client->name = m_string_duplicate(player_name);
  client->recording_lowres = data.lowres_turn;
  client->drone = data.drone;
  client->player_class = data.player_class;

  /* Send a reply back to the client, indicating a successful connection
   * and specifying the protocol that will be used for communications.
   */

  reply = net_conn_new_reliable(&client->connection, NET_PACKET_TYPE_SYN);
  net_write_string(reply, PACKAGE_STRING);
  net_write_protocol(reply, protocol);
}

/* Parse a launch packet. This is sent by the key player when the "start"
 * button is pressed, and causes the startup process to continue.
 */

static void net_sv_parse_launch(net_packet_t *packet, net_client_t *client)
{
  net_packet_t *launchpacket;
  int num_players;
  unsigned int i;

  net_log_info("server: processing launch packet");

  /* Only the controller can launch the game. */

  if (client != net_sv_controller())
    {
      net_log_err("server: this client isn't the controller, %p != %p",
                  client, net_sv_controller());
      return;
    }

  /* Can only launch when we are in the waiting state. */

  if (server_state != SERVER_WAITING_LAUNCH)
    {
      net_log_err("server: not in waiting launch state, state=%d",
                  server_state);
      return;
    }

  /* Forward launch on to all clients. */

  net_log_info("server: sending launch to all clients");
  net_sv_assign_players();
  num_players = net_sv_num_players();

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (!client_connected(&clients[i])) continue;

      launchpacket = net_conn_new_reliable(&clients[i].connection,
                                          NET_PACKET_TYPE_LAUNCH);
      net_write_int8(launchpacket, num_players);
    }

  /* Now in launch state. */

  server_state = SERVER_WAITING_START;
}

/* Transition to the in-game state and send all players the start game
 * message. Invoked once all players have indicated they are ready to
 * start the game.
 */

static void start_game(void)
{
  net_packet_t *startpacket;
  unsigned int i;
  int nowtime;

  /* Assign player numbers */

  net_sv_assign_players();

  /* Check if anyone is recording a demo and set lowres_turn if so. */

  sv_settings.lowres_turn = false;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (sv_players[i] != NULL && sv_players[i]->recording_lowres)
        {
          sv_settings.lowres_turn = true;
        }
    }

  sv_settings.num_players = net_sv_num_players();

  /* Copy player classes: */

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (sv_players[i] != NULL)
        {
          sv_settings.player_classes[i] = sv_players[i]->player_class;
        }
      else
        {
          sv_settings.player_classes[i] = 0;
        }
    }

  nowtime = i_get_time_ms();

  /* Send start packets to each connected node */

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (!client_connected(&clients[i])) continue;

      clients[i].last_gamedata_time = nowtime;

      startpacket = net_conn_new_reliable(&clients[i].connection,
                                         NET_PACKET_TYPE_GAMESTART);

      sv_settings.consoleplayer = clients[i].player_number;

      net_write_settings(startpacket, &sv_settings);
    }

  /* Change server state */

  net_log_info("server: beginning game state");
  server_state = SERVER_IN_GAME;

  memset(recvwindow, 0, sizeof(recvwindow));
  recvwindow_start = 0;
}

/* Returns true when all nodes have indicated readiness to start the game. */

static boolean all_nodes_ready(void)
{
  unsigned int i;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]) && !clients[i].ready)
        {
          return false;
        }
    }

  return true;
}

/* Check if the game should start, and if so, start it. */

static void check_start_game(void)
{
  if (!all_nodes_ready())
    {
      net_log_warn("server: not all clients ready to start yet");
      return;
    }

  net_log_info("server: all clients ready, starting game");
  start_game();
}

/* Send waiting data with current status to all nodes that are ready to
 * start the game.
 */

static void send_all_waiting_data(void)
{
  unsigned int i;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (client_connected(&clients[i]) && clients[i].ready)
        {
          net_sv_send_waiting_data(&clients[i]);
        }
    }
}

/* Parse a game start packet */

static void net_sv_parse_game_start(net_packet_t *packet,
                                    net_client_t *client)
{
  net_gamesettings_t settings;

  net_log_info("server: processing game start packet");

  /* Can only start a game if we are in the waiting start state. */

  if (server_state != SERVER_WAITING_START)
    {
      net_log_info(
          "server: error: not in waiting start state, server_state=%d",
          server_state);
      return;
    }

  if (client == net_sv_controller())
    {
      if (!net_read_settings(packet, &settings))
        {
          /* Malformed packet */

          net_log_err("server: no settings from controller");
          return;
        }

      /* Check the game settings are valid */

      if (!net_valid_game_settings(sv_gamemode, sv_gamemission, &settings))
        {
          net_log_err("server: invalid game settings");
          return;
        }

      sv_settings = settings;
    }

  client->ready = true;

  check_start_game();

  /* Update all ready clients with the current state (number of players
   * ready, etc.). This is used by games that show startup progress
   * (eg. Hexen's spinal loading)
   */

  send_all_waiting_data();
}

/* Send a resend request to a client */

static void net_sv_send_resend_request(net_client_t *client, int start,
                                       int end)
{
  net_packet_t *packet;
  net_client_recv_t *recvobj;
  int i;
  unsigned int nowtime;
  int index;

  net_log_info("server: send resend to %s for tics %d-%d",
               net_addr_to_string(client->addr), start, end);

  packet = net_new_packet(20);

  net_write_int16(packet, NET_PACKET_TYPE_GAMEDATA_RESEND);
  net_write_int32(packet, start);
  net_write_int8(packet, end - start + 1);

  net_conn_send_packet(&client->connection, packet);
  net_free_packet(packet);

  /* Store the time we send the resend request */

  nowtime = i_get_time_ms();

  for (i = start; i <= end; ++i)
    {
      index = i - recvwindow_start;

      if (index >= CONFIG_GAMES_NXDOOM_NET_BACKUPTICS)
        {
          /* Outside the range */

          continue;
        }

      recvobj = &recvwindow[index][client->player_number];

      recvobj->resend_time = nowtime;
    }
}

/* Check for expired resend requests */

static void net_sv_check_resends(net_client_t *client)
{
  int i;
  int player;
  int resend_start;
  int resend_end;
  unsigned int nowtime;

  nowtime = i_get_time_ms();

  player = client->player_number;
  resend_start = -1;
  resend_end = -1;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_BACKUPTICS; ++i)
    {
      net_client_recv_t *recvobj;
      boolean need_resend;

      recvobj = &recvwindow[i][player];

      /* if need_resend is true, this tic needs another retransmit
       * request (300ms timeout)
       */

      need_resend = !recvobj->active && recvobj->resend_time != 0 &&
                    nowtime > recvobj->resend_time + 300;

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

          net_log_warn(
              "server: resend request to %s timed out for %d-%d (%p)",
              net_addr_to_string(client->addr),
              recvwindow_start + resend_start,
              recvwindow_start + resend_end,
              &recvwindow[resend_start][player].resend_time);

          net_sv_send_resend_request(client, recvwindow_start + resend_start,
                                     recvwindow_start + resend_end);

          resend_start = -1;
        }
    }

  if (resend_start >= 0)
    {
      net_log_warn("server: resend request to %s timed out for %d-%d (%p)",
                   net_addr_to_string(client->addr),
                   recvwindow_start + resend_start,
                   recvwindow_start + resend_end,
                   &recvwindow[resend_start][player].resend_time);
      net_sv_send_resend_request(client, recvwindow_start + resend_start,
                                 recvwindow_start + resend_end);
    }
}

/* Process game data from a client */

static void net_sv_parse_game_data(net_packet_t *packet,
        net_client_t *client)
{
  net_client_recv_t *recvobj;
  unsigned int seq;
  unsigned int ackseq;
  unsigned int num_tics;
  unsigned int nowtime;
  size_t i;
  int player;
  int resend_start;
  int resend_end;
  int index;

  if (server_state != SERVER_IN_GAME)
    {
      net_log_err("server: not in game state: server_state=%d",
              server_state);
      return;
    }

  if (client->drone)
    {
      /* Drones do not contribute any game data. */

      net_log_err("server: game data from a drone?");
      return;
    }

  player = client->player_number;

  /* Read header */

  if (!net_read_int8(packet, &ackseq) || !net_read_int8(packet, &seq) ||
      !net_read_int8(packet, &num_tics))
    {
      net_log_err("server: failed to read header");
      return;
    }

  net_log_info("server: got game data, seq=%d, num_tics=%d, ackseq=%d", seq,
               num_tics, ackseq);

  /* Get the current time */

  nowtime = i_get_time_ms();

  /* Expand 8-bit values to the full sequence number */

  ackseq = net_sv_expand_ticnum(ackseq);
  seq = net_sv_expand_ticnum(seq);

  /* Sanity checks */

  for (i = 0; i < num_tics; ++i)
    {
      net_ticdiff_t diff;
      signed int latency;

      if (!net_read_sint16(packet, &latency) ||
          !net_read_ticcmd_diff(packet, &diff, sv_settings.lowres_turn))
        {
          return;
        }

      index = seq + i - recvwindow_start;

      if (index < 0 || index >= CONFIG_GAMES_NXDOOM_NET_BACKUPTICS)
        {
          /* Not in range of the recv window */

          continue;
        }

      recvobj = &recvwindow[index][player];
      recvobj->active = true;
      recvobj->diff = diff;
      recvobj->latency = latency;

      client->last_gamedata_time = nowtime;
      net_log_info("server: stored tic %zu for player %d", seq + i, player);
    }

  /* Higher acknowledgement point? */

  if (ackseq > client->acknowledged)
    {
      net_log_info("server: acknowledged up to %d", ackseq);
      client->acknowledged = ackseq;
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
      recvobj = &recvwindow[index][player];

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
      net_log_info("server: request resend for %d-%d before %d",
                   recvwindow_start + resend_start,
                   recvwindow_start + resend_end - 1, seq);
      net_sv_send_resend_request(client, recvwindow_start + resend_start,
                                 recvwindow_start + resend_end - 1);
    }
}

static void net_sv_parse_game_data_ack(net_packet_t *packet,
                                       net_client_t *client)
{
  unsigned int ackseq;

  net_log_info("server: processing game data ack packet");

  if (server_state != SERVER_IN_GAME)
    {
      net_log_err("server: not in game state, server_state=%d",
              server_state);
      return;
    }

  /* Read header */

  if (!net_read_int8(packet, &ackseq))
    {
      net_log_err("server: missing acknowledgement field");
      return;
    }

  /* Expand 8-bit values to the full sequence number */

  ackseq = net_sv_expand_ticnum(ackseq);

  /* Higher acknowledgement point than we already have? */

  if (ackseq > client->acknowledged)
    {
      net_log_info("server: acknowledged up to %d", ackseq);
      client->acknowledged = ackseq;
    }
}

static void net_sv_send_tics(net_client_t *client, unsigned int start,
                             unsigned int end)
{
  net_packet_t *packet;
  unsigned int i;

  packet = net_new_packet(500);

  net_write_int16(packet, NET_PACKET_TYPE_GAMEDATA);

  /* Send the start tic and number of tics */

  net_write_int8(packet, start & 0xff);
  net_write_int8(packet, end - start + 1);

  /* Write the tics */

  for (i = start; i <= end; ++i)
    {
      net_full_ticcmd_t *cmd;

      cmd = &client->sendqueue[i % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

      if (i != cmd->seq)
        {
          i_error("Wanted to send %i, but %i is in its place", i, cmd->seq);
        }

      /* Add command */

      net_write_full_ticcmd(packet, cmd, sv_settings.lowres_turn);
    }

  /* Send packet */

  net_conn_send_packet(&client->connection, packet);

  net_free_packet(packet);
}

/* Parse a retransmission request from a client */

static void net_sv_parse_resend_request(net_packet_t *packet,
                                        net_client_t *client)
{
  unsigned int start;
  unsigned int last;
  unsigned int num_tics;
  unsigned int i;

  net_log_info("server: processing resend request");

  /* Read the starting tic and number of tics */

  if (!net_read_int32(packet, &start) || !net_read_int8(packet, &num_tics))
    {
      net_log_err("server: missing fields for resend");
      return;
    }

  /* Check we have all the requested tics */

  last = start + num_tics - 1;

  for (i = start; i <= last; ++i)
    {
      net_full_ticcmd_t *cmd;

      cmd = &client->sendqueue[i % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

      if (i != cmd->seq)
        {
          /* We do not have the requested tic (any more)
           * This is pretty fatal.  We could disconnect the client,
           * but then again this could be a spoofed packet.  Just
           * ignore it.
           */

          net_log_err("server: don't have tic %d any more, "
                      "can't resend",
                      i);
          return;
        }
    }

  /* Resend those tics */

  net_log_info("server: resending tics %d-%d", start, last);
  net_sv_send_tics(client, start, last);
}

static void net_sv_parse_hole_punch(net_packet_t *packet)
{
  const char *addr_string;
  net_packet_t *sendpacket;
  net_addr_t *addr;

  addr_string = net_read_string(packet);
  if (addr_string == NULL)
    {
      net_log_err("server: hole punch request but no address provided");
      return;
    }

  addr = net_resolve_address(server_context, addr_string);
  if (addr == NULL)
    {
      net_log_err("server: failed to resolve address: %s", addr_string);
      return;
    }

  sendpacket = net_new_packet(16);
  net_write_int16(sendpacket, NET_PACKET_TYPE_NAT_HOLE_PUNCH);
  net_send_packet(addr, sendpacket);
  net_free_packet(sendpacket);
  net_release_address(addr);
  net_log_info("server: sent hole punch to %s", addr_string);
}

static void net_sv_master_packet(net_packet_t *packet)
{
  unsigned int packet_type;

  /* Read the packet type */

  if (!net_read_int16(packet, &packet_type))
    {
      net_log_err("server: no packet type in master server message");
      return;
    }

  net_log_info("server: packet from master server; type %d", packet_type);
  net_log_packet(packet);

  switch (packet_type)
    {
    case NET_MASTER_PACKET_TYPE_ADD_RESPONSE:
      net_query_add_response(packet);
      break;

    case NET_MASTER_PACKET_TYPE_NAT_HOLE_PUNCH:
      net_sv_parse_hole_punch(packet);
      break;
    }
}

/* Send a response back to the client */

static void net_sv_send_query_response(net_addr_t *addr)
{
  net_packet_t *reply;
  net_querydata_t querydata;
  int p;

  /* Version */

  querydata.version = PACKAGE_STRING;

  /* Server state */

  querydata.server_state = server_state;

  /* Number of players/maximum players */

  querydata.num_players = net_sv_num_players();
  querydata.max_players = net_sv_max_players();

  /* Game mode/mission */

  querydata.gamemode = sv_gamemode;
  querydata.gamemission = sv_gamemission;

  /* @category net
   * @arg <name>
   *
   * When starting a network server, specify a name for the server.
   */

  p = m_check_parm_with_args("-servername", 1);

  if (p > 0)
    {
      querydata.description = myargv[p + 1];
    }
  else
    {
      querydata.description = "Unnamed server";
    }

  /* Send it and we're done. */

  net_log_info("server: sending query response to %s",
               net_addr_to_string(addr));
  reply = net_new_packet(64);
  net_write_int16(reply, NET_PACKET_TYPE_QUERY_RESPONSE);
  net_write_query_data(reply, &querydata);
  net_send_packet(addr, reply);
  net_free_packet(reply);
}

/* Process a packet received by the server */

static void net_sv_packet(net_packet_t *packet, net_addr_t *addr)
{
  net_client_t *client;
  unsigned int packet_type;

  /* Response from master server? */

  if (addr != NULL && addr == master_server)
    {
      net_sv_master_packet(packet);
      return;
    }

  /* Find which client this packet came from */

  client = net_sv_find_client(addr);

  /* Read the packet type */

  if (!net_read_int16(packet, &packet_type))
    {
      /* no packet type */

      return;
    }

  net_log_info("server: packet from %s; type %d", net_addr_to_string(addr),
               packet_type & ~NET_RELIABLE_PACKET);
  net_log_packet(packet);

  if (packet_type == NET_PACKET_TYPE_SYN)
    {
      net_sv_parse_syn(packet, client, addr);
    }
  else if (packet_type == NET_PACKET_TYPE_QUERY)
    {
      net_sv_send_query_response(addr);
    }
  else if (client == NULL)
    {
      /* Must come from a valid client; ignore otherwise */
    }
  else if (net_conn_packet(&client->connection, packet, &packet_type))
    {
      /* Packet was eaten by the common connection code */
    }
  else
    {
      switch (packet_type)
        {
        case NET_PACKET_TYPE_GAMESTART:
          net_sv_parse_game_start(packet, client);
          break;
        case NET_PACKET_TYPE_LAUNCH:
          net_sv_parse_launch(packet, client);
          break;
        case NET_PACKET_TYPE_GAMEDATA:
          net_sv_parse_game_data(packet, client);
          break;
        case NET_PACKET_TYPE_GAMEDATA_ACK:
          net_sv_parse_game_data_ack(packet, client);
          break;
        case NET_PACKET_TYPE_GAMEDATA_RESEND:
          net_sv_parse_resend_request(packet, client);
          break;
        default:
          break; /* unknown packet type */
        }
    }
}

static void net_sv_pump_send_queue(net_client_t *client)
{
  net_full_ticcmd_t cmd;
  int recv_index;
  int num_players;
  int i;
  int starttic;
  int endtic;

  /* If a client has not sent any acknowledgments for a while,
   * wait until they catch up.
   */

  if (client->sendseq - net_sv_latest_acknowledged() > 40)
    {
      return;
    }

  /* Work out the index into the receive window */

  recv_index = client->sendseq - recvwindow_start;

  if (recv_index < 0 || recv_index >= CONFIG_GAMES_NXDOOM_NET_BACKUPTICS)
    {
      return;
    }

  /* Check if we can generate a new entry for the send queue
   * using the data in recvwindow.
   */

  num_players = 0;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (sv_players[i] == client)
        {
          /* Client does not rely on itself for data */

          continue;
        }

      if (sv_players[i] == NULL || !client_connected(sv_players[i]))
        {
          continue;
        }

      if (!recvwindow[recv_index][i].active)
        {
          /* We do not have this player's ticcmd, so we cannot generate a
           * complete command yet.
           */

          return;
        }

      ++num_players;
    }

  /* If this is a game with only a single player in it, we might
   * be sending a ticcmd set containing 0 ticcmds. This is fine;
   * however, there's nothing to stop the game running on ahead
   * and never stopping. Don't let the server get too far ahead
   * of the client.
   */

  if (num_players == 0 && client->sendseq > recvwindow_start + 10)
    {
      return;
    }

  /* We have all data we need to generate a command for this tic. */

  cmd.seq = client->sendseq;

  /* Add ticcmds from all players */

  cmd.latency = 0;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      net_client_recv_t *recvobj;

      if (sv_players[i] == client)
        {
          /* Not the player we are sending to */

          cmd.playeringame[i] = false;
          continue;
        }

      if (sv_players[i] == NULL || !recvwindow[recv_index][i].active)
        {
          cmd.playeringame[i] = false;
          continue;
        }

      cmd.playeringame[i] = true;

      recvobj = &recvwindow[recv_index][i];

      cmd.cmds[i] = recvobj->diff;

      if (recvobj->latency > cmd.latency) cmd.latency = recvobj->latency;
    }

  /* Add into the queue */

  client->sendqueue[client->sendseq %
      CONFIG_GAMES_NXDOOM_NET_BACKUPTICS] = cmd;

  /* Transmit the new tic to the client */

  starttic = client->sendseq - sv_settings.extratics;
  endtic = client->sendseq;

  if (starttic < 0) starttic = 0;

  net_log_info("server: send tics %d-%d to %s", starttic, endtic,
               net_addr_to_string(client->addr));
  net_sv_send_tics(client, starttic, endtic);

  ++client->sendseq;
}

/* Called when all players have disconnected.  Return to listening for
 * players to start a new game, and disconnect any drones still connected.
 */

static void net_sv_game_ended(void)
{
  int i;

  server_state = SERVER_WAITING_LAUNCH;
  sv_gamemode = indetermined;

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (clients[i].active)
        {
          net_sv_disconnect_client(&clients[i]);
        }
    }
}

/* Prevent against deadlock: resend requests are usually only
 * triggered if we miss a packet and receive the next one.
 * If we miss a whole load of packets, we can end up in a
 * deadlock situation where the client will not send any more.
 * If we don't receive any game data in a while, trigger a resend
 * request for the next tic we're expecting.
 */

static void net_sv_check_deadlock(net_client_t *client)
{
  int nowtime;
  int i;

  /* Don't expect game data from clients. */

  if (client->drone)
    {
      return;
    }

  nowtime = i_get_time_ms();

  /* If we haven't received anything for a long time, it may be a deadlock. */

  if (nowtime - client->last_gamedata_time > 1000)
    {
      net_log_warn("server: no gamedata from %s since %d - deadlock?",
                   net_addr_to_string(client->addr),
                   client->last_gamedata_time);

      /* Search the receive window for the first tic we are expecting
       * from this player.
       */

      for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_BACKUPTICS; ++i)
        {
          if (!recvwindow[i][client->player_number].active)
            {
              net_log_warn(
                  "server: deadlock: sending resend request for %d-%d",
                  recvwindow_start + i, recvwindow_start + i + 5);

              /* Found a tic we haven't received.  Send a resend request. */

              net_sv_send_resend_request(client, recvwindow_start + i,
                                         recvwindow_start + i + 5);

              client->last_gamedata_time = nowtime;
              break;
            }
        }

      /* If we sent a resend request to break the deadlock, also trigger a
       * resend of any tics we have sitting in the send queue, in case the
       * client is blocked waiting on tics from us that have been lost.
       * This fixes deadlock with some older clients which do not send
       * resends to break deadlock.
       */

      if (i < CONFIG_GAMES_NXDOOM_NET_BACKUPTICS &&
          client->sendseq > client->acknowledged)
        {
          net_log_warn("server: also resending tics %d-%d to break deadlock",
                       client->acknowledged, client->sendseq - 1);
          net_sv_send_tics(client, client->acknowledged,
                  client->sendseq - 1);
        }
    }
}

/* Perform any needed action on a client */

static void net_sv_run_client(net_client_t *client)
{
  /* Run common code */

  net_conn_run(&client->connection);

  if (client->connection.state == NET_CONN_STATE_DISCONNECTED &&
      client->connection.disconnect_reason == NET_DISCONNECT_TIMEOUT)
    {
      net_log_warn("server: client at %s timed out",
                   net_addr_to_string(client->addr));
      net_sv_broadcast_message("Client '%s' timed out and disconnected",
                               client->name);
    }

  /* Is this client disconnected? */

  if (client->connection.state == NET_CONN_STATE_DISCONNECTED)
    {
      client->active = false;

      /* If we were about to start a game, any player disconnecting
       * should cause an abort.
       */

      if (server_state == SERVER_WAITING_START && !client->drone)
        {
          net_sv_broadcast_message("Game startup aborted because "
                                   "player '%s' disconnected.",
                                   client->name);
          net_sv_game_ended();
        }

      free(client->name);
      net_release_address(client->addr);

      /* Are there any clients left connected?  If not, return the
       * server to the waiting-for-players state.
       *
       * Disconnect any drones still connected.
       */

      if (net_sv_num_players() <= 0)
        {
          net_log_info("server: no player clients left, game ended");
          net_sv_game_ended();
        }
    }

  if (!client_connected(client))
    {
      /* client has not yet finished connecting */

      return;
    }

  if (server_state == SERVER_WAITING_LAUNCH)
    {
      /* Waiting for the game to start */

      /* Send information once every second */

      if (client->last_send_time < 0 ||
          i_get_time_ms() - client->last_send_time > 1000)
        {
          net_sv_send_waiting_data(client);
          client->last_send_time = i_get_time_ms();
        }
    }

  if (server_state == SERVER_IN_GAME)
    {
      net_sv_pump_send_queue(client);
      net_sv_check_deadlock(client);
    }
}

static void update_master_server(void)
{
  unsigned int now;

  now = i_get_time_ms();

  /* The address of the master server can change. Periodically
   * re-resolve the master server to update.
   */

  if (now - master_resolve_time > MASTER_RESOLVE_PERIOD * 1000)
    {
      net_addr_t *new_addr;

      new_addr = net_query_resolve_master(server_context);
      net_release_address(master_server);
      master_server = new_addr;

      master_resolve_time = now;
    }

  /* Possibly refresh our registration with the master server. */

  if (now - master_refresh_time > MASTER_REFRESH_PERIOD * 1000)
    {
      net_query_add_to_master(master_server);
      master_refresh_time = now;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Add a network module to the server context */

void net_sv_add_module(net_module_t *module)
{
  module->init_server();
  net_add_module(server_context, module);
}

/* Initialize server and wait for connections */

void net_sv_init(void)
{
  int i;

  /* initialize send/receive context */

  server_context = net_new_context();

  /* no clients yet */

  for (i = 0; i < MAXNETNODES; ++i)
    {
      clients[i].active = false;
    }

  net_sv_assign_players();

  server_state = SERVER_WAITING_LAUNCH;
  sv_gamemode = indetermined;
  server_initialized = true;
}

void net_sv_register_with_master(void)
{
  /* @category net
   *
   * When running a server, don't register with the global master server.
   * Implies -server.
   */

  if (!m_check_parm("-privateserver"))
    {
      master_server = net_query_resolve_master(server_context);
    }
  else
    {
      master_server = NULL;
    }

  /* Send request. */

  if (master_server != NULL)
    {
      net_query_add_to_master(master_server);
      master_refresh_time = i_get_time_ms();
      master_resolve_time = master_refresh_time;
    }
}

/* Run server code to check for new packets/send packets as the server
 * requires
 */

void net_sv_run(void)
{
  net_addr_t *addr;
  net_packet_t *packet;
  int i;

  if (!server_initialized)
    {
      return;
    }

  while (net_recv_packet(server_context, &addr, &packet))
    {
      net_sv_packet(packet, addr);
      net_free_packet(packet);
      net_release_address(addr);
    }

  if (master_server != NULL)
    {
      update_master_server();
    }

  /* "Run" any clients that may have things to do, independent of responses
   * to received packets
   */

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (clients[i].active)
        {
          net_sv_run_client(&clients[i]);
        }
    }

  switch (server_state)
    {
    case SERVER_WAITING_LAUNCH:
      break;

    case SERVER_WAITING_START:
      check_start_game();
      break;

    case SERVER_IN_GAME:
      net_sv_advance_window();

      for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
        {
          if (sv_players[i] != NULL && client_connected(sv_players[i]))
            {
              net_sv_check_resends(sv_players[i]);
            }
        }
      break;
    }
}

void net_sv_shutdown(void)
{
  int i;
  boolean running;
  int start_time;

  if (!server_initialized)
    {
      return;
    }

  fprintf(stderr, "SV: Shutting down server...\n");

  /* Disconnect all clients */

  for (i = 0; i < MAXNETNODES; ++i)
    {
      if (clients[i].active)
        {
          net_sv_disconnect_client(&clients[i]);
        }
    }

  /* Wait for all clients to finish disconnecting */

  start_time = i_get_time_ms();
  running = true;

  while (running)
    {
      /* Check if any clients are still not finished */

      running = false;

      for (i = 0; i < MAXNETNODES; ++i)
        {
          if (clients[i].active)
            {
              running = true;
            }
        }

      /* Timed out? */

      if (i_get_time_ms() - start_time > 5000)
        {
          running = false;
          fprintf(stderr,
                  "SV: Timed out waiting for clients to disconnect.\n");
        }

      /* Run the client code in case this is a loopback client. */

      net_cl_run();
      net_sv_run();

      /* Don't hog the CPU */

      usleep(1000);
    }
}
