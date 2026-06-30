/****************************************************************************
 * apps/games/NXDoom/src/p_rejectpad.h
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
 * Padding of Reject Lump
 *
 ****************************************************************************/

#ifndef __P_REJECTPAD__
#define __P_REJECTPAD__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Padding of Reject Lump */

void pad_reject_array(byte *array, unsigned int len, int totallines);

#endif /* __P_REJECTPAD__ */
