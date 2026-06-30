/****************************************************************************
 * apps/games/NXDoom/src/doom/deh_weapon.c
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
 * Parses "Weapon" sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"

#include "d_items.h"

#include "deh_defs.h"
#include "deh_main.h"
#include "deh_mapping.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

DEH_BEGIN_MAPPING(weapon_mapping, weaponinfo_t)
DEH_MAPPING("Ammo type", ammo)
DEH_MAPPING("Deselect frame", upstate)
DEH_MAPPING("Select frame", downstate)
DEH_MAPPING("Bobbing frame", readystate)
DEH_MAPPING("Shooting frame", atkstate)
DEH_MAPPING("Firing frame", flashstate)
DEH_END_MAPPING

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void *deh_weapon_start(deh_context_t *context, char *line);
static void deh_weapon_parse_line(deh_context_t *context, char *line,
                                  void *tag);
static void deh_weapon_sha1_sum(SHA1_CTX *context);

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_weapon =
{
  "Weapon",
  NULL,
  deh_weapon_start,
  deh_weapon_parse_line,
  NULL,
  deh_weapon_sha1_sum,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *deh_weapon_start(deh_context_t *context, char *line)
{
  int weapon_number = 0;

  if (sscanf(line, "Weapon %i", &weapon_number) != 1)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  if (weapon_number < 0 || weapon_number >= NUMWEAPONS)
    {
      deh_warning(context, "Invalid weapon number: %i", weapon_number);
      return NULL;
    }

  return &weaponinfo[weapon_number];
}

static void deh_weapon_parse_line(deh_context_t *context, char *line,
                                  void *tag)
{
  char *variable_name;
  char *value;
  weaponinfo_t *weapon;
  int ivalue;

  if (tag == NULL) return;

  weapon = (weaponinfo_t *)tag;

  if (!deh_parse_assignment(line, &variable_name, &value))
    {
      /* Failed to parse */

      deh_warning(context, "Failed to parse assignment");
      return;
    }

  ivalue = atoi(value);

  deh_set_mapping(context, &weapon_mapping, weapon, variable_name, ivalue);
}

static void deh_weapon_sha1_sum(SHA1_CTX *context)
{
  int i;

  for (i = 0; i < NUMWEAPONS; ++i)
    {
      deh_struct_sha1_sum(context, &weapon_mapping, &weaponinfo[i]);
    }
}

