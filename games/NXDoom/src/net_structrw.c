/****************************************************************************
 * apps/games/NXDoom/src/net_structrw.c
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
 * Reading and writing various structures into packets
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "i_system.h"
#include "m_misc.h"
#include "net_packet.h"
#include "net_structrw.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct protocol_name
{
  net_protocol_t protocol;
  const char *name;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* String names for the enum values in net_protocol_t, which are what is
 * sent over the wire. Every enum value must have an entry in this list.
 */

static struct protocol_name g_protocol_names[] =
{
  {NET_PROTOCOL_CHOCOLATE_DOOM_0, "CHOCOLATE_DOOM_0"},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static boolean net_read_blob(net_packet_t *packet, uint8_t *buf, size_t len)
{
  unsigned int b;
  int i;

  for (i = 0; i < len; ++i)
    {
      if (!net_read_int8(packet, &b))
        {
          return false;
        }

      buf[i] = b;
    }

  return true;
}

static void net_write_blob(net_packet_t *packet, uint8_t *buf, size_t len)
{
  int i;

  for (i = 0; i < len; ++i)
    {
      net_write_int8(packet, buf[i]);
    }
}

static net_protocol_t parse_protocol_name(const char *name)
{
  int i;

  for (i = 0; i < arrlen(g_protocol_names); ++i)
    {
      if (!strcmp(g_protocol_names[i].name, name))
        {
          return g_protocol_names[i].protocol;
        }
    }

  return NET_PROTOCOL_UNKNOWN;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void net_write_connect_data(net_packet_t *packet, net_connect_data_t *data)
{
  net_write_int8(packet, data->gamemode);
  net_write_int8(packet, data->gamemission);
  net_write_int8(packet, data->lowres_turn);
  net_write_int8(packet, data->drone);
  net_write_int8(packet, data->max_players);
  net_write_int8(packet, data->is_freedoom);
  net_write_sha1_sum(packet, data->wad_sha1sum);
  net_write_sha1_sum(packet, data->deh_sha1sum);
  net_write_int8(packet, data->player_class);
}

boolean net_read_connect_data(net_packet_t *packet, net_connect_data_t *data)
{
  return net_read_int8(packet, (unsigned int *)&data->gamemode) &&
         net_read_int8(packet, (unsigned int *)&data->gamemission) &&
         net_read_int8(packet, (unsigned int *)&data->lowres_turn) &&
         net_read_int8(packet, (unsigned int *)&data->drone) &&
         net_read_int8(packet, (unsigned int *)&data->max_players) &&
         net_read_int8(packet, (unsigned int *)&data->is_freedoom) &&
         net_read_sha1_sum(packet, data->wad_sha1sum) &&
         net_read_sha1_sum(packet, data->deh_sha1sum) &&
         net_read_int8(packet, (unsigned int *)&data->player_class);
}

void net_write_settings(net_packet_t *packet, net_gamesettings_t *settings)
{
  int i;

  net_write_int8(packet, settings->ticdup);
  net_write_int8(packet, settings->extratics);
  net_write_int8(packet, settings->deathmatch);
  net_write_int8(packet, settings->nomonsters);
  net_write_int8(packet, settings->fast_monsters);
  net_write_int8(packet, settings->respawn_monsters);
  net_write_int8(packet, settings->episode);
  net_write_int8(packet, settings->map);
  net_write_int8(packet, settings->skill);
  net_write_int8(packet, settings->gameversion);
  net_write_int8(packet, settings->lowres_turn);
  net_write_int8(packet, settings->new_sync);
  net_write_int32(packet, settings->timelimit);
  net_write_int8(packet, settings->loadgame);
  net_write_int8(packet, settings->random);
  net_write_int8(packet, settings->num_players);
  net_write_int8(packet, settings->consoleplayer);

  for (i = 0; i < settings->num_players; ++i)
    {
      net_write_int8(packet, settings->player_classes[i]);
    }
}

boolean net_read_settings(net_packet_t *packet, net_gamesettings_t *settings)
{
  boolean success;
  int i;

  success =
      net_read_int8(packet, (unsigned int *)&settings->ticdup) &&
      net_read_int8(packet, (unsigned int *)&settings->extratics) &&
      net_read_int8(packet, (unsigned int *)&settings->deathmatch) &&
      net_read_int8(packet, (unsigned int *)&settings->nomonsters) &&
      net_read_int8(packet, (unsigned int *)&settings->fast_monsters) &&
      net_read_int8(packet, (unsigned int *)&settings->respawn_monsters) &&
      net_read_int8(packet, (unsigned int *)&settings->episode) &&
      net_read_int8(packet, (unsigned int *)&settings->map) &&
      net_read_sint8(packet, &settings->skill) &&
      net_read_int8(packet, (unsigned int *)&settings->gameversion) &&
      net_read_int8(packet, (unsigned int *)&settings->lowres_turn) &&
      net_read_int8(packet, (unsigned int *)&settings->new_sync) &&
      net_read_int32(packet, (unsigned int *)&settings->timelimit) &&
      net_read_sint8(packet, (signed int *)&settings->loadgame) &&
      net_read_int8(packet, (unsigned int *)&settings->random) &&
      net_read_int8(packet, (unsigned int *)&settings->num_players) &&
      net_read_sint8(packet, (signed int *)&settings->consoleplayer);

  if (!success)
    {
      return false;
    }

  for (i = 0;
       i < settings->num_players && i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS;
       ++i)
    {
      if (!net_read_int8(packet,
                         (unsigned int *)&settings->player_classes[i]))
        {
          return false;
        }
    }

  return true;
}

boolean net_read_query_data(net_packet_t *packet, net_querydata_t *query)
{
  boolean success;

  query->version = net_read_safe_string(packet);

  success = query->version != NULL &&
            net_read_int8(packet, (unsigned int *)&query->server_state) &&
            net_read_int8(packet, (unsigned int *)&query->num_players) &&
            net_read_int8(packet, (unsigned int *)&query->max_players) &&
            net_read_int8(packet, (unsigned int *)&query->gamemode) &&
            net_read_int8(packet, (unsigned int *)&query->gamemission);

  if (!success)
    {
      return false;
    }

  query->description = net_read_safe_string(packet);

  /* We read the list of protocols supported by the server. However,
   * old versions of Chocolate Doom do not support this field; it is
   * okay if it cannot be successfully read.
   */

  query->protocol = net_read_protocol_list(packet);

  return query->description != NULL;
}

void net_write_query_data(net_packet_t *packet, net_querydata_t *query)
{
  net_write_string(packet, query->version);
  net_write_int8(packet, query->server_state);
  net_write_int8(packet, query->num_players);
  net_write_int8(packet, query->max_players);
  net_write_int8(packet, query->gamemode);
  net_write_int8(packet, query->gamemission);
  net_write_string(packet, query->description);

  /* Write a list of all supported protocols. Note that the query->protocol
   * field is ignored here; it is only used when receiving.
   */

  net_write_protocol_list(packet);
}

void net_write_ticcmd_diff(net_packet_t *packet, net_ticdiff_t *diff,
                         boolean lowres_turn)
{
  /* Header */

  net_write_int8(packet, diff->diff);

  /* Write the fields which are enabled: */

  if (diff->diff & NET_TICDIFF_FORWARD)
    net_write_int8(packet, diff->cmd.forwardmove);
  if (diff->diff & NET_TICDIFF_SIDE)
    net_write_int8(packet, diff->cmd.sidemove);
  if (diff->diff & NET_TICDIFF_TURN)
    {
      if (lowres_turn)
        {
          net_write_int8(packet, diff->cmd.angleturn / 256);
        }
      else
        {
          net_write_int16(packet, diff->cmd.angleturn);
        }
    }

  if (diff->diff & NET_TICDIFF_BUTTONS)
    net_write_int8(packet, diff->cmd.buttons);
  if (diff->diff & NET_TICDIFF_CONSISTANCY)
    net_write_int8(packet, diff->cmd.consistency);
  if (diff->diff & NET_TICDIFF_CHATCHAR)
    net_write_int8(packet, diff->cmd.chatchar);
  if (diff->diff & NET_TICDIFF_RAVEN)
    {
      net_write_int8(packet, diff->cmd.lookfly);
      net_write_int8(packet, diff->cmd.arti);
    }

  if (diff->diff & NET_TICDIFF_STRIFE)
    {
      net_write_int8(packet, diff->cmd.buttons2);
      net_write_int16(packet, diff->cmd.inventory);
    }
}

boolean net_read_ticcmd_diff(net_packet_t *packet, net_ticdiff_t *diff,
                           boolean lowres_turn)
{
  unsigned int val;
  signed int sval;

  /* Read header */

  if (!net_read_int8(packet, &diff->diff)) return false;

  /* Read fields */

  if (diff->diff & NET_TICDIFF_FORWARD)
    {
      if (!net_read_sint8(packet, &sval)) return false;
      diff->cmd.forwardmove = sval;
    }

  if (diff->diff & NET_TICDIFF_SIDE)
    {
      if (!net_read_sint8(packet, &sval)) return false;
      diff->cmd.sidemove = sval;
    }

  if (diff->diff & NET_TICDIFF_TURN)
    {
      if (lowres_turn)
        {
          if (!net_read_sint8(packet, &sval)) return false;
          diff->cmd.angleturn = sval * 256;
        }
      else
        {
          if (!net_read_sint16(packet, &sval)) return false;
          diff->cmd.angleturn = sval;
        }
    }

  if (diff->diff & NET_TICDIFF_BUTTONS)
    {
      if (!net_read_int8(packet, &val)) return false;
      diff->cmd.buttons = val;
    }

  if (diff->diff & NET_TICDIFF_CONSISTANCY)
    {
      if (!net_read_int8(packet, &val)) return false;
      diff->cmd.consistency = val;
    }

  if (diff->diff & NET_TICDIFF_CHATCHAR)
    {
      if (!net_read_int8(packet, &val)) return false;
      diff->cmd.chatchar = val;
    }

  if (diff->diff & NET_TICDIFF_RAVEN)
    {
      if (!net_read_int8(packet, &val)) return false;
      diff->cmd.lookfly = val;

      if (!net_read_int8(packet, &val)) return false;
      diff->cmd.arti = val;
    }

  if (diff->diff & NET_TICDIFF_STRIFE)
    {
      if (!net_read_int8(packet, &val)) return false;
      diff->cmd.buttons2 = val;

      if (!net_read_int16(packet, &val)) return false;
      diff->cmd.inventory = val;
    }

  return true;
}

void net_ticcmd_diff(ticcmd_t *tic1, ticcmd_t *tic2, net_ticdiff_t *diff)
{
  diff->diff = 0;
  diff->cmd = *tic2;

  if (tic1->forwardmove != tic2->forwardmove)
    diff->diff |= NET_TICDIFF_FORWARD;
  if (tic1->sidemove != tic2->sidemove) diff->diff |= NET_TICDIFF_SIDE;
  if (tic1->angleturn != tic2->angleturn) diff->diff |= NET_TICDIFF_TURN;
  if (tic1->buttons != tic2->buttons) diff->diff |= NET_TICDIFF_BUTTONS;
  if (tic1->consistency != tic2->consistency)
    diff->diff |= NET_TICDIFF_CONSISTANCY;
  if (tic2->chatchar != 0) diff->diff |= NET_TICDIFF_CHATCHAR;

  /* Heretic/Hexen-specific */

  if (tic1->lookfly != tic2->lookfly || tic2->arti != 0)
    diff->diff |= NET_TICDIFF_RAVEN;

  /* Strife-specific */

  if (tic1->buttons2 != tic2->buttons2 || tic2->inventory != 0)
    diff->diff |= NET_TICDIFF_STRIFE;
}

void net_ticcmd_patch(ticcmd_t *src, net_ticdiff_t *diff, ticcmd_t *dest)
{
  memmove(dest, src, sizeof(ticcmd_t));

  /* Apply the diff */

  if (diff->diff & NET_TICDIFF_FORWARD)
    dest->forwardmove = diff->cmd.forwardmove;
  if (diff->diff & NET_TICDIFF_SIDE) dest->sidemove = diff->cmd.sidemove;
  if (diff->diff & NET_TICDIFF_TURN) dest->angleturn = diff->cmd.angleturn;
  if (diff->diff & NET_TICDIFF_BUTTONS) dest->buttons = diff->cmd.buttons;
  if (diff->diff & NET_TICDIFF_CONSISTANCY)
    dest->consistency = diff->cmd.consistency;

  if (diff->diff & NET_TICDIFF_CHATCHAR)
    dest->chatchar = diff->cmd.chatchar;
  else
    dest->chatchar = 0;

  /* Heretic/Hexen specific: */

  if (diff->diff & NET_TICDIFF_RAVEN)
    {
      dest->lookfly = diff->cmd.lookfly;
      dest->arti = diff->cmd.arti;
    }
  else
    {
      dest->arti = 0;
    }

  /* Strife-specific: */

  if (diff->diff & NET_TICDIFF_STRIFE)
    {
      dest->buttons2 = diff->cmd.buttons2;
      dest->inventory = diff->cmd.inventory;
    }
  else
    {
      dest->inventory = 0;
    }
}

/* net_full_ticcmd_t */

boolean net_read_full_ticcmd(net_packet_t *packet, net_full_ticcmd_t *cmd,
                           boolean lowres_turn)
{
  unsigned int bitfield;
  int i;

  /* Latency */

  if (!net_read_sint16(packet, &cmd->latency))
    {
      return false;
    }

  /* Regenerate playeringame from the "header" bitfield */

  if (!net_read_int8(packet, &bitfield))
    {
      return false;
    }

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      cmd->playeringame[i] = (bitfield & (1 << i)) != 0;
    }

  /* Read cmds */

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (cmd->playeringame[i])
        {
          if (!net_read_ticcmd_diff(packet, &cmd->cmds[i], lowres_turn))
            {
              return false;
            }
        }
    }

