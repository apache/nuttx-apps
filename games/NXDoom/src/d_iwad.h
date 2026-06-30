/****************************************************************************
 * apps/games/NXDoom/src/d_iwad.h
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
 * DESCRIPTION:
 *   Find IWAD and initialize according to IWAD type.
 *
 ****************************************************************************/

#ifndef __D_IWAD__
#define __D_IWAD__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_mode.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IWAD_MASK_DOOM                                                       \
  ((1 << doom) | (1 << doom2) | (1 << pack_tnt) | (1 << pack_plut) |         \
   (1 << pack_chex) | (1 << pack_hacx))
#define IWAD_MASK_HERETIC (1 << heretic)
#define IWAD_MASK_HEXEN (1 << hexen)
#define IWAD_MASK_STRIFE (1 << strife)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct
{
  const char *name;
  gamemission_t mission;
  game_mode_t mode;
  const char *description;
} iwad_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

char *d_find_wad_by_name(const char *filename);
char *d_try_find_wad_by_name(const char *filename);
char *d_find_iwad(int mask, gamemission_t *mission);
const iwad_t **d_find_all_iwads(int mask);
const char *d_save_game_iwad_name(gamemission_t gamemission,
                               game_variant_t gamevariant);
const char *d_suggest_iwad_name(gamemission_t mission, game_mode_t mode);
const char *d_suggest_game_name(gamemission_t mission, game_mode_t mode);

#endif /* __D_IWAD__ */
