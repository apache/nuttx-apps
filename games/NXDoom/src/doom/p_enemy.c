/****************************************************************************
 * apps/games/NXDoom/src/doom/p_enemy.c
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
 *  Enemy thinking, AI.
 *  Action Pointer Functions
 *  that are associated with states/frames.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "m_random.h"

#include "doomdef.h"
#include "p_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "g_game.h"

/* State. */

#include "doomstat.h"
#include "r_state.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TRACEANGLE 0xc000000

#define FATSPREAD (ANG90 / 8)

#define SKULLSPEED (20 * FRACUNIT)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Data. */

typedef enum
{
  DI_EAST,
  DI_NORTHEAST,
  DI_NORTH,
  DI_NORTHWEST,
  DI_WEST,
  DI_SOUTHWEST,
  DI_SOUTH,
  DI_SOUTHEAST,
  DI_NODIR,
  NUMDIRS
} dirtype_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void a_fall(mobj_t *actor);
void a_refire(player_t *player, pspdef_t *psp);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* p_new_chase_dir related LUT. */

dirtype_t opposite[] =
{
  DI_WEST,      DI_SOUTHWEST, DI_SOUTH,     DI_SOUTHEAST, DI_EAST,
  DI_NORTHEAST, DI_NORTH,     DI_NORTHWEST, DI_NODIR,
};

dirtype_t diags[] =
{
  DI_NORTHWEST,
  DI_NORTHEAST,
  DI_SOUTHWEST,
  DI_SOUTHEAST,
};

/* ENEMY THINKING
 * Enemies are always spawned with targetplayer = -1, threshold = 0
 * Most monsters are spawned unaware of all players, but some can be made
 * preaware
 */

/* Called by p_noise_alert.
 * Recursively traverse adjacent sectors, sound blocking lines cut off
 * traversal.
 */

mobj_t *soundtarget;

fixed_t xspeed[8] =
{
  FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000
};

fixed_t yspeed[8] =
{
  0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000
};

mobj_t *corpsehit;
mobj_t *vileobj;
fixed_t viletryx;
fixed_t viletryy;

mobj_t *braintargets[32];
int numbraintargets;
int braintargeton = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void p_recursive_sound(sector_t *sec, int soundblocks)
{
  int i;
  line_t *check;
  sector_t *other;

  /* wake up all monsters in this sector */

  if (sec->validcount == validcount &&
      sec->soundtraversed <= soundblocks + 1)
    {
      return; /* already flooded */
    }

  sec->validcount = validcount;
  sec->soundtraversed = soundblocks + 1;
  sec->soundtarget = soundtarget;

  for (i = 0; i < sec->linecount; i++)
    {
      check = sec->lines[i];
      if (!(check->flags & ML_TWOSIDED)) continue;

      p_line_opening(check);

      if (openrange <= 0) continue; /* closed door */

      if (sides[check->sidenum[0]].sector == sec)
        other = sides[check->sidenum[1]].sector;
      else
        other = sides[check->sidenum[0]].sector;

      if (check->flags & ML_SOUNDBLOCK)
        {
          if (!soundblocks) p_recursive_sound(other, 1);
        }
      else
        p_recursive_sound(other, soundblocks);
    }
}

static boolean p_check_missile_range(mobj_t *actor)
{
  fixed_t dist;

  if (!p_check_sight(actor, actor->target)) return false;

  if (actor->flags & MF_JUSTHIT)
    {
      /* the target just hit the enemy, so fight back! */

      actor->flags &= ~MF_JUSTHIT;
      return true;
    }

  if (actor->reactiontime) return false; /* do not attack yet */

  /* OPTIMIZE: get this from a global checksight */

  dist = p_approx_distance(actor->x - actor->target->x,
                           actor->y - actor->target->y) -
         64 * FRACUNIT;

  if (!actor->info->meleestate)
    dist -= 128 * FRACUNIT; /* no melee attack, so fire more */

  dist >>= FRACBITS;

  if (actor->type == MT_VILE)
    {
      if (dist > 14 * 64) return false; /* too far away */
    }

  if (actor->type == MT_UNDEAD)
    {
      if (dist < 196) return false; /* close for fist attack */
      dist >>= 1;
    }

  if (actor->type == MT_CYBORG || actor->type == MT_SPIDER ||
      actor->type == MT_SKULL)
    {
      dist >>= 1;
    }

  if (dist > 200) dist = 200;

  if (actor->type == MT_CYBORG && dist > 160) dist = 160;

  if (p_random() < dist) return false;

  return true;
}

/* p_move
 * Move in the current direction, returns false if the move is blocked.
 */

static boolean p_move(mobj_t *actor)
{
  fixed_t tryx;
  fixed_t tryy;

  line_t *ld;

  /* warning: 'catch', 'throw', and 'try' are all C++ reserved words */

  boolean try_ok;
  boolean good;

  if (actor->movedir == DI_NODIR) return false;

  if ((unsigned)actor->movedir >= 8) i_error("Weird actor->movedir!");

  tryx = actor->x + actor->info->speed * xspeed[actor->movedir];
  tryy = actor->y + actor->info->speed * yspeed[actor->movedir];

  try_ok = p_try_move(actor, tryx, tryy);

  if (!try_ok)
    {
      /* open any specials */

      if (actor->flags & MF_FLOAT && floatok)
        {
          /* must adjust height */

          if (actor->z < tmfloorz)
            actor->z += FLOATSPEED;
          else
            actor->z -= FLOATSPEED;

          actor->flags |= MF_INFLOAT;
          return true;
        }

      if (!numspechit) return false;

      actor->movedir = DI_NODIR;
      good = false;
      while (numspechit--)
        {
          ld = spechit[numspechit];

          /* if the special is not a door that can be opened, return false */

          if (p_use_special_line(actor, ld, 0)) good = true;
        }

      return good;
    }
  else
    {
      actor->flags &= ~MF_INFLOAT;
    }

  if (!(actor->flags & MF_FLOAT)) actor->z = actor->floorz;
  return true;
}

