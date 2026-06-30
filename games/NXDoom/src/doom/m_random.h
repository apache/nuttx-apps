/****************************************************************************
 * apps/games/NXDoom/src/doom/m_random.h
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

#ifndef __M_RANDOM__
#define __M_RANDOM__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Returns a number from 0 to 255, from a lookup table. */

int m_random(void);

/* As m_random, but used only by the play simulation. */

int p_random(void);

/* Fix randoms for demos. */

void m_clear_random(void);

/* Defined version of p_random() - p_random() */

int p_sub_random(void);

#endif /* __M_RANDOM__ */
