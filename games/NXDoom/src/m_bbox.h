/****************************************************************************
 * apps/games/NXDoom/src/m_bbox.h
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
 *    Nil.
 *
 ****************************************************************************/

#ifndef __M_BBOX__
#define __M_BBOX__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <limits.h>

#include "m_fixed.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Bounding box coordinate storage. */

enum
{
  BOXTOP,
  BOXBOTTOM,
  BOXLEFT,
  BOXRIGHT
}; /* bbox coordinates */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Bounding box functions. */

void m_clear_box(fixed_t *box);

void m_add_to_box(fixed_t *box, fixed_t x, fixed_t y);

#endif /* __M_BBOX__ */
