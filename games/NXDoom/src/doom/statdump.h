/****************************************************************************
 * apps/games/NXDoom/src/doom/statdump.h
 *
 * SPDX-License-Identifer: GPLv2
 *
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

#ifndef DOOM_STATDUMP_H
#define DOOM_STATDUMP_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stat_copy(const wbstartstruct_t *stats);
void stat_dump(void);

#endif /* DOOM_STATDUMP_H */
