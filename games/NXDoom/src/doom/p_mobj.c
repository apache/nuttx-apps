/****************************************************************************
 * apps/games/NXDoom/src/doom/p_mobj.c
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
 *  Moving object handling. Spawn functions.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "i_system.h"
#include "m_random.h"
#include "z_zone.h"

#include "doomdef.h"
#include "p_local.h"

#include "hu_stuff.h"
#include "st_stuff.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "doomstat.h"

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Use a heuristic approach to detect infinite state cycles: Count the number
 * of times the loop in p_set_mobj_state() executes and exit with an error
 * once an arbitrary very large limit is reached.
 */

#define MOBJ_CYCLE_LIMIT 1000000

#define STOPSPEED 0x1000
#define FRICTION 0xe800

/****************************************************************************
 * Public Data
 ****************************************************************************/

int test;

mapthing_t itemrespawnque[ITEMQUESIZE];
int itemrespawntime[ITEMQUESIZE];
int iquehead;
int iquetail;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void g_player_reborn(int player);
void p_spawn_map_thing(mapthing_t *mthing);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: p_explode_missile
 ****************************************************************************/

static void p_explode_missile(mobj_t *mo)
{
  mo->momx = mo->momy = mo->momz = 0;

  p_set_mobj_state(mo, mobjinfo[mo->type].deathstate);

  mo->tics -= p_random() & 3;

  if (mo->tics < 1) mo->tics = 1;

  mo->flags &= ~MF_MISSILE;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (mo->info->deathsound) s_start_sound(mo, mo->info->deathsound);
#endif
}

/****************************************************************************
 * Name: p_xy_movement
 ****************************************************************************/

static void p_xy_movement(mobj_t *mo)
{
  fixed_t ptryx;
  fixed_t ptryy;
  player_t *player;
  fixed_t xmove;
  fixed_t ymove;

  if (!mo->momx && !mo->momy)
    {
      if (mo->flags & MF_SKULLFLY)
        {
          /* the skull slammed into something */

          mo->flags &= ~MF_SKULLFLY;
          mo->momx = mo->momy = mo->momz = 0;

          p_set_mobj_state(mo, mo->info->spawnstate);
        }

      return;
    }

  player = mo->player;

  if (mo->momx > MAXMOVE)
    mo->momx = MAXMOVE;
  else if (mo->momx < -MAXMOVE)
    mo->momx = -MAXMOVE;

  if (mo->momy > MAXMOVE)
    mo->momy = MAXMOVE;
  else if (mo->momy < -MAXMOVE)
    mo->momy = -MAXMOVE;

  xmove = mo->momx;
  ymove = mo->momy;

  do
    {
      if (xmove > MAXMOVE / 2 || ymove > MAXMOVE / 2)
        {
          ptryx = mo->x + xmove / 2;
          ptryy = mo->y + ymove / 2;
          xmove >>= 1;
          ymove >>= 1;
        }
      else
        {
          ptryx = mo->x + xmove;
          ptryy = mo->y + ymove;
          xmove = ymove = 0;
        }

      if (!p_try_move(mo, ptryx, ptryy))
        {
          /* blocked move */

          if (mo->player)
            {
              /* try to slide along it */

              p_slide_move(mo);
            }
          else if (mo->flags & MF_MISSILE)
            {
              /* explode a missile */

              if (ceilingline && ceilingline->backsector &&
                  ceilingline->backsector->ceilingpic == skyflatnum)
                {
                  /* Hack to prevent missiles exploding
                   * against the sky.
                   * Does not handle sky floors.
                   */

                  p_remove_mobj(mo);
                  return;
                }

              p_explode_missile(mo);
            }
          else
            mo->momx = mo->momy = 0;
        }
    }
  while (xmove || ymove);

  /* slow down */

  if (player && player->cheats & CF_NOMOMENTUM)
    {
      /* debug option for no sliding at all */

      mo->momx = mo->momy = 0;
      return;
    }

  if (mo->flags & (MF_MISSILE | MF_SKULLFLY))
    return; /* no friction for missiles ever */

  if (mo->z > mo->floorz) return; /* no friction when airborne */

  if (mo->flags & MF_CORPSE)
    {
      /* do not stop sliding
       *  if halfway off a step with some momentum
       */

      if (mo->momx > FRACUNIT / 4 || mo->momx < -FRACUNIT / 4 ||
          mo->momy > FRACUNIT / 4 || mo->momy < -FRACUNIT / 4)
        {
          if (mo->floorz != mo->subsector->sector->floorheight) return;
        }
    }

  if (mo->momx > -STOPSPEED && mo->momx < STOPSPEED &&
      mo->momy > -STOPSPEED && mo->momy < STOPSPEED &&
      (!player ||
       (player->cmd.forwardmove == 0 && player->cmd.sidemove == 0)))
    {
      /* if in a walking frame, stop moving */

      if (player &&
          (unsigned)((player->mo->state - states) - S_PLAY_RUN1) < 4)
        p_set_mobj_state(player->mo, S_PLAY);

      mo->momx = 0;
      mo->momy = 0;
    }
  else
    {
      mo->momx = fixed_mul(mo->momx, FRICTION);
      mo->momy = fixed_mul(mo->momy, FRICTION);
    }
}