/* p_try_walk
 *
 * Attempts to move actor on in its current (ob->moveangle) direction.
 * If blocked by either a wall or an actor returns FALSE
 * If move is either clear or blocked only by a door, returns TRUE and
 * sets...
 * If a door is in the way, an OpenDoor call is made to start it opening.
 */

static boolean p_try_walk(mobj_t *actor)
{
  if (!p_move(actor))
    {
      return false;
    }

  actor->movecount = p_random() & 15;
  return true;
}

static void p_new_chase_dir(mobj_t *actor)
{
  fixed_t deltax;
  fixed_t deltay;

  dirtype_t d[3];

  int tdir;
  dirtype_t olddir;

  dirtype_t turnaround;

  if (!actor->target) i_error("p_new_chase_dir: called with no target");

  olddir = actor->movedir;
  turnaround = opposite[olddir];

  deltax = actor->target->x - actor->x;
  deltay = actor->target->y - actor->y;

  if (deltax > 10 * FRACUNIT)
    d[1] = DI_EAST;
  else if (deltax < -10 * FRACUNIT)
    d[1] = DI_WEST;
  else
    d[1] = DI_NODIR;

  if (deltay < -10 * FRACUNIT)
    d[2] = DI_SOUTH;
  else if (deltay > 10 * FRACUNIT)
    d[2] = DI_NORTH;
  else
    d[2] = DI_NODIR;

  /* try direct route */

  if (d[1] != DI_NODIR && d[2] != DI_NODIR)
    {
      actor->movedir = diags[((deltay < 0) << 1) + (deltax > 0)];
      if (actor->movedir != (int)turnaround && p_try_walk(actor)) return;
    }

  /* try other directions */

  if (p_random() > 200 || abs(deltay) > abs(deltax))
    {
      tdir = d[1];
      d[1] = d[2];
      d[2] = tdir;
    }

  if (d[1] == turnaround) d[1] = DI_NODIR;
  if (d[2] == turnaround) d[2] = DI_NODIR;

  if (d[1] != DI_NODIR)
    {
      actor->movedir = d[1];
      if (p_try_walk(actor))
        {
          return; /* either moved forward or attacked */
        }
    }

  if (d[2] != DI_NODIR)
    {
      actor->movedir = d[2];

      if (p_try_walk(actor)) return;
    }

  /* there is no direct path to the player, so pick another direction. */

  if (olddir != DI_NODIR)
    {
      actor->movedir = olddir;

      if (p_try_walk(actor)) return;
    }

  /* randomly determine direction of search */

  if (p_random() & 1)
    {
      for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
        {
          if (tdir != (int)turnaround)
            {
              actor->movedir = tdir;

              if (p_try_walk(actor)) return;
            }
        }
    }
  else
    {
      for (tdir = DI_SOUTHEAST; tdir != (DI_EAST - 1); tdir--)
        {
          if (tdir != (int)turnaround)
            {
              actor->movedir = tdir;

              if (p_try_walk(actor)) return;
            }
        }
    }

  if (turnaround != DI_NODIR)
    {
      actor->movedir = turnaround;
      if (p_try_walk(actor)) return;
    }

  actor->movedir = DI_NODIR; /* can not move */
}

/* p_look_for_players
 * If allaround is false, only look 180 degrees in front. Returns true if a
 * player is targeted.
 */

static boolean p_look_for_players(mobj_t *actor, boolean allaround)
{
  int c;
  int stop;
  player_t *player;
  angle_t an;
  fixed_t dist;

  c = 0;
  stop = (actor->lastlook - 1) & 3;

  for (; ; actor->lastlook = (actor->lastlook + 1) & 3)
    {
      if (!playeringame[actor->lastlook]) continue;

      if (c++ == 2 || actor->lastlook == stop)
        {
          return false; /* done looking */
        }

      player = &players[actor->lastlook];

      if (player->health <= 0) continue; /* dead */

      if (!p_check_sight(actor, player->mo)) continue; /* out of sight */

      if (!allaround)
        {
          an = r_point_to_angle2(actor->x, actor->y, player->mo->x,
                                 player->mo->y) -
               actor->angle;

          if (an > ANG90 && an < ANG270)
            {
              dist = p_approx_distance(player->mo->x - actor->x,
                                       player->mo->y - actor->y);

              /* if real close, react anyway */

              if (dist > MELEERANGE) continue; /* behind back */
            }
        }

      actor->target = player->mo;
      return true;
    }

  return false;
}

/* pit_vile_check
 * Detect a corpse that could be raised.
 */

static boolean pit_vile_check(mobj_t *thing)
{
  int maxdist;
  boolean check;

  if (!(thing->flags & MF_CORPSE)) return true; /* not a monster */

  if (thing->tics != -1) return true; /* not lying still yet */

  if (thing->info->raisestate == S_NULL)
    {
      return true; /* monster doesn't have a raise state */
    }

  maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;

  if (abs(thing->x - viletryx) > maxdist ||
      abs(thing->y - viletryy) > maxdist)
    {
      return true; /* not actually touching */
    }

  corpsehit = thing;
  corpsehit->momx = corpsehit->momy = 0;
  corpsehit->height <<= 2;
  check = p_check_position(corpsehit, corpsehit->x, corpsehit->y);
  corpsehit->height >>= 2;

  if (!check) return true; /* doesn't fit here */

  return false; /* got one, so stop checking */
}

