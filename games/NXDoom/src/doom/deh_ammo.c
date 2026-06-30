/****************************************************************************
 * apps/games/NXDoom/src/doom/deh_ammo.c
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
 * Parses "Ammo" sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"
#include "doomdef.h"
#include "doomtype.h"
#include "p_local.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void *deh_ammo_start(deh_context_t *context, char *line);
static void deh_ammo_parse_line(deh_context_t *context, char *line,
                                void *tag);
static void deh_ammo_sha1_hash(SHA1_CTX *context);

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_ammo =
{
  "Ammo",
  NULL,
  deh_ammo_start,
  deh_ammo_parse_line,
  NULL,
  deh_ammo_sha1_hash,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *deh_ammo_start(deh_context_t *context, char *line)
{
  int ammo_number = 0;

  if (sscanf(line, "Ammo %i", &ammo_number) != 1)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  if (ammo_number < 0 || ammo_number >= NUMAMMO)
    {
      deh_warning(context, "Invalid ammo number: %i", ammo_number);
      return NULL;
    }

  return &maxammo[ammo_number];
}

static void deh_ammo_parse_line(deh_context_t *context, char *line,
        void *tag)
{
  char *variable_name;
  char *value;
  int ivalue;
  int ammo_number;

  if (tag == NULL) return;

  ammo_number = ((int *)tag) - maxammo;

  /* Parse the assignment */

  if (!deh_parse_assignment(line, &variable_name, &value))
    {
      /* Failed to parse */

      deh_warning(context, "Failed to parse assignment");
      return;
    }

  ivalue = atoi(value);

  /* maxammo */

  if (!strcasecmp(variable_name, "Per ammo"))
    clipammo[ammo_number] = ivalue;
  else if (!strcasecmp(variable_name, "Max ammo"))
    maxammo[ammo_number] = ivalue;
  else
    {
      deh_warning(context, "Field named '%s' not found", variable_name);
    }
}

static void deh_ammo_sha1_hash(SHA1_CTX *context)
{
  int i;

  for (i = 0; i < NUMAMMO; ++i)
    {
      sha1_updateint32(context, clipammo[i]);
      sha1_updateint32(context, maxammo[i]);
    }
}
