/****************************************************************************
 * apps/games/NXDoom/src/doom/deh_sound.c
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
 * Parses "Sound" sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "deh_defs.h"
#include "deh_main.h"
#include "deh_mapping.h"
#include "doomtype.h"
#include "sounds.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

DEH_BEGIN_MAPPING(sound_mapping, sfxinfo_t)
DEH_UNSUPPORTED_MAPPING("Offset")
DEH_UNSUPPORTED_MAPPING("Zero/One")
DEH_MAPPING("Value", priority)
DEH_MAPPING("Zero 1", link)
DEH_MAPPING("Zero 2", pitch)
DEH_MAPPING("Zero 3", volume)
DEH_UNSUPPORTED_MAPPING("Zero 4")
DEH_MAPPING("Neg. One 1", usefulness)
DEH_MAPPING("Neg. One 2", lumpnum)
DEH_END_MAPPING

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void *deh_sound_start(deh_context_t *context, char *line);
static void deh_sound_parse_line(deh_context_t *context, char *line,
                                 void *tag);

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_sound =
{
  "Sound",
  NULL,
  deh_sound_start,
  deh_sound_parse_line,
  NULL,
  NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *deh_sound_start(deh_context_t *context, char *line)
{
  int sound_number = 0;

  if (sscanf(line, "Sound %i", &sound_number) != 1)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  if (sound_number < 0 || sound_number >= SFX_NUMSFX)
    {
      deh_warning(context, "Invalid sound number: %i", sound_number);
      return NULL;
    }

  if (sound_number >= DEH_VANILLA_NUMSFX)
    {
      deh_warning(context,
                  "Attempt to modify SFX %i.  This will cause "
                  "problems in Vanilla dehacked.",
                  sound_number);
    }

  return &s_sfx[sound_number];
}

static void deh_sound_parse_line(deh_context_t *context, char *line,
                                 void *tag)
{
  sfxinfo_t *sfx;
  char *variable_name;
  char *value;
  int ivalue;

  if (tag == NULL) return;

  sfx = (sfxinfo_t *)tag;

  /* Parse the assignment */

  if (!deh_parse_assignment(line, &variable_name, &value))
    {
      /* Failed to parse */

      deh_warning(context, "Failed to parse assignment");
      return;
    }

  /* all values are integers */

  ivalue = atoi(value);

  /* Set the field value */

  deh_set_mapping(context, &sound_mapping, sfx, variable_name, ivalue);
}
