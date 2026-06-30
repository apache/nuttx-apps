/****************************************************************************
 * apps/games/NXDoom/src/deh_str.h
 *
 * SPDX-License-Identifier: GPLv2
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
 * Dehacked string replacements
 *
 ****************************************************************************/

#ifndef DEH_STR_H
#define DEH_STR_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Used to do dehacked text substitutions throughout the program */

void deh_add_string_replacement(const char *from_text, const char *to_text);

#endif /* DEH_STR_H */
