/****************************************************************************
 * apps/games/NXDoom/src/doom/wi_stuff.h
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
 *  Intermission.
 *
 ****************************************************************************/

#ifndef __WI_STUFF__
#define __WI_STUFF__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomdef.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: wi_ticker
 *
 * Description:
 *  Called by main loop, animate the intermission.
 *
 ****************************************************************************/

void wi_ticker(void);

/****************************************************************************
 * Name: wi_drawer
 *
 * Description:
 *  Called by main loop, draws the intermission directly into the screen
 *  buffer.
 *
 ****************************************************************************/

void wi_drawer(void);

/****************************************************************************
 * Name: wi_start
 *
 * Description:
 *  Setup for an intermission screen.
 *
 ****************************************************************************/

void wi_start(wbstartstruct_t *wbstartstruct);

/****************************************************************************
 * Name: wi_end
 *
 * Description:
 *  Shut down the intermission screen
 *
 ****************************************************************************/

void wi_end(void);

#endif /* __WI_STUFF__ */