/* Check whether the death of the specified monster type is allowed
 * to trigger the end of episode special action.
 *
 * This behavior changed in v1.9, the most notable effect of which
 * was to break uac_dead.wad
 */

static boolean check_boss_end(mobjtype_t motype)
{
  if (gameversion < exe_ultimate)
    {
      if (gamemap != 8)
        {
          return false;
        }

      /* Baron death on later episodes is nothing special. */

      if (motype == MT_BRUISER && gameepisode != 1)
        {
          return false;
        }

      return true;
    }
  else
    {
      /* New logic that appeared in Ultimate Doom.
       * Looks like the logic was overhauled while adding in the
       * episode 4 support.  Now bosses only trigger on their
       * specific episode.
       */

      switch (gameepisode)
        {
        case 1:
          return gamemap == 8 && motype == MT_BRUISER;

        case 2:
          return gamemap == 8 && motype == MT_CYBORG;

        case 3:
          return gamemap == 8 && motype == MT_SPIDER;

        case 4:
          return (gamemap == 6 && motype == MT_CYBORG) ||
                 (gamemap == 8 && motype == MT_SPIDER);

        default:
          return gamemap == 8;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* p_noise_alert
 * If a monster yells at a player,
 * it will alert other monsters to the player.
 */

void p_noise_alert(mobj_t *target, mobj_t *emmiter)
{
  soundtarget = target;
  validcount++;
  p_recursive_sound(emmiter->subsector->sector, 0);
}

boolean p_check_melee_range(mobj_t *actor)
{
  mobj_t *pl;
  fixed_t dist;
  fixed_t range;

  if (!actor->target) return false;

  pl = actor->target;
  dist = p_approx_distance(pl->x - actor->x, pl->y - actor->y);

  if (gameversion < exe_doom_1_5)
    range = MELEERANGE;
  else
    range = MELEERANGE - 20 * FRACUNIT + pl->info->radius;

  if (dist >= range) return false;

  if (!p_check_sight(actor, actor->target)) return false;

  return true;
}

/* a_keen_die
 * DOOM II special, map 32. Uses special tag 666.
 */

void a_keen_die(mobj_t *mo)
{
  thinker_t *th;
  mobj_t *mo2;
  line_t junk;

  a_fall(mo);

  /* scan the remaining thinkers to see if all Keens are dead */

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      if (th->function.acp1 != (actionf_p1)p_mobj_thinker) continue;

      mo2 = (mobj_t *)th;
      if (mo2 != mo && mo2->type == mo->type && mo2->health > 0)
        {
          return; /* other Keen not dead */
        }
    }

  junk.tag = 666;
  ev_do_door(&junk, VLD_OPEN);
}

/* ACTION ROUTINES */

/* a_look
 * Stay in state until a player is sighted.
 */

void a_look(mobj_t *actor)
{
  mobj_t *targ;

  actor->threshold = 0; /* any shot will wake up */
  targ = actor->subsector->sector->soundtarget;

  if (targ && (targ->flags & MF_SHOOTABLE))
    {
      actor->target = targ;

      if (actor->flags & MF_AMBUSH)
        {
          if (p_check_sight(actor, actor->target)) goto seeyou;
        }
      else
        {
          goto seeyou;
        }
    }

  if (!p_look_for_players(actor, false)) return;

  /* go into chase state */

seeyou:
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (actor->info->seesound)
    {
      int sound;

      switch (actor->info->seesound)
        {
        case SFX_POSIT1:
        case SFX_POSIT2:
        case SFX_POSIT3:
          sound = SFX_POSIT1 + p_random() % 3;
          break;

        case SFX_BGSIT1:
        case SFX_BGSIT2:
          sound = SFX_BGSIT1 + p_random() % 2;
          break;

        default:
          sound = actor->info->seesound;
          break;
        }

      if (actor->type == MT_SPIDER || actor->type == MT_CYBORG)
        {
          /* full volume */

          s_start_sound(NULL, sound);
        }
      else
        s_start_sound(actor, sound);
    }
#endif

  p_set_mobj_state(actor, actor->info->seestate);
}

/* a_chase
 * Actor has a melee attack, so it tries to close as fast as possible
 */

void a_chase(mobj_t *actor)
{
  int delta;

  if (actor->reactiontime) actor->reactiontime--;

  /* modify target threshold */

  if (actor->threshold)
    {
      if (gameversion > exe_doom_1_2 &&
          (!actor->target || actor->target->health <= 0))
        {
          actor->threshold = 0;
        }
      else
        actor->threshold--;
    }

  /* turn towards movement direction if not there yet */

  if (actor->movedir < 8)
    {
      actor->angle &= (7u << 29);
      delta = actor->angle - (actor->movedir << 29);

      if (delta > 0)
        actor->angle -= ANG90 / 2;
      else if (delta < 0)
        actor->angle += ANG90 / 2;
    }

  if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
    {
      /* look for a new target */

      if (p_look_for_players(actor, true)) return; /* got a new target */

      p_set_mobj_state(actor, actor->info->spawnstate);
      return;
    }

  /* do not attack twice in a row */

  if (actor->flags & MF_JUSTATTACKED)
    {
      actor->flags &= ~MF_JUSTATTACKED;
      if (gameskill != sk_nightmare && !fastparm) p_new_chase_dir(actor);
      return;
    }

  /* check for melee attack */

  if (actor->info->meleestate && p_check_melee_range(actor))
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (actor->info->attacksound)
        s_start_sound(actor, actor->info->attacksound);
#endif

      p_set_mobj_state(actor, actor->info->meleestate);
      return;
    }

  /* check for missile attack */

  if (actor->info->missilestate)
    {
      if (gameskill < sk_nightmare && !fastparm && actor->movecount)
        {
          goto nomissile;
        }

      if (!p_check_missile_range(actor)) goto nomissile;

      p_set_mobj_state(actor, actor->info->missilestate);
      actor->flags |= MF_JUSTATTACKED;
      return;
    }

  /* ? */

nomissile:

  /* possibly choose another target */

  if (netgame && !actor->threshold && !p_check_sight(actor, actor->target))
    {
      if (p_look_for_players(actor, true)) return; /* got a new target */
    }

  /* chase towards player */

  if (--actor->movecount < 0 || !p_move(actor))
    {
      p_new_chase_dir(actor);
    }

#ifdef CONFIG_GAMES_NXDOOM_SOUND

  /* make active sound */

  if (actor->info->activesound && p_random() < 3)
    {
      s_start_sound(actor, actor->info->activesound);
    }
#endif
}

void a_face_target(mobj_t *actor)
{
  if (!actor->target) return;

  actor->flags &= ~MF_AMBUSH;

  actor->angle = r_point_to_angle2(actor->x, actor->y, actor->target->x,
                                   actor->target->y);

  if (actor->target->flags & MF_SHADOW) actor->angle += p_sub_random() << 21;
}

void a_pos_attack(mobj_t *actor)
{
  int angle;
  int damage;
  int slope;

  if (!actor->target) return;

  a_face_target(actor);
  angle = actor->angle;
  slope = p_aim_line_attack(actor, angle, MISSILERANGE);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_PISTOL);
#endif
  angle += p_sub_random() << 20;
  damage = ((p_random() % 5) + 1) * 3;
  p_line_attack(actor, angle, MISSILERANGE, slope, damage);
}

