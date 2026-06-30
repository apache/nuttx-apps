/****************************************************************************
 * apps/games/NXDoom/src/d_loop.c
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
 *   Main loop code.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "d_event.h"
#include "d_loop.h"
#include "d_ticcmd.h"

#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "m_argv.h"
#include "m_fixed.h"

#ifdef CONFIG_GAMES_NXDOOM_NET
#include "net_client.h"
#include "net_gui.h"
#include "net_io.h"
#include "net_loop.h"
#include "net_query.h"
#include "net_sdl.h"
#include "net_server.h"
#endif

/* TODO: Move nonvanilla demo functions into a dedicated file. */

#include "m_misc.h"
#include "w_wad.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maximum time that we wait in try_run_tics() for netgame data to be
 * received before we bail out and render a frame anyway.
 * Vanilla Doom used 20 for this value, but we use a smaller value
 * instead for better responsiveness of the menu when we're stuck.
 */

#define MAX_NETGAME_STALL_TICS 5

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The complete set of data for a particular tic. */

typedef struct
{
  ticcmd_t cmds[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];
  boolean ingame[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];
} ticcmd_set_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* gametic is the tic about to (or currently being) run
 * maketic is the tic that hasn't had control made for it yet
 * recvtic is the latest tic received from the server.
 *
 * a gametic cannot be run until ticcmds are received for it
 * from all players.
 */

static ticcmd_set_t ticdata[CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

/* The index of the next tic to be made (with a call to build_ticcmd). */

static int maketic;

/* The number of complete tics received from the server so far. */

static int recvtic;

/* Index of the local player. */

static int localplayer;

/* Used for original sync code. */

static int skiptics = 0;

/* Use new client synchronization code */

static boolean new_sync = true;

/* Callback functions for loop code. */

static loop_interface_t *loop_interface = NULL;

/* Current players in the multiplayer game.
 * This is distinct from playeringame[] used by the game code, which may
 * modify playeringame[] when playing back multiplayer demos.
 */

static boolean local_playeringame[CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS];

/* Requested player class "sent" to the server on connect.
 * If we are only doing a single player game then this needs to be remembered
 * and saved in the game settings.
 */

static int player_class;

#ifndef CONFIG_GAMES_NXDOOM_NET
static boolean net_client_connected = false;
static boolean drone = false;
#endif

static int frameon;
static int frameskip[4];
static int oldnettics;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The number of tics that have been run (using run_tic) so far. */

int gametic;

/* When set to true, a single tic is run each time try_run_tics() is called.
 * This is used for -timedemo mode.
 */

boolean singletics = false;

/* Reduce the bandwidth needed by sampling game input less and transmitting
 * less.  If ticdup is 2, sample half normal, 3 = one third normal, etc.
 */

int ticdup;

/* Amount to offset the timer for game sync. */

fixed_t offsetms;

int lasttime;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* 35 fps clock adjusted by offsetms milliseconds */

static int get_adjusted_time(void)
{
  int time_ms;

  time_ms = i_get_time_ms();

  if (new_sync)
    {
      /* Use the adjustments from net_client.c only if we are
       * using the new sync mode.
       */

      time_ms += (offsetms / FRACUNIT);
    }

  return (time_ms * TICRATE) / 1000;
}

static boolean build_new_tic(void)
{
  int gameticdiv;
  ticcmd_t cmd;

  gameticdiv = gametic / ticdup;

  i_start_tic();
  loop_interface->process_events();

  /* Always run the menu */

  loop_interface->run_menu();

  if (drone)
    {
      /* In drone mode, do not generate any ticcmds. */

      return false;
    }

  if (new_sync)
    {
      /* If playing single player, do not allow tics to buffer
       * up very far
       */

      if (!net_client_connected && maketic - gameticdiv > 2) return false;

      /* Never go more than ~200ms ahead */

      if (maketic - gameticdiv > 8) return false;
    }
  else
    {
      if (maketic - gameticdiv >= 5) return false;
    }

  /* printf ("mk:%i ",maketic); */

  memset(&cmd, 0, sizeof(ticcmd_t));
  loop_interface->build_ticcmd(&cmd, maketic);

#ifdef CONFIG_GAMES_NXDOOM_NET
  if (net_client_connected)
    {
      net_cl_send_ticcmd(&cmd, maketic);
    }
#endif

  ticdata[maketic % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].cmds[localplayer] =
      cmd;
  ticdata[maketic % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].ingame[localplayer] =
      true;

  ++maketic;

  return true;
}

static boolean strict_demos(void)
{
  /* @category demo
   *
   * When recording or playing back demos, disable any extensions
   * of the vanilla demo format - record demos as vanilla would do,
   * and play back demos as vanilla would do.
   *
   */

  return m_parm_exists("-strictdemos");
}

static void d_disconnected(void)
{
  /* In drone mode, the game cannot continue once disconnected. */

  if (drone)
    {
      i_error("Disconnected from server in drone mode.");
    }

  /* disconnected from server */

  printf("Disconnected from server.\n");
}

static int get_low_tic(void)
{
  int lowtic;

  lowtic = maketic;

  if (net_client_connected)
    {
      if (drone || recvtic < lowtic)
        {
          lowtic = recvtic;
        }
    }

  return lowtic;
}

static void old_net_sync(void)
{
  unsigned int i;
  int keyplayer = -1;

  frameon++;

  /* ideally maketic should be 1 - 3 tics above lowtic
   * if we are consistently slower, speed up time
   */

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; i++)
    {
      if (local_playeringame[i])
        {
          keyplayer = i;
          break;
        }
    }

  if (keyplayer < 0)
    {
      /* If there are no players, we can never advance anyway */

      return;
    }

  if (localplayer == keyplayer)
    {
      /* the key player does not adapt */
    }
  else
    {
      if (maketic <= recvtic)
        {
          lasttime--;
        }

      frameskip[frameon & 3] = oldnettics > recvtic;
      oldnettics = maketic;

      if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
        {
          skiptics = 1;
        }
    }
}

