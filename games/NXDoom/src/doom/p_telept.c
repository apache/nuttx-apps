/****************************************************************************
 * apps/games/NXDoom/src/doom/p_telept.c
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
 *  Teleportation.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomdef.h"
#include "doomstat.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "p_local.h"

/* State. */

#include "r_state.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ev_teleport(line_t *line, int side, mobj_t *thing)
{
  int i;
  int tag;
  mobj_t *m;
  mobj_t *fog;
  unsigned an;
  thinker_t *thinker;
  sector_t *sector;
  fixed_t oldx;
  fixed_t oldy;
  fixed_t oldz;

  /* don't teleport missiles */

  if (thing->flags & MF_MISSILE) return 0;

  /* Don't teleport if hit back of line, so you can get out of teleporter. */

  if (side == 1) return 0;

  tag = line->tag;
  for (i = 0; i < numsectors; i++)
    {
      if (sectors[i].tag == tag)
        {
          for (thinker = thinkercap.next; thinker != &thinkercap;
               thinker = thinker->next)
            {
              /* not a mobj */

              if (thinker->function.acp1 != (actionf_p1)p_mobj_thinker)
                continue;

              m = (mobj_t *)thinker;

              /* not a teleportman */

              if (m->type != MT_TELEPORTMAN) continue;

              sector = m->subsector->sector;

              /* wrong sector */

              if (sector - sectors != i) continue;

              oldx = thing->x;
              oldy = thing->y;
              oldz = thing->z;

              if (!p_teleport_move(thing, m->x, m->y)) return 0;

              /* The first Final Doom executable does not set thing->z
               * when teleporting. This quirk is unique to this
               * particular version; the later version included in
               * some versions of the Id Anthology fixed this.
               */

              if (gameversion != exe_final) thing->z = thing->floorz;

              if (thing->player)
                thing->player->viewz = thing->z + thing->player->viewheight;

              /* spawn teleport fog at source and destination */

              fog = p_spawn_mobj(oldx, oldy, oldz, MT_TFOG);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(fog, SFX_TELEPT);
#endif
              an = m->angle >> ANGLETOFINESHIFT;
              fog = p_spawn_mobj(m->x + 20 * finecosine[an],
                                 m->y + 20 * finesine[an],
                                 thing->z, MT_TFOG);

#ifdef CONFIG_GAMES_NXDOOM_SOUND

              /* emit sound, where? */

              s_start_sound(fog, SFX_TELEPT);
#else
              UNUSED(fog);
#endif

              /* don't move for a bit */

              if (thing->player) thing->reactiontime = 18;

              thing->angle = m->angle;
              thing->momx = thing->momy = thing->momz = 0;
              return 1;
            }
        }
    }

  return 0;
}
