/****************************************************************************
 * apps/games/NXDoom/src/doom/g_game.h
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
 *   Duh.
 *
 ****************************************************************************/

#ifndef __G_GAME__
#define __G_GAME__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "d_ticcmd.h"
#include "doomdef.h"
#include "m_fixed.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int vanilla_savegame_limit;
extern int vanilla_demo_limit;

extern fixed_t forwardmove[2];
extern fixed_t sidemove[2];

extern boolean sendpause;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* GAME */

void g_death_match_spawn_player(int playernum);

void g_init_new(skill_t skill, int episode, int map);

/* Can be called by the startup code or m_responder.
 * A normal game starts at map 1,
 * but a warp test can start elsewhere
 */

void g_deferred_init_new(skill_t skill, int episode, int map);

void g_defered_play_demo(const char *demo);

/* Can be called by the startup code or m_responder,
 * calls p_setup_level or W_EnterWorld.
 */

void g_load_game(char *name);

void g_do_load_game(void);

/* Called by m_responder. */

void g_save_game(int slot, char *description);

/* Only called by startup code. */

void g_record_demo(const char *name);

void g_begin_recording(void);

void g_play_demo(char *name);
void g_time_demo(char *name);
boolean g_check_demo_status(void);

void g_exit_level(void);
void g_secret_exit_level(void);

void g_world_done(void);

/* Read current data from inputs and build a player movement command. */

void g_build_ticcmd(ticcmd_t *cmd, int maketic);

void g_ticker(void);
boolean g_responder(event_t *ev);

void g_screenshot(void);

int g_vanilla_version_code(void);

#endif /* __G_GAME__ */
