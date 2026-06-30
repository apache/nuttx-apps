/****************************************************************************
 * apps/games/NXDoom/src/v_patch.h
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
 *   Refresh/rendering module, shared data struct definitions.
 *
 ****************************************************************************/

#ifndef V_PATCH_H
#define V_PATCH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/compiler.h>
#include <nuttx/config.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Patches.
 * A patch holds one or more columns.
 * Patches are used for sprites and all masked pictures,
 * and we compose textures from the TEXTURE1/2 lists
 * of patches.
 */

begin_packed_struct struct patch_t
{
  short width; /* bounding box size */
  short height;
  short leftoffset; /* pixels to the left of origin */
  short topoffset;  /* pixels below the origin */
  int columnofs[8]; /* only [width] used the [0] is &columnofs[width] */
} end_packed_struct;

typedef struct patch_t patch_t;

/* posts are runs of non masked source pixels */

begin_packed_struct struct post_t
{
  byte topdelta; /* -1 is the last post in a column */
  byte length;   /* length data bytes follows */
} end_packed_struct;

typedef struct post_t post_t;

/* column_t is a list of 0 or more post_t, (byte)-1 terminated */

typedef post_t column_t;

#endif /* V_PATCH_H */
