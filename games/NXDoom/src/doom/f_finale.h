/****************************************************************************
 * apps/games/NXDoom/src/doom/f_finale.h
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
 ****************************************************************************/

#ifndef __F_FINALE__
#define __F_FINALE__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "doomtype.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Called by main loop. */

boolean f_responder(event_t *ev);

/* Called by main loop. */

void f_ticker(void);

/* Called by main loop. */

void f_drawer(void);

void f_start_finale(void);

#endif /* __F_FINALE__ */