void a_s_pos_attack(mobj_t *actor)
{
  int i;
  int angle;
  int bangle;
  int damage;
  int slope;

  if (!actor->target) return;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_SHOTGN);
#endif
  a_face_target(actor);
  bangle = actor->angle;
  slope = p_aim_line_attack(actor, bangle, MISSILERANGE);

  for (i = 0; i < 3; i++)
    {
      angle = bangle + (p_sub_random() << 20);
      damage = ((p_random() % 5) + 1) * 3;
      p_line_attack(actor, angle, MISSILERANGE, slope, damage);
    }
}

void a_c_pos_attack(mobj_t *actor)
{
  int angle;
  int bangle;
  int damage;
  int slope;

  if (!actor->target) return;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_SHOTGN);
#endif
  a_face_target(actor);
  bangle = actor->angle;
  slope = p_aim_line_attack(actor, bangle, MISSILERANGE);

  angle = bangle + (p_sub_random() << 20);
  damage = ((p_random() % 5) + 1) * 3;
  p_line_attack(actor, angle, MISSILERANGE, slope, damage);
}

void a_c_pos_refire(mobj_t *actor)
{
  /* keep firing unless target got out of sight */

  a_face_target(actor);

  if (p_random() < 40) return;

  if (!actor->target || actor->target->health <= 0 ||
      !p_check_sight(actor, actor->target))
    {
      p_set_mobj_state(actor, actor->info->seestate);
    }
}

void a_spid_refire(mobj_t *actor)
{
  /* keep firing unless target got out of sight */

  a_face_target(actor);

  if (p_random() < 10) return;

  if (!actor->target || actor->target->health <= 0 ||
      !p_check_sight(actor, actor->target))
    {
      p_set_mobj_state(actor, actor->info->seestate);
    }
}

void a_bspi_attack(mobj_t *actor)
{
  if (!actor->target) return;

  a_face_target(actor);

  /* launch a missile */

  p_spawn_missile(actor, actor->target, MT_ARACHPLAZ);
}

void a_troop_attack(mobj_t *actor)
{
  int damage;

  if (!actor->target) return;

  a_face_target(actor);
  if (p_check_melee_range(actor))
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(actor, SFX_CLAW);
#endif
      damage = (p_random() % 8 + 1) * 3;
      p_damage_mobj(actor->target, actor, actor, damage);
      return;
    }

  /* launch a missile */

  p_spawn_missile(actor, actor->target, MT_TROOPSHOT);
}

void a_sarg_attack(mobj_t *actor)
{
  int damage;

  if (!actor->target) return;

  a_face_target(actor);

  if (gameversion >= exe_doom_1_5)
    {
      if (!p_check_melee_range(actor)) return;
    }

  damage = ((p_random() % 10) + 1) * 4;

  if (gameversion <= exe_doom_1_2)
    p_line_attack(actor, actor->angle, MELEERANGE, 0, damage);
  else
    p_damage_mobj(actor->target, actor, actor, damage);
}

void a_head_attack(mobj_t *actor)
{
  int damage;

  if (!actor->target) return;

  a_face_target(actor);
  if (p_check_melee_range(actor))
    {
      damage = (p_random() % 6 + 1) * 10;
      p_damage_mobj(actor->target, actor, actor, damage);
      return;
    }

  /* launch a missile */

  p_spawn_missile(actor, actor->target, MT_HEADSHOT);
}

