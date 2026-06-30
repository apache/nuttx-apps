/****************************************************************************
 * apps/games/NXDoom/src/m_fixed.h
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
 *  Fixed point arithemtics, implementation.
 *
 ****************************************************************************/

#ifndef __M_FIXED__
#define __M_FIXED__

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Fixed point, 32bit as 16.16. */

#define FRACBITS 16
#define FRACUNIT (1 << FRACBITS)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef int fixed_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

fixed_t fixed_mul(fixed_t a, fixed_t b);
fixed_t fixed_div(fixed_t a, fixed_t b);

#endif /* __M_FIXED__ */
