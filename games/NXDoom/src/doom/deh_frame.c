/****************************************************************************
 * apps/games/NXDoom/src/doom/deh_frame.c
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
 * Parses "Frame" sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "d_items.h"
#include "doomtype.h"
#include "info.h"

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"
#include "deh_mapping.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

DEH_BEGIN_MAPPING(state_mapping, state_t)
DEH_MAPPING("Sprite number", sprite)
DEH_MAPPING("Sprite subnumber", frame)
DEH_MAPPING("Duration", tics)
DEH_MAPPING("Next frame", nextstate)
#if 0
DEH_MAPPING("Unknown 1", misc1)
DEH_MAPPING("Unknown 2", misc2)
#endif
DEH_UNSUPPORTED_MAPPING("Codep frame")
DEH_END_MAPPING

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void *deh_frame_start(deh_context_t *context, char *line);
static void deh_frame_overflow(deh_context_t *context, char *varname,
                              int value);
static void deh_frame_parse_line(deh_context_t *context, char *line,
                                 void *tag);
static void deh_frame_sha1_sum(SHA1_CTX *context);

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_frame =
{
  "Frame",
  NULL,
  deh_frame_start,
  deh_frame_parse_line,
  NULL,
  deh_frame_sha1_sum,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *deh_frame_start(deh_context_t *context, char *line)
{
  int frame_number = 0;
  state_t *state;

  if (sscanf(line, "Frame %i", &frame_number) != 1)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  if (frame_number < 0 || frame_number >= NUMSTATES)
    {
      deh_warning(context, "Invalid frame number: %i", frame_number);
      return NULL;
    }

  if (frame_number >= DEH_VANILLA_NUMSTATES)
    {
      deh_warning(context,
                  "Attempt to modify frame %i: this will cause "
                  "problems in Vanilla dehacked.",
                  frame_number);
    }

  state = &states[frame_number];

  return state;
}

/* Simulate a frame overflow: Doom has 967 frames in the states[] array, but
 * DOS dehacked internally only allocates memory for 966.  As a result,
 * attempts to set frame 966 (the last frame) will overflow the dehacked
 * array and overwrite the weaponinfo[] array instead.
 *
 * This is noticeable in Batman Doom where it is impossible to switch weapons
 * away from the fist once selected.
 */

static void deh_frame_overflow(deh_context_t *context, char *varname,
                              int value)
{
  if (!strcasecmp(varname, "Duration"))
    {
      weaponinfo[0].ammo = value;
    }
  else if (!strcasecmp(varname, "Codep frame"))
    {
      weaponinfo[0].upstate = value;
    }
  else if (!strcasecmp(varname, "Next frame"))
    {
      weaponinfo[0].downstate = value;
    }
  else if (!strcasecmp(varname, "Unknown 1"))
    {
      weaponinfo[0].readystate = value;
    }
  else if (!strcasecmp(varname, "Unknown 2"))
    {
      weaponinfo[0].atkstate = value;
    }
  else
    {
      deh_error(context, "Unable to simulate frame overflow: field '%s'",
                varname);
    }
}

static void deh_frame_parse_line(deh_context_t *context, char *line,
                                 void *tag)
{
  state_t *state;
  char *variable_name;
  char *value;
  int ivalue;

  if (tag == NULL) return;

  state = (state_t *)tag;

  /* Parse the assignment */

  if (!deh_parse_assignment(line, &variable_name, &value))
    {
      /* Failed to parse */

      deh_warning(context, "Failed to parse assignment");
      return;
    }

  /* all values are integers */

  ivalue = atoi(value);

  if (state == &states[NUMSTATES - 1])
    {
      deh_frame_overflow(context, variable_name, ivalue);
    }
  else
    {
      /* set the appropriate field */

      deh_set_mapping(context, &state_mapping, state, variable_name, ivalue);
    }
}

static void deh_frame_sha1_sum(SHA1_CTX *context)
{
  int i;

  for (i = 0; i < NUMSTATES; ++i)
    {
      deh_struct_sha1_sum(context, &state_mapping, &states[i]);
    }
}