/****************************************************************************
 * Name: p_z_movement
 ****************************************************************************/

static void p_z_movement(mobj_t *mo)
{
  fixed_t dist;
  fixed_t delta;

  /* check for smooth step up */

  if (mo->player && mo->z < mo->floorz)
    {
      mo->player->viewheight -= mo->floorz - mo->z;

      mo->player->deltaviewheight =
          (VIEWHEIGHT - mo->player->viewheight) >> 3;
    }

  mo->z += mo->momz; /* adjust height */

  if (mo->flags & MF_FLOAT && mo->target)
    {
      /* float down towards target if too close */

      if (!(mo->flags & MF_SKULLFLY) && !(mo->flags & MF_INFLOAT))
        {
          dist = p_approx_distance(mo->x - mo->target->x,
                  mo->y - mo->target->y);

          delta = (mo->target->z + (mo->height >> 1)) - mo->z;

          if (delta < 0 && dist < -(delta * 3))
            mo->z -= FLOATSPEED;
          else if (delta > 0 && dist < (delta * 3))
            mo->z += FLOATSPEED;
        }
    }

  /* clip movement */

  if (mo->z <= mo->floorz)
    {
      /* hit the floor */

      /* Note (id):
       *  somebody left this after the setting momz to 0,
       *  kinda useless there.
       *
       * cph - This was the a bug in the linuxdoom-1.10 source which
       *  caused it not to sync Doom 2 v1.9 demos. Someone
       *  added the above comment and moved up the following code. So
       *  demos would desync in close lost soul fights.
       * Note that this only applies to original Doom 1 or Doom2 demos - not
       *  Final Doom and Ultimate Doom.  So we test demo_compatibility *and*
       *  gamemission. (Note we assume that Doom1 is always Ult Doom, which
       *  seems to hold for most published demos.)
       *
       *  fraggle - cph got the logic here slightly wrong.  There are three
       *  versions of Doom 1.9:
       *
       *  * The version used in registered doom 1.9 + doom2 - no bounce
       *  * The version used in ultimate doom - has bounce
       *  * The version used in final doom - has bounce
       *
       * So we need to check that this is either retail or commercial
       * (but not doom2)
       */

      int correct_lost_soul_bounce = gameversion >= exe_ultimate;

      if (correct_lost_soul_bounce && mo->flags & MF_SKULLFLY)
        {
          /* the skull slammed into something */

          mo->momz = -mo->momz;
        }

      if (mo->momz < 0)
        {
          if (mo->player && mo->momz < -GRAVITY * 8)
            {
              /* Squat down.
               * Decrease viewheight for a moment after hitting the ground
               * (hard), and utter appropriate sound.
               */

              mo->player->deltaviewheight = mo->momz >> 3;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(mo, SFX_OOF);
#endif
            }

          mo->momz = 0;
        }

      mo->z = mo->floorz;

      /* cph 2001/05/26 -
       * See lost soul bouncing comment above. We need this here for bug
       * compatibility with original Doom2 v1.9 - if a soul is charging and
       * hit by a raising floor this incorrectly reverses its Y momentum.
       */

      if (!correct_lost_soul_bounce && mo->flags & MF_SKULLFLY)
        mo->momz = -mo->momz;

      if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
        {
          p_explode_missile(mo);
          return;
        }
    }
  else if (!(mo->flags & MF_NOGRAVITY))
    {
      if (mo->momz == 0)
        mo->momz = -GRAVITY * 2;
      else
        mo->momz -= GRAVITY;
    }

  if (mo->z + mo->height > mo->ceilingz)
    {
      /* hit the ceiling */

      if (mo->momz > 0)
        {
          mo->momz = 0;
        }

      mo->z = mo->ceilingz - mo->height;

      if (mo->flags & MF_SKULLFLY)
        {
          /* the skull slammed into something */

          mo->momz = -mo->momz;
        }

      if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
        {
          p_explode_missile(mo);
          return;
        }
    }
}

