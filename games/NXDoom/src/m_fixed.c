/****************************************************************************
 * apps/games/NXDoom/src/m_fixed.c
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
 *  Fixed point implementation.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#include "m_fixed.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Fixme. __USE_C_FIXED__ or something. */

fixed_t fixed_mul(fixed_t a, fixed_t b)
{
  return ((int64_t)a * (int64_t)b) >> FRACBITS;
}

/* fixed_div, C version. */

fixed_t fixed_div(fixed_t a, fixed_t b)
{
  if ((abs(a) >> 14) >= abs(b))
    {
      return (a ^ b) < 0 ? INT_MIN : INT_MAX;
    }
  else
    {
      int64_t result;

      result = ((int64_t)a << FRACBITS) / b;

      return (fixed_t)result;
    }
}