void a_cyber_attack(mobj_t *actor)
{
  if (!actor->target) return;

  a_face_target(actor);
  p_spawn_missile(actor, actor->target, MT_ROCKET);
}

void a_bruis_attack(mobj_t *actor)
{
  int damage;

  if (!actor->target) return;

  if (p_check_melee_range(actor))
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(actor, SFX_CLAW);
#endif
      damage = (p_random() % 8 + 1) * 10;
      p_damage_mobj(actor->target, actor, actor, damage);
      return;
    }

  /* launch a missile */

  p_spawn_missile(actor, actor->target, MT_BRUISERSHOT);
}

void a_skel_missile(mobj_t *actor)
{
  mobj_t *mo;

  if (!actor->target) return;

  a_face_target(actor);
  actor->z += 16 * FRACUNIT; /* so missile spawns higher */
  mo = p_spawn_missile(actor, actor->target, MT_TRACER);
  actor->z -= 16 * FRACUNIT; /* back to normal */

  mo->x += mo->momx;
  mo->y += mo->momy;
  mo->tracer = actor->target;
}

void a_tracer(mobj_t *actor)
{
  angle_t exact;
  fixed_t dist;
  fixed_t slope;
  mobj_t *dest;
  mobj_t *th;

  if (gametic & 3) return;

  /* spawn a puff of smoke behind the rocket */

  p_spawn_puff(actor->x, actor->y, actor->z);

  th = p_spawn_mobj(actor->x - actor->momx, actor->y - actor->momy, actor->z,
                    MT_SMOKE);

  th->momz = FRACUNIT;
  th->tics -= p_random() & 3;
  if (th->tics < 1) th->tics = 1;

  /* adjust direction */

  dest = actor->tracer;

  if (!dest || dest->health <= 0) return;

  /* change angle */

  exact = r_point_to_angle2(actor->x, actor->y, dest->x, dest->y);

  if (exact != actor->angle)
    {
      if (exact - actor->angle > 0x80000000)
        {
          actor->angle -= TRACEANGLE;
          if (exact - actor->angle < 0x80000000) actor->angle = exact;
        }
      else
        {
          actor->angle += TRACEANGLE;
          if (exact - actor->angle > 0x80000000) actor->angle = exact;
        }
    }

  exact = actor->angle >> ANGLETOFINESHIFT;
  actor->momx = fixed_mul(actor->info->speed, finecosine[exact]);
  actor->momy = fixed_mul(actor->info->speed, finesine[exact]);

  /* change slope */

  dist = p_approx_distance(dest->x - actor->x, dest->y - actor->y);

  dist = dist / actor->info->speed;

  if (dist < 1) dist = 1;
  slope = (dest->z + 40 * FRACUNIT - actor->z) / dist;

  if (slope < actor->momz)
    actor->momz -= FRACUNIT / 8;
  else
    actor->momz += FRACUNIT / 8;
}

void a_skel_woosh(mobj_t *actor)
{
  if (!actor->target) return;
  a_face_target(actor);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_SKESWG);
#endif
}

void a_skel_fist(mobj_t *actor)
{
  int damage;

  if (!actor->target) return;

  a_face_target(actor);

  if (p_check_melee_range(actor))
    {
      damage = ((p_random() % 10) + 1) * 6;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(actor, SFX_SKEPCH);
#endif
      p_damage_mobj(actor->target, actor, actor, damage);
    }
}

/* a_vile_chase
 * Check for resurrecting a body
 */

void a_vile_chase(mobj_t *actor)
{
  int xl;
  int xh;
  int yl;
  int yh;

  int bx;
  int by;

  mobjinfo_t *info;
  mobj_t *temp;

  if (actor->movedir != DI_NODIR)
    {
      /* check for corpses to raise */

      viletryx = actor->x + actor->info->speed * xspeed[actor->movedir];
      viletryy = actor->y + actor->info->speed * yspeed[actor->movedir];

      xl = (viletryx - bmaporgx - MAXRADIUS * 2) >> MAPBLOCKSHIFT;
      xh = (viletryx - bmaporgx + MAXRADIUS * 2) >> MAPBLOCKSHIFT;
      yl = (viletryy - bmaporgy - MAXRADIUS * 2) >> MAPBLOCKSHIFT;
      yh = (viletryy - bmaporgy + MAXRADIUS * 2) >> MAPBLOCKSHIFT;

      vileobj = actor;
      for (bx = xl; bx <= xh; bx++)
        {
          for (by = yl; by <= yh; by++)
            {
              /* Call pit_vile_check to check whether object is a corpse that
               * can be raised.
               */

              if (!p_block_things_iterator(bx, by, pit_vile_check))
                {
                  /* got one! */

                  temp = actor->target;
                  actor->target = corpsehit;
                  a_face_target(actor);
                  actor->target = temp;

                  p_set_mobj_state(actor, S_VILE_HEAL1);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
                  s_start_sound(corpsehit, SFX_SLOP);
#endif
                  info = corpsehit->info;

                  p_set_mobj_state(corpsehit, info->raisestate);
                  corpsehit->height <<= 2;
                  corpsehit->flags = info->flags;
                  corpsehit->health = info->spawnhealth;
                  corpsehit->target = NULL;

                  return;
                }
            }
        }
    }

  /* Return to normal attack. */

  a_chase(actor);
}

/* Keep fire in front of player unless out of sight */