  return true;
}

void net_write_full_ticcmd(net_packet_t *packet, net_full_ticcmd_t *cmd,
                         boolean lowres_turn)
{
  unsigned int bitfield;
  int i;

  /* Write the latency */

  net_write_int16(packet, cmd->latency);

  /* Write "header" byte indicating which players are active
   * in this ticcmd
   */

  bitfield = 0;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (cmd->playeringame[i])
        {
          bitfield |= 1 << i;
        }
    }

  net_write_int8(packet, bitfield);

  /* Write player ticcmds */

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (cmd->playeringame[i])
        {
          net_write_ticcmd_diff(packet, &cmd->cmds[i], lowres_turn);
        }
    }
}

void net_write_wait_data(net_packet_t *packet, net_waitdata_t *data)
{
  int i;

  net_write_int8(packet, data->num_players);
  net_write_int8(packet, data->num_drones);
  net_write_int8(packet, data->ready_players);
  net_write_int8(packet, data->max_players);
  net_write_int8(packet, data->is_controller);
  net_write_int8(packet, data->consoleplayer);

  for (i = 0;
       i < data->num_players && i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS;
       ++i)
    {
      net_write_string(packet, data->player_names[i]);
      net_write_string(packet, data->player_addrs[i]);
    }

  net_write_sha1_sum(packet, data->wad_sha1sum);
  net_write_sha1_sum(packet, data->deh_sha1sum);
  net_write_int8(packet, data->is_freedoom);
}