/****************************************************************************
 * Name: p_nightmare_respawn
 ****************************************************************************/

static void p_nightmare_respawn(mobj_t *mobj)
{
  fixed_t x;
  fixed_t y;
  fixed_t z;
  subsector_t *ss;
  mobj_t *mo;
  mapthing_t *mthing;

  x = mobj->spawnpoint.x << FRACBITS;
  y = mobj->spawnpoint.y << FRACBITS;

  /* something is occupying it's position? */

  if (!p_check_position(mobj, x, y)) return; /* no respawn */

  /* spawn a teleport fog at old spot because of removal of the body? */

  mo = p_spawn_mobj(mobj->x, mobj->y, mobj->subsector->sector->floorheight,
                    MT_TFOG);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  /* initiate teleport sound */

  s_start_sound(mo, SFX_TELEPT);
#endif

  /* spawn a teleport fog at the new spot */

  ss = r_point_in_subsector(x, y);

  mo = p_spawn_mobj(x, y, ss->sector->floorheight, MT_TFOG);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(mo, SFX_TELEPT);
#endif

  mthing = &mobj->spawnpoint; /* spawn the new monster */

  /* spawn it */

  if (mobj->info->flags & MF_SPAWNCEILING)
    z = ONCEILINGZ;
  else
    z = ONFLOORZ;

  /* inherit attributes from deceased one */

  mo = p_spawn_mobj(x, y, z, mobj->type);
  mo->spawnpoint = mobj->spawnpoint;
  mo->angle = ANG45 * (mthing->angle / 45);

  if (mthing->options & MTF_AMBUSH) mo->flags |= MF_AMBUSH;

  mo->reactiontime = 18;

  /* remove the old monster, */

  p_remove_mobj(mobj);
}

/****************************************************************************
 * Name: p_check_missile_spawn
 *
 * Description:
 *  Moves the missile forward a bit and possibly explodes it right there.
 *
 ****************************************************************************/

