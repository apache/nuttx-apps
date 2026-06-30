/****************************************************************************
 * apps/games/NXDoom/src/doom/m_menu.h
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
 *   Menu widget stuff, episode selection and such.
 *
 ****************************************************************************/

#ifndef __M_MENU__
#define __M_MENU__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int g_detail_level;
extern int screenblocks;

extern boolean inhelpscreens;
extern int g_show_messages;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* MENUS
 *
 * Called by main loop,
 * saves config file and calls i_quit when user exits.
 * Even when the menu is not displayed,
 * this can resize the view and change game parameters.
 * Does all the real work of the menu interaction.
 */

boolean m_responder(event_t *ev);

/* Called by main loop, only used for menu (skull cursor) animation. */

void m_ticker(void);

/* Called by main loop, draws the menus directly into the screen buffer. */

void m_drawer(void);

/* Called by d_doom_main, loads the config file. */

void m_init(void);

/* Called by intro code to force menu up upon a keypress, does nothing if
 * menu is already up.
 */

void m_start_control_panel(void);

#endif /* __M_MENU__ */