boolean net_read_wait_data(net_packet_t *packet, net_waitdata_t *data)
{
  int i;
  char *s;

  if (!net_read_int8(packet, (unsigned int *)&data->num_players) ||
      !net_read_int8(packet, (unsigned int *)&data->num_drones) ||
      !net_read_int8(packet, (unsigned int *)&data->ready_players) ||
      !net_read_int8(packet, (unsigned int *)&data->max_players) ||
      !net_read_int8(packet, (unsigned int *)&data->is_controller) ||
      !net_read_sint8(packet, &data->consoleplayer))
    {
      return false;
    }

  for (i = 0;
       i < data->num_players && i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS;
       ++i)
    {
      s = net_read_string(packet);

      if (s == NULL || strlen(s) >= CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME)
        {
          return false;
        }

      m_str_copy(data->player_names[i], s,
                 CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME);

      s = net_read_string(packet);

      if (s == NULL || strlen(s) >= CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME)
        {
          return false;
        }

      m_str_copy(data->player_addrs[i], s,
                 CONFIG_GAMES_NXDOOM_NET_MAXPLAYERNAME);
    }

  return net_read_sha1_sum(packet, data->wad_sha1sum) &&
         net_read_sha1_sum(packet, data->deh_sha1sum) &&
         net_read_int8(packet, (unsigned int *)&data->is_freedoom);
}