static void p_check_missile_spawn(mobj_t *th)
{
  th->tics -= p_random() & 3;
  if (th->tics < 1) th->tics = 1;

  /* move a little forward so an angle can be computed if it immediately
   * explodes
   */

  th->x += (th->momx >> 1);
  th->y += (th->momy >> 1);
  th->z += (th->momz >> 1);

  if (!p_try_move(th, th->x, th->y)) p_explode_missile(th);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: p_set_mobj_state
 *
 * Description:
 *  Returns true if the mobj is still present.
 *
 ****************************************************************************/

boolean p_set_mobj_state(mobj_t *mobj, statenum_t state)
{
  state_t *st;
  int cycle_counter = 0;

  do
    {
      if (state == S_NULL)
        {
          mobj->state = (state_t *)S_NULL;
          p_remove_mobj(mobj);
          return false;
        }

      st = &states[state];
      mobj->state = st;
      mobj->tics = st->tics;
      mobj->sprite = st->sprite;
      mobj->frame = st->frame;

      /* Modified handling.
       * Call action functions when the state is set
       */

      if (st->action.acp1) st->action.acp1(mobj);

      state = st->nextstate;

      if (cycle_counter++ > MOBJ_CYCLE_LIMIT)
        {
          i_error("p_set_mobj_state: Infinite state cycle detected!");
        }
    }
  while (!mobj->tics);

  return true;
}

/****************************************************************************
 * Name: p_mobj_thinker
 ****************************************************************************/

void p_mobj_thinker(mobj_t *mobj)
{
  /* momentum movement */

  if (mobj->momx || mobj->momy || (mobj->flags & MF_SKULLFLY))
    {
      p_xy_movement(mobj);

      /* FIXME: decent NOP/NULL/Nil function pointer please. */

      if (mobj->thinker.function.acv == (actionf_v)(-1))
        return; /* mobj was removed */
    }

  if ((mobj->z != mobj->floorz) || mobj->momz)
    {
      p_z_movement(mobj);

      /* FIXME: decent NOP/NULL/Nil function pointer please. */

      if (mobj->thinker.function.acv == (actionf_v)(-1))
        return; /* mobj was removed */
    }

  /* cycle through states, calling action functions at transitions */

  if (mobj->tics != -1)
    {
      mobj->tics--;

      /* you can cycle through multiple states in a tic */

      if (!mobj->tics)
        {
          if (!p_set_mobj_state(mobj, mobj->state->nextstate))
            {
              return; /* freed itself */
            }
        }
    }
  else
    {
      /* check for nightmare respawn */

      if (!(mobj->flags & MF_COUNTKILL)) return;

      if (!respawnmonsters) return;

      mobj->movecount++;

      if (mobj->movecount < 12 * TICRATE) return;

      if (leveltime & 31) return;

      if (p_random() > 4) return;

      p_nightmare_respawn(mobj);
    }
}

/****************************************************************************
 * Name: p_spawn_mobj
 ****************************************************************************/

mobj_t *p_spawn_mobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
  mobj_t *mobj;
  state_t *st;
  mobjinfo_t *info;

  mobj = z_malloc(sizeof(*mobj), PU_LEVEL, NULL);
  memset(mobj, 0, sizeof(*mobj));
  info = &mobjinfo[type];

  mobj->type = type;
  mobj->info = info;
  mobj->x = x;
  mobj->y = y;
  mobj->radius = info->radius;
  mobj->height = info->height;
  mobj->flags = info->flags;
  mobj->health = info->spawnhealth;

  if (gameskill != sk_nightmare) mobj->reactiontime = info->reactiontime;

  mobj->lastlook = p_random() % MAXPLAYERS;

  /* do not set the state with p_set_mobj_state,
   * because action routines can not be called yet
   */

  st = &states[info->spawnstate];

  mobj->state = st;
  mobj->tics = st->tics;
  mobj->sprite = st->sprite;
  mobj->frame = st->frame;

  /* set subsector and/or block links */

  p_set_thing_position(mobj);

  mobj->floorz = mobj->subsector->sector->floorheight;
  mobj->ceilingz = mobj->subsector->sector->ceilingheight;

  if (z == ONFLOORZ)
    mobj->z = mobj->floorz;
  else if (z == ONCEILINGZ)
    mobj->z = mobj->ceilingz - mobj->info->height;
  else
    mobj->z = z;

  mobj->thinker.function.acp1 = (actionf_p1)p_mobj_thinker;

  p_add_thinker(&mobj->thinker);

  return mobj;
}

/****************************************************************************
 * Name: p_remove_mobj
 ****************************************************************************/

