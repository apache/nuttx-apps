/****************************************************************************
 * apps/games/NXDoom/src/doom/st_stuff.h
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
 *  Status bar code.
 *  Does the face/direction indicator animating.
 *  Does palette indicators as well (red pain/berserk, bright pickup)
 *
 ****************************************************************************/

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "doomtype.h"
#include "m_cheat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Size of statusbar.
 * Now sensitive for scaling.
 */

#define ST_HEIGHT 32
#define ST_WIDTH SCREENWIDTH
#define ST_Y (SCREENHEIGHT - ST_HEIGHT)

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern pixel_t *st_backing_screen;
extern cheatseq_t cheat_mus;
extern cheatseq_t cheat_god;
extern cheatseq_t cheat_ammo;
extern cheatseq_t cheat_ammonokey;
extern cheatseq_t cheat_noclip;
extern cheatseq_t cheat_commercial_noclip;
extern cheatseq_t cheat_powerup[7];
extern cheatseq_t cheat_choppers;
extern cheatseq_t cheat_clev;
extern cheatseq_t cheat_mypos;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* STATUS BAR */

/* Called by main loop. */

boolean st_responder(event_t *ev);

/* Called by main loop. */

void st_ticker(void);

/* Called by main loop. */

void st_drawer(boolean fullscreen, boolean refresh);

/* Called when the console player is spawned on each level. */

void st_start(void);

/* Called by startup code. */

void st_init(void);

#endif /* __STSTUFF_H__ */