boolean net_read_sha1_sum(net_packet_t *packet, sha1_digest_t digest)
{
  return net_read_blob(packet, digest, sizeof(sha1_digest_t));
}

void net_write_sha1_sum(net_packet_t *packet, sha1_digest_t digest)
{
  net_write_blob(packet, digest, sizeof(sha1_digest_t));
}

boolean net_read_prng_seed(net_packet_t *packet, prng_seed_t seed)
{
  return net_read_blob(packet, seed, sizeof(prng_seed_t));
}

void net_write_prng_seed(net_packet_t *packet, prng_seed_t seed)
{
  net_write_blob(packet, seed, sizeof(prng_seed_t));
}

/* net_read_protocol reads a single string-format protocol name from the
 * given packet, returning NET_PROTOCOL_UNKNOWN if the string describes an
 * unknown protocol.
 */

net_protocol_t net_read_protocol(net_packet_t *packet)
{
  const char *name;

  name = net_read_string(packet);
  if (name == NULL)
    {
      return NET_PROTOCOL_UNKNOWN;
    }

  return parse_protocol_name(name);
}

/* net_write_protocol writes a single string-format protocol name to a
 * packet.
 */

void net_write_protocol(net_packet_t *packet, net_protocol_t protocol)
{
  int i;

  for (i = 0; i < arrlen(g_protocol_names); ++i)
    {
      if (g_protocol_names[i].protocol == protocol)
        {
          net_write_string(packet, g_protocol_names[i].name);
          return;
        }
    }

  /* If you add an entry to the net_protocol_t enum, a corresponding entry
   * must be added to the protocol_names list.
   */

  i_error("net_write_protocol: protocol %d missing from protocol_names "
          "list; please add it.",
          protocol);
}