void p_remove_mobj(mobj_t *mobj)
{
  if ((mobj->flags & MF_SPECIAL) && !(mobj->flags & MF_DROPPED) &&
      (mobj->type != MT_INV) && (mobj->type != MT_INS))
    {
      itemrespawnque[iquehead] = mobj->spawnpoint;
      itemrespawntime[iquehead] = leveltime;
      iquehead = (iquehead + 1) & (ITEMQUESIZE - 1);

      /* lose one off the end? */

      if (iquehead == iquetail)
        {
          iquetail = (iquetail + 1) & (ITEMQUESIZE - 1);
        }
    }

  /* unlink from sector and block lists */

  p_unset_thing_position(mobj);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  /* stop any playing sound */

  s_stop_sound(mobj);
#endif

  p_remove_thinker((thinker_t *)mobj); /* free block */
}

/****************************************************************************
 * Name: p_respawn_specials
 ****************************************************************************/

void p_respawn_specials(void)
{
  fixed_t x;
  fixed_t y;
  fixed_t z;

  subsector_t *ss;
  mobj_t *mo;
  mapthing_t *mthing;

  int i;

  /* only respawn items in deathmatch */

  if (deathmatch != 2) return;

  /* nothing left to respawn? */

  if (iquehead == iquetail) return;

  /* wait at least 30 seconds */

  if (leveltime - itemrespawntime[iquetail] < 30 * TICRATE) return;

  mthing = &itemrespawnque[iquetail];

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  /* spawn a teleport fog at the new spot */

  ss = r_point_in_subsector(x, y);
  mo = p_spawn_mobj(x, y, ss->sector->floorheight, MT_IFOG);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(mo, SFX_ITMBK);
#endif

  /* find which type to spawn */

  for (i = 0; i < NUMMOBJTYPES; i++)
    {
      if (mthing->type == mobjinfo[i].doomednum) break;
    }

  if (i >= NUMMOBJTYPES)
    {
      i_error("p_respawn_specials: Failed to find mobj type with doomednum "
              "%d when respawning thing. This would cause a buffer overrun "
              "in vanilla Doom",
              mthing->type);
    }

  /* spawn it */

  if (mobjinfo[i].flags & MF_SPAWNCEILING)
    z = ONCEILINGZ;
  else
    z = ONFLOORZ;

  mo = p_spawn_mobj(x, y, z, i);
  mo->spawnpoint = *mthing;
  mo->angle = ANG45 * (mthing->angle / 45);

  /* pull it from the que */

  iquetail = (iquetail + 1) & (ITEMQUESIZE - 1);
}

/****************************************************************************
 * Name: p_spawn_player
 *
 * Description:
 *  Called when a player is spawned on the level.
 *  Most of the player structure stays unchanged between levels.
 *
 ****************************************************************************/

void p_spawn_player(mapthing_t *mthing)
{
  player_t *p;
  fixed_t x;
  fixed_t y;
  fixed_t z;

  mobj_t *mobj;

  int i;

  if (mthing->type == 0)
    {
      return;
    }

  /* not playing? */

  if (!playeringame[mthing->type - 1]) return;

  p = &players[mthing->type - 1];

  if (p->playerstate == PST_REBORN) g_player_reborn(mthing->type - 1);

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;
  z = ONFLOORZ;
  mobj = p_spawn_mobj(x, y, z, MT_PLAYER);

  /* set color translations for player sprites */

  if (mthing->type > 1) mobj->flags |= (mthing->type - 1) << MF_TRANSSHIFT;

  mobj->angle = ANG45 * (mthing->angle / 45);
  mobj->player = p;
  mobj->health = p->health;

  p->mo = mobj;
  p->playerstate = PST_LIVE;
  p->refire = 0;
  p->message = NULL;
  p->damagecount = 0;
  p->bonuscount = 0;
  p->extralight = 0;
  p->fixedcolormap = 0;
  p->viewheight = VIEWHEIGHT;

  p_setup_psprites(p); /* setup gun psprite */

  /* give all cards in death match mode */

  if (deathmatch)
    {
      for (i = 0; i < NUMCARDS; i++)
        {
          p->cards[i] = true;
        }
    }

  if (mthing->type - 1 == consoleplayer)
    {
      st_start(); /* wake up the status bar */
      hu_start(); /* wake up the heads up text */
    }
}

