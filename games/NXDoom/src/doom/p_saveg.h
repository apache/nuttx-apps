/****************************************************************************
 * apps/games/NXDoom/src/doom/p_saveg.h
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
 *  Savegame I/O, archiving, persistence.
 *
 ****************************************************************************/

#ifndef __P_SAVEG__
#define __P_SAVEG__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SAVEGAME_EOF 0x1d
#define VERSIONSIZE 16

/* maximum size of a savegame description */

#define SAVESTRINGSIZE 24

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern FILE *save_stream;
extern boolean savegame_error;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* temporary filename to use while saving. */

char *p_temp_save_game_file(void);

/* filename to use for a savegame slot */

char *p_save_game_file(int slot);

/* Savegame file header read/write functions */

boolean p_read_save_game_header(void);
void p_write_save_game_header(char *description);

/* Savegame end-of-file read/write functions */

boolean p_read_save_game_eof(void);
void p_write_save_game_eof(void);

/* Persistent storage/archiving.
 * These are the load / save game routines.
 */

void p_archive_players(void);
void p_unarchive_players(void);
void p_archive_world(void);
void p_unarchive_world(void);
void p_archive_thinkers(void);
void p_unarchive_thinkers(void);
void p_archive_specials(void);
void p_unarchive_specials(void);

#endif /* __P_SAVEG__ */