/* Returns true if there are players in the game: */

static boolean players_in_game(void)
{
  boolean result = false;
  unsigned int i;

  /* If we are connected to a server, check if there are any players
   * in the game.
   */

  if (net_client_connected)
    {
      for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
        {
          result = result || local_playeringame[i];
        }
    }

  /* Whether single or multi-player, unless we are running as a drone,
   * we are in the game.
   */

  if (!drone)
    {
      result = true;
    }

  return result;
}

/* When using ticdup, certain values must be cleared out when running
 * the duplicate ticcmds.
 */

static void ticdup_squash(ticcmd_set_t *set)
{
  ticcmd_t *cmd;
  unsigned int i;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      cmd = &set->cmds[i];
      cmd->chatchar = 0;
      if (cmd->buttons & BT_SPECIAL) cmd->buttons = 0;
    }
}

/* When running in single player mode, clear all the ingame[] array
 * except the local player.
 */

static void single_player_clear(ticcmd_set_t *set)
{
  unsigned int i;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (i != localplayer)
        {
          set->ingame[i] = false;
        }
    }
}

/* Returns true if the given lump number corresponds to data from a .lmp
 * file, as opposed to a WAD.
 */

static boolean is_demo_file(int lumpnum)
{
  char *lower;
  boolean result;

  lower = m_string_duplicate(lumpinfo[lumpnum]->wad_file->path);
  m_force_lowercase(lower);
  result = m_string_ends_with(lower, ".lmp");
  free(lower);

  return result;
}

/* Block until the game start message is received from the server. */

#ifdef CONFIG_GAMES_NXDOOM_NET
static void block_until_start(net_gamesettings_t *settings,
                            netgame_startup_callback_t callback)
{
  while (!net_cl_get_settings(settings))
    {
      net_cl_run();
      net_sv_run();

      if (!net_client_connected)
        {
          i_error("Lost connection to server");
        }

      if (callback != NULL && !callback(net_client_wait_data.ready_players,
                                        net_client_wait_data.num_players))
        {
          i_error("Netgame startup aborted.");
        }

      usleep(100000);
    }
}

/* d_quit_net_game
 *
 * Called before quitting to leave a net game without hanging the other
 * players
 *
 * Broadcasts special packets to other players to notify of game exit
 */

static void d_quit_net_game(void)
{
  net_sv_shutdown();
  net_cl_disconnect();
}
#endif /* CONFIG_GAMES_NXDOOM_NET */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* net_update
 * Builds ticcmds for console player, sends out a packet
 */

void net_update(void)
{
  int nowtime;
  int newtics;
  int i;

  /* If we are running with singletics (timing a demo), this
   * is all done separately.
   */

  if (singletics) return;

  /* Run network subsystems */

#ifdef CONFIG_GAMES_NXDOOM_NET
  net_cl_run();
  net_sv_run();
#endif

  /* check time */

  nowtime = get_adjusted_time() / ticdup;
  newtics = nowtime - lasttime;

  lasttime = nowtime;

  if (skiptics <= newtics)
    {
      newtics -= skiptics;
      skiptics = 0;
    }
  else
    {
      skiptics -= newtics;
      newtics = 0;
    }

  /* build new ticcmds for console player */

  for (i = 0; i < newtics; i++)
    {
      if (!build_new_tic())
        {
          break;
        }
    }
}

/* Invoked by the network engine when a complete set of ticcmds is
 * available.
 */

