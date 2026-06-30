/****************************************************************************
 * apps/games/NXDoom/src/w_wad.h
 *
 * SPDX-License-Identifier: GPLv2
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
 *  WAD I/O functions.
 *
 ****************************************************************************/

#ifndef __W_WAD__
#define __W_WAD__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "doomtype.h"
#include "w_file.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* WADFILE I/O related stuff. */

typedef struct lumpinfo_s lumpinfo_t;
typedef int lumpindex_t;

struct lumpinfo_s
{
  char name[8];
  wad_file_t *wad_file;
  int position;
  int size;
  void *cache;

  /* Used for hash table lookups */

  lumpindex_t next;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern lumpinfo_t **lumpinfo;
extern unsigned int numlumps;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

wad_file_t *w_add_file(const char *filename);
void w_reload(void);

lumpindex_t w_check_num_for_name(const char *name);
lumpindex_t w_get_num_for_name(const char *name);

int w_lump_length(lumpindex_t lump);
void w_read_lump(lumpindex_t lump, void *dest);

void *w_cache_lump_num(lumpindex_t lump, int tag);
void *w_cache_lump_name(const char *name, int tag);

void w_generate_hash_table(void);

extern unsigned int w_lump_name_hash(const char *s);

void w_release_lump_num(lumpindex_t lump);
void w_release_lump_name(const char *name);

const char *w_wad_name_for_lump(const lumpinfo_t *lump);
boolean w_is_iwad_lump(const lumpinfo_t *lump);

#endif /* __W_WAD_H__ */