void a_fire(mobj_t *actor)
{
  mobj_t *dest;
  mobj_t *target;
  unsigned an;

  dest = actor->tracer;
  if (!dest) return;

  target = p_subst_null_mobj(actor->target);

  /* don't move it if the vile lost sight */

  if (!p_check_sight(target, dest)) return;

  an = dest->angle >> ANGLETOFINESHIFT;

  p_unset_thing_position(actor);
  actor->x = dest->x + fixed_mul(24 * FRACUNIT, finecosine[an]);
  actor->y = dest->y + fixed_mul(24 * FRACUNIT, finesine[an]);
  actor->z = dest->z;
  p_set_thing_position(actor);
}

void a_vile_start(mobj_t *actor)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_VILATK);
#endif
}

void a_start_fire(mobj_t *actor)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_FLAMST);
#endif
  a_fire(actor);
}

void a_fire_crackle(mobj_t *actor)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_FLAME);
#endif
  a_fire(actor);
}

/* a_vile_target
 * Spawn the hellfire
 */

void a_vile_target(mobj_t *actor)
{
  mobj_t *fog;

  if (!actor->target) return;

  a_face_target(actor);

  fog = p_spawn_mobj(actor->target->x, actor->target->x, actor->target->z,
                     MT_FIRE);

  actor->tracer = fog;
  fog->target = actor;
  fog->tracer = actor->target;
  a_fire(fog);
}

void a_vile_attack(mobj_t *actor)
{
  mobj_t *fire;
  int an;

  if (!actor->target) return;

  a_face_target(actor);

  if (!p_check_sight(actor, actor->target)) return;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_BAREXP);
#endif
  p_damage_mobj(actor->target, actor, actor, 20);
  actor->target->momz = 1000 * FRACUNIT / actor->target->info->mass;

  an = actor->angle >> ANGLETOFINESHIFT;

  fire = actor->tracer;

  if (!fire) return;

  /* move the fire between the vile and the player */

  fire->x = actor->target->x - fixed_mul(24 * FRACUNIT, finecosine[an]);
  fire->y = actor->target->y - fixed_mul(24 * FRACUNIT, finesine[an]);
  p_radius_attack(fire, actor, 70);
}

/* Mancubus attack,
 * firing three missiles (bruisers) in three different directions?
 * Doesn't look like it.
 */

void a_fat_raise(mobj_t *actor)
{
  a_face_target(actor);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_MANATK);
#endif
}

void a_fat_attack1(mobj_t *actor)
{
  mobj_t *mo;
  mobj_t *target;
  int an;

  a_face_target(actor);

  /* Change direction  to ... */

  actor->angle += FATSPREAD;
  target = p_subst_null_mobj(actor->target);
  p_spawn_missile(actor, target, MT_FATSHOT);

  mo = p_spawn_missile(actor, target, MT_FATSHOT);
  mo->angle += FATSPREAD;
  an = mo->angle >> ANGLETOFINESHIFT;
  mo->momx = fixed_mul(mo->info->speed, finecosine[an]);
  mo->momy = fixed_mul(mo->info->speed, finesine[an]);
}

void a_fat_attack2(mobj_t *actor)
{
  mobj_t *mo;
  mobj_t *target;
  int an;

  a_face_target(actor);

  /* Now here choose opposite deviation. */

  actor->angle -= FATSPREAD;
  target = p_subst_null_mobj(actor->target);
  p_spawn_missile(actor, target, MT_FATSHOT);

  mo = p_spawn_missile(actor, target, MT_FATSHOT);
  mo->angle -= FATSPREAD * 2;
  an = mo->angle >> ANGLETOFINESHIFT;
  mo->momx = fixed_mul(mo->info->speed, finecosine[an]);
  mo->momy = fixed_mul(mo->info->speed, finesine[an]);
}

void a_fat_attack3(mobj_t *actor)
{
  mobj_t *mo;
  mobj_t *target;
  int an;

  a_face_target(actor);

  target = p_subst_null_mobj(actor->target);

  mo = p_spawn_missile(actor, target, MT_FATSHOT);
  mo->angle -= FATSPREAD / 2;
  an = mo->angle >> ANGLETOFINESHIFT;
  mo->momx = fixed_mul(mo->info->speed, finecosine[an]);
  mo->momy = fixed_mul(mo->info->speed, finesine[an]);

  mo = p_spawn_missile(actor, target, MT_FATSHOT);
  mo->angle += FATSPREAD / 2;
  an = mo->angle >> ANGLETOFINESHIFT;
  mo->momx = fixed_mul(mo->info->speed, finecosine[an]);
  mo->momy = fixed_mul(mo->info->speed, finesine[an]);
}

/* SkullAttack
 * Fly at the player like a missile.
 */

void a_skull_attack(mobj_t *actor)
{
  mobj_t *dest;
  angle_t an;
  int dist;

  if (!actor->target) return;

  dest = actor->target;
  actor->flags |= MF_SKULLFLY;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, actor->info->attacksound);
#endif
  a_face_target(actor);
  an = actor->angle >> ANGLETOFINESHIFT;
  actor->momx = fixed_mul(SKULLSPEED, finecosine[an]);
  actor->momy = fixed_mul(SKULLSPEED, finesine[an]);
  dist = p_approx_distance(dest->x - actor->x, dest->y - actor->y);
  dist = dist / SKULLSPEED;

  if (dist < 1) dist = 1;
  actor->momz = (dest->z + (dest->height >> 1) - actor->z) / dist;
}

/* a_pain_shoot_skull
 * Spawn a lost soul and launch it at the target
 */