/****************************************************************************
 * Name: p_spawn_map_thing
 *
 * Description:
 *  The fields of the mapthing should already be in host byte order.
 *
 ****************************************************************************/

void p_spawn_map_thing(mapthing_t *mthing)
{
  int i;
  int bit;
  mobj_t *mobj;
  fixed_t x;
  fixed_t y;
  fixed_t z;

  /* count deathmatch start positions */

  if (mthing->type == 11)
    {
      if (deathmatch_p < &deathmatchstarts[10])
        {
          memcpy(deathmatch_p, mthing, sizeof(*mthing));
          deathmatch_p++;
        }

      return;
    }

  if (mthing->type <= 0)
    {
      /* Thing type 0 is actually "player -1 start".
       * For some reason, Vanilla Doom accepts/ignores this.
       */

      return;
    }

  /* check for players specially */

  if (mthing->type <= 4)
    {
      /* save spots for respawning in network games */

      playerstarts[mthing->type - 1] = *mthing;
      playerstartsingame[mthing->type - 1] = true;
      if (!deathmatch) p_spawn_player(mthing);

      return;
    }

  /* check for appropriate skill level */

  if (!netgame && (mthing->options & 16)) return;

  if (gameskill == sk_baby)
    {
      bit = 1;
    }
  else if (gameskill == sk_nightmare)
    {
      bit = 4;
    }
  else
    {
      /* avoid undefined behavior (left shift by negative value and rhs too
       * big) by accurately emulating what doom.exe did: reduce mod 32. For
       * more details check:
       * https://github.com/chocolate-doom/chocolate-doom/issues/1677
       */

      bit = (int)(1U << ((gameskill - 1) & 0x1f));
    }

  if (!(mthing->options & bit)) return;

  /* find which type to spawn */

  for (i = 0; i < NUMMOBJTYPES; i++)
    {
      if (mthing->type == mobjinfo[i].doomednum) break;
    }

  if (i == NUMMOBJTYPES)
    i_error("p_spawn_map_thing: Unknown type %i at (%i, %i)", mthing->type,
            mthing->x, mthing->y);

  /* don't spawn keycards and players in deathmatch */

  if (deathmatch && mobjinfo[i].flags & MF_NOTDMATCH) return;

  /* don't spawn any monsters if -nomonsters */

  if (nomonsters && (i == MT_SKULL || (mobjinfo[i].flags & MF_COUNTKILL)))
    {
      return;
    }

  /* spawn it */

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  if (mobjinfo[i].flags & MF_SPAWNCEILING)
    z = ONCEILINGZ;
  else
    z = ONFLOORZ;

  mobj = p_spawn_mobj(x, y, z, i);
  mobj->spawnpoint = *mthing;

  if (mobj->tics > 0) mobj->tics = 1 + (p_random() % mobj->tics);
  if (mobj->flags & MF_COUNTKILL) totalkills++;
  if (mobj->flags & MF_COUNTITEM) totalitems++;

  mobj->angle = ANG45 * (mthing->angle / 45);
  if (mthing->options & MTF_AMBUSH) mobj->flags |= MF_AMBUSH;
}

/* GAME SPAWN FUNCTIONS */

/****************************************************************************
 * Name: p_spawn_puff
 ****************************************************************************/

void p_spawn_puff(fixed_t x, fixed_t y, fixed_t z)
{
  mobj_t *th;

  z += (p_sub_random() << 10);

  th = p_spawn_mobj(x, y, z, MT_PUFF);
  th->momz = FRACUNIT;
  th->tics -= p_random() & 3;

  if (th->tics < 1) th->tics = 1;

  /* don't make punches spark on the wall */

  if (attackrange == MELEERANGE) p_set_mobj_state(th, S_PUFF3);
}

/****************************************************************************
 * Name: p_spawn_blood
 ****************************************************************************/

