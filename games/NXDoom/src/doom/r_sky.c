/****************************************************************************
 * apps/games/NXDoom/src/doom/r_sky.c
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
 *  Sky rendering. The DOOM sky is a texture map like any
 *  wall, wrapping around. A 1024 columns equal 360 degrees.
 *  The default sky map is 256 columns and repeats 4 times
 *  on a 320 screen?
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Needed for FRACUNIT. */

#include "m_fixed.h"

/* Needed for Flat retrieval. */

#include "r_data.h"

#include "r_sky.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* sky mapping */

int skyflatnum;
int skytexture;
int skytexturemid;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: r_init_sky_map
 *
 * Description:
 *  Called whenever the view size changes.
 *
 ****************************************************************************/

void r_init_sky_map(void)
{
  /* skyflatnum = r_flat_num_for_name ( SKYFLATNAME ); */

  skytexturemid = SCREENHEIGHT / 2 * FRACUNIT;
}
