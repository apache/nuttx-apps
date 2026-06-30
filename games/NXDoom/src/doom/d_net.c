/****************************************************************************
 * apps/games/NXDoom/src/doom/d_net.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
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
 * DESCRIPTION:
 *  DOOM Network game communication and protocol, all OS independent parts.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>

#include "d_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "w_checksum.h"
#include "w_wad.h"

#include "deh_main.h"

#include "d_loop.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void run_tic(ticcmd_t *cmds, boolean *ingame);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static loop_interface_t g_doom_loop_interface =
{
  d_process_events,
  g_build_ticcmd,
  run_tic,
  m_ticker,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

ticcmd_t *netcmds;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: player_quit_game
 *
 * Description:
 *  Called when a player leaves the game
 *
 ****************************************************************************/

static void player_quit_game(player_t *player)
{
  static char exitmsg[80];
  unsigned int player_num;

  player_num = player - players;

  /* Do this the same way as Vanilla Doom does, to allow dehacked
   * replacements of this message
   */

  m_str_copy(exitmsg, ("Player 1 left the game"), sizeof(exitmsg));

  exitmsg[7] += player_num;

  playeringame[player_num] = false;
  players[consoleplayer].message = exitmsg;

  /* TODO: check if it is sensible to do this: */

  if (demorecording)
    {
      g_check_demo_status();
    }
}

static void run_tic(ticcmd_t *cmds, boolean *ingame)
{
  unsigned int i;

  /* Check for player quits. */

  for (i = 0; i < MAXPLAYERS; ++i)
    {
      if (!demoplayback && playeringame[i] && !ingame[i])
        {
          player_quit_game(&players[i]);
        }
    }

  netcmds = cmds;

  /* check that there are players in the game.  if not, we cannot
   * run a tic.
   */

  if (advancedemo) d_do_advance_demo();

  g_ticker();
}

/****************************************************************************
 * Name: Load_Game_Settings
 *
 * Description:
 *  Load game settings from the specified structure and
 *  set global variables.
 *
 ****************************************************************************/

static void load_game_settings(net_gamesettings_t *settings)
{
  unsigned int i;

  deathmatch = settings->deathmatch;
  startepisode = settings->episode;
  startmap = settings->map;
  startskill = settings->skill;
  startloadgame = settings->loadgame;
  lowres_turn = settings->lowres_turn;
  nomonsters = settings->nomonsters;
  fastparm = settings->fast_monsters;
  respawnparm = settings->respawn_monsters;
  timelimit = settings->timelimit;
  consoleplayer = settings->consoleplayer;

  if (lowres_turn)
    {
      printf("NOTE: Turning resolution is reduced; this is probably "
             "because there is a client recording a Vanilla demo.\n");
    }

  for (i = 0; i < MAXPLAYERS; ++i)
    {
      playeringame[i] = i < settings->num_players;
    }
}

/****************************************************************************
 * Name: save_game_settings
 *
 * Description:
 *  Save the game settings from global variables to the specified
 *  game settings structure.
 *
 ****************************************************************************/

static void save_game_settings(net_gamesettings_t *settings)
{
  /* Fill in game settings structure with appropriate parameters
   * for the new game
   */

  settings->deathmatch = deathmatch;
  settings->episode = startepisode;
  settings->map = startmap;
  settings->skill = startskill;
  settings->loadgame = startloadgame;
  settings->gameversion = gameversion;
  settings->nomonsters = nomonsters;
  settings->fast_monsters = fastparm;
  settings->respawn_monsters = respawnparm;
  settings->timelimit = timelimit;

  settings->lowres_turn =
      (m_parm_exists("-record") && !m_parm_exists("-longtics")) ||
      m_parm_exists("-shorttics");
}

static void init_connect_data(net_connect_data_t *connect_data)
{
  boolean shorttics;

  connect_data->max_players = MAXPLAYERS;
  connect_data->drone = false;

  /* @category net
   *
   * Run as the left screen in three screen mode.
   */

  if (m_check_parm("-left") > 0)
    {
      viewangleoffset = ANG90;
      connect_data->drone = true;
    }

  /* @category net
   *
   * Run as the right screen in three screen mode.
   */

  if (m_check_parm("-right") > 0)
    {
      viewangleoffset = ANG270;
      connect_data->drone = true;
    }

  /* Connect data */

  /* Game type fields: */

  connect_data->gamemode = gamemode;
  connect_data->gamemission = gamemission;

  /* @category demo
   *
   * Play with low turning resolution to emulate demo recording.
   */

  shorttics = m_parm_exists("-shorttics");

  /* Are we recording a demo? Possibly set lowres turn mode */

  connect_data->lowres_turn =
      (m_parm_exists("-record") && !m_parm_exists("-longtics")) || shorttics;

  /* Read checksums of our WAD directory and dehacked information */

  w_checksum(connect_data->wad_sha1sum);
  deh_checksum(connect_data->deh_sha1sum);

  /* Are we playing with the Freedoom IWAD? */

  connect_data->is_freedoom = w_check_num_for_name("FREEDOOM") >= 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void d_connect_net_game(void)
{
  net_connect_data_t connect_data;

  init_connect_data(&connect_data);
  netgame = d_init_net_game(&connect_data);

#ifdef CONFIG_GAMES_NXDOOM_NET
  /* @category net
   *
   * Start the game playing as though in a netgame with a single
   * player. This can also be used to play back single player netgame
   * demos.
   */

  if (m_check_parm("-solo-net") > 0)
    {
      netgame = true;
    }
#endif
}

/****************************************************************************
 * Name: d_check_net_game
 *
 * Description:
 *  Works out player numbers among the net participants
 *
 ****************************************************************************/

void d_check_net_game(void)
{
  net_gamesettings_t settings;

  if (netgame)
    {
      autostart = true;
    }

  d_register_loop_callbacks(&g_doom_loop_interface);

  save_game_settings(&settings);
  d_start_net_game(&settings, NULL);
  load_game_settings(&settings);

  printf("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
         startskill, deathmatch, startmap, startepisode);

  printf("player %i of %i (%i nodes)\n", consoleplayer + 1,
         settings.num_players, settings.num_players);

  /* Show players here; the server might have specified a time limit */

  if (timelimit > 0 && deathmatch)
    {
      /* Gross hack to work like Vanilla: */

      if (timelimit == 20 && m_check_parm("-avg"))
        {
          printf("Austin Virtual Gaming: Levels will end "
                 "after 20 minutes\n");
        }
      else
        {
          printf("Levels will end after %d minute", timelimit);
          if (timelimit > 1) printf("s");
          printf(".\n");
        }
    }
}
