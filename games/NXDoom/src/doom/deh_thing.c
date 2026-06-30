/****************************************************************************
 * apps/games/NXDoom/src/doom/deh_thing.c
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
 * Parses "Thing" sections in dehacked files
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"

#include "deh_defs.h"
#include "deh_main.h"
#include "deh_mapping.h"

#include "info.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

DEH_BEGIN_MAPPING(thing_mapping, mobjinfo_t)
DEH_MAPPING("Bits", flags)
DEH_MAPPING("Width", radius)
DEH_MAPPING("Height", height)
DEH_MAPPING("Mass", mass)
DEH_MAPPING("Speed", speed)
DEH_MAPPING("ID #", doomednum)
DEH_MAPPING("Hit points", spawnhealth)
DEH_MAPPING("Initial frame", spawnstate)
DEH_MAPPING("First moving frame", seestate)
DEH_MAPPING("Injury frame", painstate)
DEH_MAPPING("Pain chance", painchance)
DEH_MAPPING("Close attack frame", meleestate)
DEH_MAPPING("Far attack frame", missilestate)
DEH_MAPPING("Death frame", deathstate)
DEH_MAPPING("Exploding frame", xdeathstate)
DEH_MAPPING("Respawn frame", raisestate)
DEH_MAPPING("Missile damage", damage)
#ifdef CONFIG_GAMES_NXDOOM_SOUND
DEH_MAPPING("Reaction time", reactiontime)
DEH_MAPPING("Alert sound", seesound)
DEH_MAPPING("Attack sound", attacksound)
DEH_MAPPING("Pain sound", painsound)
DEH_MAPPING("Death sound", deathsound)
DEH_MAPPING("Action sound", activesound)
#endif
DEH_END_MAPPING

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void *deh_thing_start(deh_context_t *context, char *line);
static void deh_thing_parse_line(deh_context_t *context, char *line,
                                 void *tag);
static void deh_thing_sha1_sum(SHA1_CTX *context);

/****************************************************************************
 * Public Data
 ****************************************************************************/

deh_section_t deh_section_thing =
{
  "Thing",
  NULL,
  deh_thing_start,
  deh_thing_parse_line,
  NULL,
  deh_thing_sha1_sum,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *deh_thing_start(deh_context_t *context, char *line)
{
  int thing_number = 0;
  mobjinfo_t *mobj;

  if (sscanf(line, "Thing %i", &thing_number) != 1)
    {
      deh_warning(context, "Parse error on section start");
      return NULL;
    }

  /* dehacked files are indexed from 1 */

  --thing_number;

  if (thing_number < 0 || thing_number >= NUMMOBJTYPES)
    {
      deh_warning(context, "Invalid thing number: %i", thing_number);
      return NULL;
    }

  mobj = &mobjinfo[thing_number];

  return mobj;
}

static void deh_thing_parse_line(deh_context_t *context, char *line,
                                 void *tag)
{
  mobjinfo_t *mobj;
  char *variable_name;
  char *value;
  int ivalue;

  if (tag == NULL) return;

  mobj = (mobjinfo_t *)tag;

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

  deh_set_mapping(context, &thing_mapping, mobj, variable_name, ivalue);
}

static void deh_thing_sha1_sum(SHA1_CTX *context)
{
  int i;

  for (i = 0; i < NUMMOBJTYPES; ++i)
    {
      deh_struct_sha1_sum(context, &thing_mapping, &mobjinfo[i]);
    }
}