/* net_read_protocol_list reads a list of string-format protocol names from
 * the given packet, returning a single protocol number. The protocol that is
 * returned is the last protocol in the list that is a supported protocol. If
 * no recognized protocols are read, NET_PROTOCOL_UNKNOWN is returned.
 */

net_protocol_t net_read_protocol_list(net_packet_t *packet)
{
  net_protocol_t result;
  unsigned int num_protocols;
  int i;

  if (!net_read_int8(packet, &num_protocols))
    {
      return NET_PROTOCOL_UNKNOWN;
    }

  result = NET_PROTOCOL_UNKNOWN;

  for (i = 0; i < num_protocols; ++i)
    {
      net_protocol_t p;
      const char *name;

      name = net_read_string(packet);
      if (name == NULL)
        {
          return NET_PROTOCOL_UNKNOWN;
        }

      p = parse_protocol_name(name);
      if (p != NET_PROTOCOL_UNKNOWN)
        {
          result = p;
        }
    }

  return result;
}

/* net_write_protocol_list writes a list of string-format protocol names into
 * the given packet, all the supported protocols in the net_protocol_t enum.
 * This is slightly different to other functions in this file, in that there
 * is nothing the caller can "choose" to write; the built-in list of all
 * protocols is always sent.
 */

void net_write_protocol_list(net_packet_t *packet)
{
  int i;

  net_write_int8(packet, NET_NUM_PROTOCOLS);

  for (i = 0; i < NET_NUM_PROTOCOLS; ++i)
    {
      net_write_protocol(packet, i);
    }
}
