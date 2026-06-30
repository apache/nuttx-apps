/****************************************************************************
 * apps/games/NXDoom/src/doom/p_lights.c
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
 *  Handle Sector base lighting effects.
 *  Muzzle flash?
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "m_random.h"
#include "z_zone.h"

#include "doomdef.h"
#include "p_local.h"

/* State. */

#include "r_state.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* FIRELIGHT FLICKER */

/* t_fire_flicker */

static void t_fire_flicker(fireflicker_t *flick)
{
  int amount;

  if (--flick->count) return;

  amount = (p_random() & 3) * 16;

  if (flick->sector->lightlevel - amount < flick->minlight)
    flick->sector->lightlevel = flick->minlight;
  else
    flick->sector->lightlevel = flick->maxlight - amount;

  flick->count = 4;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void p_spawn_fire_flicker(sector_t *sector)
{
  fireflicker_t *flick;

  /* Note that we are resetting sector attributes. Nothing special about it
   * during gameplay.
   */

  sector->special = 0;

  flick = z_malloc(sizeof(*flick), PU_LEVSPEC, 0);

  p_add_thinker(&flick->thinker);

  flick->thinker.function.acp1 = (actionf_p1)t_fire_flicker;
  flick->sector = sector;
  flick->maxlight = sector->lightlevel;
  flick->minlight = p_find_min_surrounding(sector, sector->lightlevel) + 16;
  flick->count = 4;
}

/* BROKEN LIGHT FLASHING */

/* t_light_flash
 * Do flashing lights.
 */

void t_light_flash(lightflash_t *flash)
{
  if (--flash->count) return;

  if (flash->sector->lightlevel == flash->maxlight)
    {
      flash->sector->lightlevel = flash->minlight;
      flash->count = (p_random() & flash->mintime) + 1;
    }
  else
    {
      flash->sector->lightlevel = flash->maxlight;
      flash->count = (p_random() & flash->maxtime) + 1;
    }
}

/* p_spawn_light_flash
 * After the map has been loaded, scan each sector for specials that spawn
 * thinkers
 */

void p_spawn_light_flash(sector_t *sector)
{
  lightflash_t *flash;

  /* nothing special about it during gameplay */

  sector->special = 0;

  flash = z_malloc(sizeof(*flash), PU_LEVSPEC, 0);

  p_add_thinker(&flash->thinker);

  flash->thinker.function.acp1 = (actionf_p1)t_light_flash;
  flash->sector = sector;
  flash->maxlight = sector->lightlevel;

  flash->minlight = p_find_min_surrounding(sector, sector->lightlevel);
  flash->maxtime = 64;
  flash->mintime = 7;
  flash->count = (p_random() & flash->maxtime) + 1;
}

/* STROBE LIGHT FLASHING */

void t_strobe_flash(strobe_t *flash)
{
  if (--flash->count) return;

  if (flash->sector->lightlevel == flash->minlight)
    {
      flash->sector->lightlevel = flash->maxlight;
      flash->count = flash->brighttime;
    }
  else
    {
      flash->sector->lightlevel = flash->minlight;
      flash->count = flash->darktime;
    }
}

/* p_spawn_strobe_flash
 * After the map has been loaded, scan each sector for specials that spawn
 * thinkers
 */

void p_spawn_strobe_flash(sector_t *sector, int fast_or_slow, int in_sync)
{
  strobe_t *flash;

  flash = z_malloc(sizeof(*flash), PU_LEVSPEC, 0);

  p_add_thinker(&flash->thinker);

  flash->sector = sector;
  flash->darktime = fast_or_slow;
  flash->brighttime = STROBEBRIGHT;
  flash->thinker.function.acp1 = (actionf_p1)t_strobe_flash;
  flash->maxlight = sector->lightlevel;
  flash->minlight = p_find_min_surrounding(sector, sector->lightlevel);

  if (flash->minlight == flash->maxlight) flash->minlight = 0;

  /* nothing special about it during gameplay */

  sector->special = 0;

  if (!in_sync)
    flash->count = (p_random() & 7) + 1;
  else
    flash->count = 1;
}

/* Start strobing lights (usually from a trigger) */

void ev_start_light_strobing(line_t *line)
{
  int secnum;
  sector_t *sec;

  secnum = -1;
  while ((secnum = p_find_sector_from_line_tag(line, secnum)) >= 0)
    {
      sec = &sectors[secnum];
      if (sec->specialdata) continue;

      p_spawn_strobe_flash(sec, SLOWDARK, 0);
    }
}

/* TURN LINE'S TAG LIGHTS OFF */

void ev_turn_tag_lights_off(line_t *line)
{
  int i;
  int j;
  int min;
  sector_t *sector;
  sector_t *tsec;
  line_t *templine;

  sector = sectors;

  for (j = 0; j < numsectors; j++, sector++)
    {
      if (sector->tag == line->tag)
        {
          min = sector->lightlevel;
          for (i = 0; i < sector->linecount; i++)
            {
              templine = sector->lines[i];
              tsec = get_next_sector(templine, sector);
              if (!tsec) continue;
              if (tsec->lightlevel < min) min = tsec->lightlevel;
            }

          sector->lightlevel = min;
        }
    }
}

/* TURN LINE'S TAG LIGHTS ON */

void ev_light_turn_on(line_t *line, int bright)
{
  int i;
  int j;
  sector_t *sector;
  sector_t *temp;
  line_t *templine;

  sector = sectors;

  for (i = 0; i < numsectors; i++, sector++)
    {
      if (sector->tag == line->tag)
        {
          /* bright = 0 means to search for highest light level surrounding
           * sector
           */

          if (!bright)
            {
              for (j = 0; j < sector->linecount; j++)
                {
                  templine = sector->lines[j];
                  temp = get_next_sector(templine, sector);

                  if (!temp) continue;

                  if (temp->lightlevel > bright) bright = temp->lightlevel;
                }
            }

          sector->lightlevel = bright;
        }
    }
}

/* Spawn glowing light */

void t_glow(glow_t *g)
{
  switch (g->direction)
    {
    case -1: /* DOWN */
      g->sector->lightlevel -= GLOWSPEED;
      if (g->sector->lightlevel <= g->minlight)
        {
          g->sector->lightlevel += GLOWSPEED;
          g->direction = 1;
        }
      break;

    case 1: /* UP */
      g->sector->lightlevel += GLOWSPEED;
      if (g->sector->lightlevel >= g->maxlight)
        {
          g->sector->lightlevel -= GLOWSPEED;
          g->direction = -1;
        }
      break;
    }
}

void p_spawn_glowing_light(sector_t *sector)
{
  glow_t *g;

  g = z_malloc(sizeof(*g), PU_LEVSPEC, 0);

  p_add_thinker(&g->thinker);

  g->sector = sector;
  g->minlight = p_find_min_surrounding(sector, sector->lightlevel);
  g->maxlight = sector->lightlevel;
  g->thinker.function.acp1 = (actionf_p1)t_glow;
  g->direction = -1;

  sector->special = 0;
}
