/****************************************************************************
 * apps/games/NXDoom/src/doom/d_main.h
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
 *  System specific interface stuff.
 *
 ****************************************************************************/

#ifndef __D_MAIN__
#define __D_MAIN__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomdef.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern gameaction_t gameaction;
extern boolean advancedemo;

extern const char *pagename;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Read events from all input devices */

void d_process_events(void);

/* BASE LEVEL */

void d_page_ticker(void);
void d_page_drawer(void);
void d_advance_demo(void);
void d_do_advance_demo(void);
void d_start_title(void);

#endif /* __D_MAIN__ */
