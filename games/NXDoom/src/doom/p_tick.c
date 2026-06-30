/****************************************************************************
 * apps/games/NXDoom/src/doom/p_tick.c
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
 *  Archiving: SaveGame I/O.
 *  Thinker, Ticker.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "p_local.h"
#include "z_zone.h"

#include "doomstat.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

int leveltime;

/* THINKERS
 * All thinkers should be allocated by z_malloc
 * so they can be operated on uniformly.
 * The actual structures will vary in size,
 * but the first element must be thinker_t.
 */

/* Both the head and tail of the thinker list. */

thinker_t thinkercap;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void p_run_thinkers(void)
{
  thinker_t *currentthinker, *nextthinker;

  currentthinker = thinkercap.next;
  while (currentthinker != &thinkercap)
    {
      if (currentthinker->function.acv == (actionf_v)(-1))
        {
          /* time to remove it */

          nextthinker = currentthinker->next;
          currentthinker->next->prev = currentthinker->prev;
          currentthinker->prev->next = currentthinker->next;
          z_free(currentthinker);
        }
      else
        {
          if (currentthinker->function.acp1)
            currentthinker->function.acp1(currentthinker);
          nextthinker = currentthinker->next;
        }

      currentthinker = nextthinker;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void p_init_thinkers(void)
{
  thinkercap.prev = thinkercap.next = &thinkercap;
}

/* p_add_thinker
 * Adds a new thinker at the end of the list.
 */

void p_add_thinker(thinker_t *thinker)
{
  thinkercap.prev->next = thinker;
  thinker->next = &thinkercap;
  thinker->prev = thinkercap.prev;
  thinkercap.prev = thinker;
}

/* p_remove_thinker Deallocation is lazy -- it will not actually be freed
 * until its thinking turn comes up.
 */

void p_remove_thinker(thinker_t *thinker)
{
  /* FIXME: NOP. */

  thinker->function.acv = (actionf_v)(-1);
}

void p_ticker(void)
{
  int i;

  /* run the tic */

  if (paused) return;

  /* pause if in menu and at least one tic has been run */

  if (!netgame && g_menuactive && !demoplayback &&
      players[consoleplayer].viewz != 1)
    {
      return;
    }

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
          p_player_think(&players[i]);
        }
    }

  p_run_thinkers();
  p_update_specials();
  p_respawn_specials();

  /* for par times */

  leveltime++;
}