void a_pain_shoot_skull(mobj_t *actor, angle_t angle)
{
  fixed_t x;
  fixed_t y;
  fixed_t z;

  mobj_t *newmobj;
  angle_t an;
  int prestep;
  int count;
  thinker_t *currentthinker;

  /* count total number of skull currently on the level */

  count = 0;

  currentthinker = thinkercap.next;
  while (currentthinker != &thinkercap)
    {
      if ((currentthinker->function.acp1 == (actionf_p1)p_mobj_thinker) &&
          ((mobj_t *)currentthinker)->type == MT_SKULL)
        count++;
      currentthinker = currentthinker->next;
    }

  /* if there are already 20 skulls on the level, don't spit another one */

  if (count > 20) return;

  /* okay, there's player for another one */

  an = angle >> ANGLETOFINESHIFT;

  prestep = 4 * FRACUNIT +
            3 * (actor->info->radius + mobjinfo[MT_SKULL].radius) / 2;

  x = actor->x + fixed_mul(prestep, finecosine[an]);
  y = actor->y + fixed_mul(prestep, finesine[an]);
  z = actor->z + 8 * FRACUNIT;

  newmobj = p_spawn_mobj(x, y, z, MT_SKULL);

  /* Check for movements. */

  if (!p_try_move(newmobj, newmobj->x, newmobj->y))
    {
      /* kill it immediately */

      p_damage_mobj(newmobj, actor, actor, 10000);
      return;
    }

  newmobj->target = actor->target;
  a_skull_attack(newmobj);
}

/* a_pain_attack
 * Spawn a lost soul and launch it at the target
 */

void a_pain_attack(mobj_t *actor)
{
  if (!actor->target) return;

  a_face_target(actor);
  a_pain_shoot_skull(actor, actor->angle);
}

void a_pain_die(mobj_t *actor)
{
  a_fall(actor);
  a_pain_shoot_skull(actor, actor->angle + ANG90);
  a_pain_shoot_skull(actor, actor->angle + ANG180);
  a_pain_shoot_skull(actor, actor->angle + ANG270);
}

void a_scream(mobj_t *actor)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  int sound;

  switch (actor->info->deathsound)
    {
    case 0:
      return;

    case SFX_PODTH1:
    case SFX_PODTH2:
    case SFX_PODTH3:
      sound = SFX_PODTH1 + p_random() % 3;
      break;

    case SFX_BGDTH1:
    case SFX_BGDTH2:
      sound = SFX_BGDTH1 + p_random() % 2;
      break;

    default:
      sound = actor->info->deathsound;
      break;
    }

  /* Check for bosses. */

  if (actor->type == MT_SPIDER || actor->type == MT_CYBORG)
    {
      s_start_sound(NULL, sound); /* full volume */
    }
  else
    s_start_sound(actor, sound);
#endif
}

void a_xscream(mobj_t *actor)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(actor, SFX_SLOP);
#endif
}

void a_pain(mobj_t *actor)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (actor->info->painsound) s_start_sound(actor, actor->info->painsound);
#endif
}

void a_fall(mobj_t *actor)
{
  /* actor is on ground, it can be walked over */

  actor->flags &= ~MF_SOLID;

  /* So change this if corpse objects are meant to be obstacles. */
}

void a_explode(mobj_t *thingy)
{
  p_radius_attack(thingy, thingy->target, 128);
}

/* a_boss_death
 * Possibly trigger special effects if on first boss level
 */

void a_boss_death(mobj_t *mo)
{
  thinker_t *th;
  mobj_t *mo2;
  line_t junk;
  int i;

  if (gamemode == commercial)
    {
      if (gamemap != 7) return;

      if ((mo->type != MT_FATSO) && (mo->type != MT_BABY)) return;
    }
  else
    {
      if (!check_boss_end(mo->type))
        {
          return;
        }
    }

  /* make sure there is a player alive for victory */

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i] && players[i].health > 0) break;
    }

  if (i == MAXPLAYERS) return; /* no one left alive, so do not end game */

  /* scan the remaining thinkers to see if all bosses are dead */

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      if (th->function.acp1 != (actionf_p1)p_mobj_thinker) continue;

      mo2 = (mobj_t *)th;
      if (mo2 != mo && mo2->type == mo->type && mo2->health > 0)
        {
          return; /* other boss not dead */
        }
    }

  /* victory! */

  if (gamemode == commercial)
    {
      if (gamemap == 7)
        {
          if (mo->type == MT_FATSO)
            {
              junk.tag = 666;
              ev_do_floor(&junk, FLOOR_LOWERFLOORTOLOWEST);
              return;
            }

          if (mo->type == MT_BABY)
            {
              junk.tag = 667;
              ev_do_floor(&junk, FLOOR_RAISETOTEXTURE);
              return;
            }
        }
    }
  else
    {
      switch (gameepisode)
        {
        case 1:
          junk.tag = 666;
          ev_do_floor(&junk, FLOOR_LOWERFLOORTOLOWEST);
          return;
          break;

        case 4:
          switch (gamemap)
            {
            case 6:
              junk.tag = 666;
              ev_do_door(&junk, VLD_BLAZEOPEN);
              return;
              break;

            case 8:
              junk.tag = 666;
              ev_do_floor(&junk, FLOOR_LOWERFLOORTOLOWEST);
              return;
              break;
            }
        }
    }

  g_exit_level();
}

void a_hoof(mobj_t *mo)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(mo, SFX_HOOF);
#endif
  a_chase(mo);
}

void a_metal(mobj_t *mo)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(mo, SFX_METAL);
#endif
  a_chase(mo);
}

void a_baby_metal(mobj_t *mo)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(mo, SFX_BSPWLK);
#endif
  a_chase(mo);
}