void p_spawn_blood(fixed_t x, fixed_t y, fixed_t z, int damage)
{
  mobj_t *th;

  z += (p_sub_random() << 10);
  th = p_spawn_mobj(x, y, z, MT_BLOOD);
  th->momz = FRACUNIT * 2;
  th->tics -= p_random() & 3;

  if (th->tics < 1) th->tics = 1;

  if (damage <= 12 && damage >= 9)
    p_set_mobj_state(th, S_BLOOD2);
  else if (damage < 9)
    p_set_mobj_state(th, S_BLOOD3);
}

/****************************************************************************
 * Name: p_subst_null_mobj
 *
 * Description:
 *  Certain functions assume that a mobj_t pointer is non-NULL,
 *  causing a crash in some situations where it is NULL.  Vanilla
 *  Doom did not crash because of the lack of proper memory
 *  protection. This function substitutes NULL pointers for
 *  pointers to a dummy mobj, to avoid a crash.
 *
 ****************************************************************************/

mobj_t *p_subst_null_mobj(mobj_t *mobj)
{
  if (mobj == NULL)
    {
      static mobj_t dummy_mobj;

      dummy_mobj.x = 0;
      dummy_mobj.y = 0;
      dummy_mobj.z = 0;
      dummy_mobj.flags = 0;

      mobj = &dummy_mobj;
    }

  return mobj;
}

/****************************************************************************
 * Name: p_spawn_missile
 ****************************************************************************/

mobj_t *p_spawn_missile(mobj_t *source, mobj_t *dest, mobjtype_t type)
{
  mobj_t *th;
  angle_t an;
  int dist;

  th = p_spawn_mobj(source->x, source->y,
          source->z + 4 * 8 * FRACUNIT, type);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (th->info->seesound) s_start_sound(th, th->info->seesound);
#endif

  th->target = source; /* where it came from */
  an = r_point_to_angle2(source->x, source->y, dest->x, dest->y);

  /* fuzzy player */

  if (dest->flags & MF_SHADOW) an += p_sub_random() << 20;

  th->angle = an;
  an >>= ANGLETOFINESHIFT;
  th->momx = fixed_mul(th->info->speed, finecosine[an]);
  th->momy = fixed_mul(th->info->speed, finesine[an]);

  dist = p_approx_distance(dest->x - source->x, dest->y - source->y);
  dist = dist / th->info->speed;

  if (dist < 1) dist = 1;

  th->momz = (dest->z - source->z) / dist;
  p_check_missile_spawn(th);

  return th;
}

/****************************************************************************
 * Name: p_spawn_player_missile
 *
 * Description:
 *  Tries to aim at a nearby monster
 *
 ****************************************************************************/

void p_spawn_player_missile(mobj_t *source, mobjtype_t type)
{
  mobj_t *th;
  angle_t an;

  fixed_t x;
  fixed_t y;
  fixed_t z;
  fixed_t slope;

  /* see which target is to be aimed at */

  an = source->angle;
  slope = p_aim_line_attack(source, an, 16 * 64 * FRACUNIT);

  if (!linetarget)
    {
      an += 1 << 26;
      slope = p_aim_line_attack(source, an, 16 * 64 * FRACUNIT);

      if (!linetarget)
        {
          an -= 2 << 26;
          slope = p_aim_line_attack(source, an, 16 * 64 * FRACUNIT);
        }

      if (!linetarget)
        {
          an = source->angle;
          slope = 0;
        }
    }

  x = source->x;
  y = source->y;
  z = source->z + 4 * 8 * FRACUNIT;

  th = p_spawn_mobj(x, y, z, type);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (th->info->seesound) s_start_sound(th, th->info->seesound);
#endif

  th->target = source;
  th->angle = an;
  th->momx = fixed_mul(th->info->speed, finecosine[an >> ANGLETOFINESHIFT]);
  th->momy = fixed_mul(th->info->speed, finesine[an >> ANGLETOFINESHIFT]);
  th->momz = fixed_mul(th->info->speed, slope);

  p_check_missile_spawn(th);
}
