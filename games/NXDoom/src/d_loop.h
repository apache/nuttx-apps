/****************************************************************************
 * apps/games/NXDoom/src/d_loop.h
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
 *  Main loop stuff.
 *
 ****************************************************************************/

#ifndef __D_LOOP__
#define __D_LOOP__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "m_fixed.h"
#include "net_defs.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Callback function invoked while waiting for the netgame to start.
 * The callback is invoked when new players are ready. The callback
 * should return true, or return false to abort startup.
 */

typedef boolean (*netgame_startup_callback_t)(int ready_players,
                                              int num_players);

typedef struct
{
  /* Read events from the event queue, and process them. */

  void (*process_events)(void);

  /* Given the current input state, fill in the fields of the specified
   * ticcmd_t structure with data for a new tic.
   */

  void (*build_ticcmd)(ticcmd_t *cmd, int maketic);

  /* Advance the game forward one tic, using the specified player input. */

  void (*run_tic)(ticcmd_t *cmds, boolean *ingame);

  /* Run the menu (runs independently of the game). */

  void (*run_menu)(void);
} loop_interface_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern boolean singletics;
extern int gametic;
extern int ticdup;
extern fixed_t offsetms;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Register callback functions for the main loop code to use. */

void d_register_loop_callbacks(loop_interface_t *i);

/* Create any new ticcmds and broadcast to other players. */

void net_update(void);

/* ? how many ticks to run? */

void try_run_tics(void);

/* Called at start of game loop to initialize timers */

void d_start_game_loop(void);

/* Initialize networking code and connect to server. */

boolean d_init_net_game(net_connect_data_t *connect_data);

/* Start game with specified settings. The structure will be updated
 * with the actual settings for the game.
 */

void d_start_net_game(net_gamesettings_t *settings,
                    netgame_startup_callback_t callback);

/* Check if it is permitted to record a demo with a non-vanilla feature. */

boolean d_non_vanilla_record(boolean conditional, const char *feature);

/* Check if it is permitted to play back a demo with a non-vanilla feature. */

boolean d_non_vanilla_playback(boolean conditional, int lumpnum,
                             const char *feature);

void d_receive_tic(ticcmd_t *ticcmds, boolean *playeringame);

#endif /* __D_LOOP__ */