void d_receive_tic(ticcmd_t *ticcmds, boolean *players_mask)
{
  int i;

  /* Disconnected from server? */

  if (ticcmds == NULL && players_mask == NULL)
    {
      d_disconnected();
      return;
    }

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      if (!drone && i == localplayer)
        {
          /* This is us.  Don't overwrite it. */
        }
      else
        {
          ticdata[recvtic % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].cmds[i] =
              ticcmds[i];
          ticdata[recvtic % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS].ingame[i] =
              players_mask[i];
        }
    }

  ++recvtic;
}

/****************************************************************************
 * Name: d_start_game_loop
 *
 * Description:
 *   Called after the screen is set but before the game starts running.
 *
 ****************************************************************************/

void d_start_game_loop(void)
{
  lasttime = get_adjusted_time() / ticdup;
}

void d_start_net_game(net_gamesettings_t *settings,
                      netgame_startup_callback_t callback)
{
  int i;

  offsetms = 0;
  recvtic = 0;

  settings->consoleplayer = 0;
  settings->num_players = 1;
  settings->player_classes[0] = player_class;

  /* @category net
   *
   * Use original network client sync code rather than the improved
   * sync code.
   */

  settings->new_sync = !m_parm_exists("-oldsync");

  /* @category net
   * @arg <n>
   *
   * Send n extra tics in every packet as insurance against dropped
   * packets.
   */

  i = m_check_parm_with_args("-extratics", 1);

  if (i > 0)
    settings->extratics = atoi(myargv[i + 1]);
  else
    settings->extratics = 1;

  /* @category net
   * @arg <n>
   *
   * Reduce the resolution of the game by a factor of n, reducing
   * the amount of network bandwidth needed.
   */

  i = m_check_parm_with_args("-dup", 1);

  if (i > 0)
    settings->ticdup = atoi(myargv[i + 1]);
  else
    settings->ticdup = 1;

#ifdef CONFIG_GAMES_NXDOOM_NET
  if (net_client_connected)
    {
      /* Send our game settings and block until game start is received
       * from the server.
       */

      net_cl_start_game(settings);
      block_until_start(settings, callback);

      /* Read the game settings that were received. */

      net_cl_get_settings(settings);
    }

  if (drone)
    {
      settings->consoleplayer = 0;
    }
#endif

  /* Set the local player and playeringame[] values. */

  localplayer = settings->consoleplayer;

  for (i = 0; i < CONFIG_GAMES_NXDOOM_NET_MAXPLAYERS; ++i)
    {
      local_playeringame[i] = i < settings->num_players;
    }

  /* Copy settings to global variables. */

  ticdup = settings->ticdup;
  new_sync = settings->new_sync;

  if (ticdup < 1)
    {
      i_error("d_start_net_game: invalid ticdup value (%d)", ticdup);
    }

  /* TODO: Message disabled until we fix new_sync.
   *
   * if (!new_sync)
   * {
   *     printf("Syncing netgames like Vanilla Doom.\n");
   * }
   */
}

boolean d_init_net_game(net_connect_data_t *connect_data)
{
#ifdef CONFIG_GAMES_NXDOOM_NET
  boolean result = false;
  net_addr_t *addr = NULL;
  int i;

  /* Call d_quit_net_game on exit: */

  i_at_exit(d_quit_net_game, true);
#endif

  player_class = connect_data->player_class;

#ifdef CONFIG_GAMES_NXDOOM_NET
  /* @category net
   *
   * Start a multiplayer server, listening for connections.
   */

  if (m_check_parm("-server") > 0 || m_check_parm("-privateserver") > 0)
    {
      net_sv_init();
      net_sv_add_module(&net_loop_server_module);
      net_sv_add_module(&net_sdl_module);
      net_sv_register_with_master();

      net_loop_client_module.init_client();
      addr = net_loop_client_module.resolve_address(NULL);
      net_reference_address(addr);
    }
  else
    {
      /* @category net
       *
       * Automatically search the local LAN for a multiplayer
       * server and join it.
       */

      i = m_check_parm("-autojoin");

      if (i > 0)
        {
          addr = net_find_lan_server();

          if (addr == NULL)
            {
              i_error("No server found on local LAN");
            }
        }

      /* @arg <address>
       * @category net
       *
       * Connect to a multiplayer server running on the given
       * address.
       */

      i = m_check_parm_with_args("-connect", 1);

      if (i > 0)
        {
          net_sdl_module.init_client();
          addr = net_sdl_module.resolve_address(myargv[i + 1]);
          net_reference_address(addr);

          if (addr == NULL)
            {
              i_error("Unable to resolve '%s'\n", myargv[i + 1]);
            }
        }
    }

  if (addr != NULL)
    {
      if (m_check_parm("-drone") > 0)
        {
          connect_data->drone = true;
        }

      if (!net_cl_connect(addr, connect_data))
        {
          i_error("d_init_net_game: Failed to connect to %s:\n%s\n",
                  net_addr_to_string(addr), net_client_reject_reason);
        }

      printf("d_init_net_game: Connected to %s\n", net_addr_to_string(addr));
      net_release_address(addr);

      /* Wait for launch message received from server. */

      net_wait_for_launch();

      result = true;
    }

  return result;
#endif
  return false;
}

