/****************************************************************************
 * apps/games/NXDoom/src/i_timer.h
 *
 * SPDX-License-Identifier: GPLv2
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
 *      System-specific timer interface
 *
 ****************************************************************************/

#ifndef __I_TIMER__
#define __I_TIMER__

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define TICRATE 35

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: i_get_time
 *
 * Description:
 *  Called by d_doomloop.
 *
 * Returns:
 *  The current time in tics.
 ****************************************************************************/

int i_get_time(void);

/****************************************************************************
 * Name: i_get_time_ms
 *
 * Returns:
 *  The current time in ms.
 ****************************************************************************/

int i_get_time_ms(void);

/****************************************************************************
 * Name: i_init_timer
 *
 * Description:
 *  Initialize timer.
 ****************************************************************************/

void i_init_timer(void);

/****************************************************************************
 * Name: i_wait_vbl
 *
 * Description:
 *   Wait for vertical retrace or pause a bit.
 ****************************************************************************/

void i_wait_vbl(int count);

#endif /* __I_TIMER__ */
