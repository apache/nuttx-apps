/****************************************************************************
 * apps/games/NXDoom/src/deh_main.h
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
 * Dehacked entrypoint and common code
 *
 ****************************************************************************/

#ifndef DEH_MAIN_H
#define DEH_MAIN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "deh_str.h"
#include "doomtype.h"
#include "sha1.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* These are the limits that dehacked uses (from dheinit.h in the dehacked
 * source). If these limits are exceeded, it does not generate an error, but
 * a warning is displayed.
 */

#define DEH_VANILLA_NUMSTATES 966
#define DEH_VANILLA_NUMSFX 107

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern boolean deh_allow_extended_strings;
extern boolean deh_allow_long_strings;
extern boolean deh_allow_long_cheats;
extern boolean deh_apply_cheats;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void deh_parse_command_line(void);
int deh_loadfile(const char *filename);
void deh_auto_load_patches(const char *path);
int deh_load_lump(int lumpnum, boolean allow_long, boolean allow_error);
int deh_load_lump_by_name(const char *name, boolean allow_long,
                          boolean allow_error);

boolean deh_parse_assignment(char *line, char **variable_name, char **value);

void deh_checksum(sha1_digest_t digest);

#endif /* DEH_MAIN_H */