void a_open_shotgun2(player_t *player, pspdef_t *psp)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_DBOPN);
#endif
}

void a_load_shotgun2(player_t *player, pspdef_t *psp)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_DBLOAD);
#endif
}

void a_close_shotgun2(player_t *player, pspdef_t *psp)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_DBCLS);
#endif
  a_refire(player, psp);
}

void a_brain_awake(mobj_t *mo)
{
  thinker_t *thinker;
  mobj_t *m;

  /* find all the target spots */

  numbraintargets = 0;
  braintargeton = 0;

  for (thinker = thinkercap.next; thinker != &thinkercap;
       thinker = thinker->next)
    {
      if (thinker->function.acp1 != (actionf_p1)p_mobj_thinker)
        {
          continue; /* not a mobj */
        }

      m = (mobj_t *)thinker;

      if (m->type == MT_BOSSTARGET)
        {
          braintargets[numbraintargets] = m;
          numbraintargets++;
        }
    }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(NULL, SFX_BOSSIT);
#endif
}

void a_brain_pain(mobj_t *mo)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(NULL, SFX_BOSPN);
#endif
}

void a_brain_scream(mobj_t *mo)
{
  int x;
  int y;
  int z;
  mobj_t *th;

  for (x = mo->x - 196 * FRACUNIT; x < mo->x + 320 * FRACUNIT;
       x += FRACUNIT * 8)
    {
      y = mo->y - 320 * FRACUNIT;
      z = 128 + p_random() * 2 * FRACUNIT;
      th = p_spawn_mobj(x, y, z, MT_ROCKET);
      th->momz = p_random() * 512;

      p_set_mobj_state(th, S_BRAINEXPLODE1);

      th->tics -= p_random() & 7;
      if (th->tics < 1) th->tics = 1;
    }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(NULL, SFX_BOSDTH);
#endif
}

void a_brain_explode(mobj_t *mo)
{
  int x;
  int y;
  int z;
  mobj_t *th;

  x = mo->x + p_sub_random() * 2048;
  y = mo->y;
  z = 128 + p_random() * 2 * FRACUNIT;
  th = p_spawn_mobj(x, y, z, MT_ROCKET);
  th->momz = p_random() * 512;

  p_set_mobj_state(th, S_BRAINEXPLODE1);

  th->tics -= p_random() & 7;
  if (th->tics < 1) th->tics = 1;
}

void a_brain_die(mobj_t *mo)
{
  g_exit_level();
}

void a_brain_split(mobj_t *mo)
{
  mobj_t *targ;
  mobj_t *newmobj;

  static int easy = 0;

  easy ^= 1;
  if (gameskill <= sk_easy && (!easy)) return;

  /* shoot a cube at current target */

  targ = braintargets[braintargeton];
  if (numbraintargets == 0)
    {
      i_error("a_brain_split: numbraintargets was 0 (vanilla crashes here)");
    }

  braintargeton = (braintargeton + 1) % numbraintargets;

  /* spawn brain missile */

  newmobj = p_spawn_missile(mo, targ, MT_SPAWNSHOT);
  newmobj->target = targ;
  newmobj->reactiontime =
      ((targ->y - mo->y) / newmobj->momy) / newmobj->state->tics;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(NULL, SFX_BOSPIT);
#endif
}

void a_spawn_fly(mobj_t *mo)
{
  mobj_t *newmobj;
  mobj_t *fog;
  mobj_t *targ;
  int r;
  mobjtype_t type;

  if (--mo->reactiontime) return; /* still flying */

  targ = p_subst_null_mobj(mo->target);

  /* First spawn teleport fog. */

  fog = p_spawn_mobj(targ->x, targ->y, targ->z, MT_SPAWNFIRE);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(fog, SFX_TELEPT);
#else
  UNUSED(fog);
#endif

  /* Randomly select monster to spawn. */

  r = p_random();

  /* Probability distribution (kind of :), decreasing likelihood. */

  if (r < 50)
    type = MT_TROOP;
  else if (r < 90)
    type = MT_SERGEANT;
  else if (r < 120)
    type = MT_SHADOWS;
  else if (r < 130)
    type = MT_PAIN;
  else if (r < 160)
    type = MT_HEAD;
  else if (r < 162)
    type = MT_VILE;
  else if (r < 172)
    type = MT_UNDEAD;
  else if (r < 192)
    type = MT_BABY;
  else if (r < 222)
    type = MT_FATSO;
  else if (r < 246)
    type = MT_KNIGHT;
  else
    type = MT_BRUISER;

  newmobj = p_spawn_mobj(targ->x, targ->y, targ->z, type);
  if (p_look_for_players(newmobj, true))
    p_set_mobj_state(newmobj, newmobj->info->seestate);

  /* telefrag anything in this spot */

  p_teleport_move(newmobj, newmobj->x, newmobj->y);

  /* remove self (i.e., cube). */

  p_remove_mobj(mo);
}

/* travelling cube sound */

void a_spawn_sound(mobj_t *mo)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(mo, SFX_BOSCUB);
#endif
  a_spawn_fly(mo);
}

void a_player_screm(mobj_t *mo)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND

  /* Default death sound. */

  int sound = SFX_PLDETH;

  if ((gamemode == commercial) && (mo->health < -50))
    {
      /* IF THE PLAYER DIES LESS THAN -50% WITHOUT GIBBING */

      sound = SFX_PDIEHI;
    }

  s_start_sound(mo, sound);
#else
  UNUSED(mo);
#endif
}