/* try_run_tics */

void try_run_tics(void)
{
  int i;
  int lowtic;
  int entertic;
  static int oldentertics;
  int realtics;
  int availabletics;
  int counts;

  /* get real tics */

  entertic = i_get_time() / ticdup;
  realtics = entertic - oldentertics;
  oldentertics = entertic;

  /* in singletics mode, run a single tic every time this function
   * is called.
   */

  if (singletics)
    {
      build_new_tic();
    }
  else
    {
      net_update();
    }

  lowtic = get_low_tic();

  availabletics = lowtic - gametic / ticdup;

  /* decide how many tics to run */

  if (new_sync)
    {
      counts = availabletics;
    }
  else
    {
      /* decide how many tics to run */

      if (realtics < availabletics - 1)
        counts = realtics + 1;
      else if (realtics < availabletics)
        counts = realtics;
      else
        counts = availabletics;

      if (counts < 1) counts = 1;

      if (net_client_connected)
        {
          old_net_sync();
        }
    }

  if (counts < 1) counts = 1;

  /* wait for new tics if needed */

  while (!players_in_game() || lowtic < gametic / ticdup + counts)
    {
      net_update();

      lowtic = get_low_tic();

      if (lowtic < gametic / ticdup)
        i_error("try_run_tics: lowtic < gametic");

      /* Still no tics to run? Sleep until some are available. */

      if (lowtic < gametic / ticdup + counts)
        {
          /* If we're in a netgame, we might spin forever waiting for
           * new network data to be received. So don't stay in here
           * forever - give the menu a chance to work.
           */

          if (i_get_time() / ticdup - entertic >= MAX_NETGAME_STALL_TICS)
            {
              return;
            }

          usleep(1000);
        }
    }

  /* run the count * ticdup dics */

  while (counts--)
    {
      ticcmd_set_t *set;

      if (!players_in_game())
        {
          return;
        }

      set = &ticdata[(gametic / ticdup) %
                     CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

      if (!net_client_connected)
        {
          single_player_clear(set);
        }

      for (i = 0; i < ticdup; i++)
        {
          if (gametic / ticdup > lowtic) i_error("gametic>lowtic");

          memcpy(local_playeringame, set->ingame,
                  sizeof(local_playeringame));

          loop_interface->run_tic(set->cmds, set->ingame);
          gametic++;

          /* modify command for duplicated tics */

          ticdup_squash(set);
        }

      net_update(); /* check for new console commands */
    }
}

void d_register_loop_callbacks(loop_interface_t *i)
{
  loop_interface = i;
}

/* If the provided conditional value is true, we're trying to record
 * a demo file that will include a non-vanilla extension. The function
 * will return true if the conditional is true and it's allowed to use
 * this extension (no extensions are allowed if -strictdemos is given
 * on the command line). A warning is shown on the console using the
 * provided string describing the non-vanilla expansion.
 */

boolean d_non_vanilla_record(boolean conditional, const char *feature)
{
  if (!conditional || strict_demos())
    {
      return false;
    }

  printf("Warning: Recording a demo file with a non-vanilla extension "
         "(%s). Use -strictdemos to disable this extension.\n",
         feature);

  return true;
}

/* If the provided conditional value is true, we're trying to play back
 * a demo that includes a non-vanilla extension. We return true if the
 * conditional is true and it's allowed to use this extension, checking
 * that:
 *  - The -strictdemos command line argument is not provided.
 *  - The given lumpnum identifying the demo to play back identifies a
 *    demo that comes from a .lmp file, not a .wad file.
 *  - Before proceeding, a warning is shown to the user on the console.
 */

boolean d_non_vanilla_playback(boolean conditional, int lumpnum,
                             const char *feature)
{
  if (!conditional || strict_demos())
    {
      return false;
    }

  if (!is_demo_file(lumpnum))
    {
      printf("Warning: WAD contains demo with a non-vanilla extension "
             "(%s)\n",
             feature);
      return false;
    }

  printf("Warning: Playing back a demo file with a non-vanilla extension "
         "(%s). Use -strictdemos to disable this extension.\n",
         feature);

  return true;
}
