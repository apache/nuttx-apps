/****************************************************************************
 * apps/games/NXDoom/src/doom/p_map.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard, Andrey Budko
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
 *  Movement, collision handling.
 *  Shooting and aiming.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "deh_misc.h"

#include "i_system.h"
#include "m_bbox.h"
#include "m_random.h"

#include "doomdef.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "doomstat.h"
#include "r_state.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Spechit overrun magic value.
 *
 * This is the value used by PrBoom-plus.  I think the value below is
 * actually better and works with more demos.  However, I think
 * it's better for the spechits emulation to be compatible with
 * PrBoom-plus, at least so that the big spechits emulation list
 * on Doomworld can also be used with Chocolate Doom.
 */

#define DEFAULT_SPECHIT_MAGIC 0x01C09C98

/* This is from a post by myk on the Doomworld forums,
 * outputted from entryway's spechit_magic generator for
 * s205n546.lmp.  The _exact_ value of this isn't too
 * important; as long as it is in the right general
 * range, it will usually work.  Otherwise, we can use
 * the generator (hacked doom2.exe) and provide it
 * with -spechit.
 */

/* #define DEFAULT_SPECHIT_MAGIC 0x84f968e8 */

/****************************************************************************
 * Public Data
 ****************************************************************************/

fixed_t tmbbox[4];
mobj_t *tmthing;
int tmflags;
fixed_t tmx;
fixed_t tmy;

/* If "floatok" true, move would be ok
 * if within "tmfloorz - tmceilingz".
 */

boolean floatok;

fixed_t tmfloorz;
fixed_t tmceilingz;
fixed_t tmdropoffz;

/* keep track of the line that lowers the ceiling,
 * so missiles don't explode against sky hack walls
 */

line_t *ceilingline;

/* keep track of special lines as they are hit,
 * but don't process them until the move is proven valid
 */

line_t *spechit[MAXSPECIALCROSS];
int numspechit;

/* SLIDE MOVE
 * Allows the player to slide along any angled walls.
 */

fixed_t bestslidefrac;
fixed_t secondslidefrac;

line_t *bestslideline;
line_t *secondslideline;

mobj_t *slidemo;

fixed_t tmxmove;
fixed_t tmymove;

/* p_line_attack */

mobj_t *linetarget; /* who got hit (or NULL) */
mobj_t *shootthing;

/* Height if not aiming up or down
 * ???: use slope for monsters?
 */

fixed_t shootz;

int la_damage;
fixed_t attackrange;

fixed_t aimslope;

/* USE LINES */

mobj_t *usething;

/* RADIUS ATTACK */

mobj_t *bombsource;
mobj_t *bombspot;
int bombdamage;

/* SECTOR HEIGHT CHANGING
 * After modifying a sectors floor or ceiling height,
 * call this routine to adjust the positions
 * of all things that touch the sector.
 *
 * If anything doesn't fit anymore, true will be returned.
 *
 * If crunch is true, they will take damage as they are being crushed.
 *
 * If Crunch is false, you should set the sector height back the way it was
 * and call p_change_sector again to undo the changes.
 */

boolean crushchange;
boolean nofit;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* TELEPORT MOVE */

static boolean pit_stomp_thing(mobj_t *thing)
{
  fixed_t blockdist;

  if (!(thing->flags & MF_SHOOTABLE)) return true;

  blockdist = thing->radius + tmthing->radius;

  if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
    {
      return true; /* didn't hit it */
    }

  /* don't clip against self */

  if (thing == tmthing) return true;

  /* monsters don't stomp things except on boss level */

  if (!tmthing->player && gamemap != 30) return false;

  p_damage_mobj(thing, tmthing, tmthing, 10000);

  return true;
}

/* Code to emulate the behavior of Vanilla Doom when encountering an overrun
 * of the spechit array.  This is by Andrey Budko (e6y) and comes from his
 * PrBoom plus port.  A big thanks to Andrey for this.
 */

static void spechit_overrun(line_t *ld)
{
  static unsigned int baseaddr = 0;
  unsigned int addr;

  if (baseaddr == 0)
    {
      int p;

      /* This is the first time we have had an overrun.  Work out
       * what base address we are going to use.
       * Allow a spechit value to be specified on the command line.
       */

      /* @category compat
       * @arg <n>
       *
       * Use the specified magic value when emulating spechit overruns.
       */

      p = m_check_parm_with_args("-spechit", 1);

      if (p > 0)
        {
          m_str_to_int(myargv[p + 1], (int *)&baseaddr);
        }
      else
        {
          baseaddr = DEFAULT_SPECHIT_MAGIC;
        }
    }

  /* Calculate address used in doom2.exe */

  addr = baseaddr + (ld - lines) * 0x3e;

  switch (numspechit)
    {
    case 9:
    case 10:
    case 11:
    case 12:
      tmbbox[numspechit - 9] = addr;
      break;
    case 13:
      crushchange = addr;
      break;
    case 14:
      nofit = addr;
      break;
    default:
      fprintf(stderr,
              "SpechitOverrun: Warning: unable to emulate"
              "an overrun where numspechit=%i\n",
              numspechit);
      break;
    }
}

/* pit_check_line
 * Adjusts tmfloorz and tmceilingz as lines are contacted
 */

static boolean pit_check_line(line_t *ld)
{
  if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
      tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
      tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
      tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
    {
      return true;
    }

  if (p_box_on_line_side(tmbbox, ld) != -1) return true;

  /* A line has been hit */

  /* The moving thing's destination position will cross
   * the given line.
   * If this should not be allowed, return false.
   * If the line is special, keep track of it
   * to process later if the move is proven ok.
   * NOTE: specials are NOT sorted by order,
   * so two special lines that are only 8 pixels apart
   * could be crossed in either order.
   */

  if (!ld->backsector) return false; /* one sided line */

  if (!(tmthing->flags & MF_MISSILE))
    {
      if (ld->flags & ML_BLOCKING)
        {
          return false; /* explicitly blocking everything */
        }

      if (!tmthing->player && ld->flags & ML_BLOCKMONSTERS)
        {
          return false; /* block monsters only */
        }
    }

  /* set openrange, opentop, openbottom */

  p_line_opening(ld);

  /* adjust floor / ceiling heights */

  if (opentop < tmceilingz)
    {
      tmceilingz = opentop;
      ceilingline = ld;
    }

  if (openbottom > tmfloorz) tmfloorz = openbottom;

  if (lowfloor < tmdropoffz) tmdropoffz = lowfloor;

  /* if contacted a special line, add it to the list */

  if (ld->special)
    {
      spechit[numspechit] = ld;
      numspechit++;

      /* fraggle: spechits overrun emulation code from prboom-plus */

      if (numspechit > MAXSPECIALCROSS_ORIGINAL)
        {
          spechit_overrun(ld);
        }
    }

  return true;
}

/* pit_radius_attack
 * "bombsource" is the creature that caused the explosion at "bombspot".
 */

static boolean pit_radius_attack(mobj_t *thing)
{
  fixed_t dx;
  fixed_t dy;
  fixed_t dist;

  if (!(thing->flags & MF_SHOOTABLE)) return true;

  /* Boss spider and cyborg take no damage from concussion. */

  if (thing->type == MT_CYBORG || thing->type == MT_SPIDER) return true;

  dx = abs(thing->x - bombspot->x);
  dy = abs(thing->y - bombspot->y);

  dist = dx > dy ? dx : dy;
  dist = (dist - thing->radius) >> FRACBITS;

  if (dist < 0) dist = 0;

  if (dist >= bombdamage) return true; /* out of range */

  if (p_check_sight(thing, bombspot))
    {
      /* must be in direct path */

      p_damage_mobj(thing, bombspot, bombsource, bombdamage - dist);
    }

  return true;
}

/* p_things_height_clip
 * Takes a valid thing and adjusts the thing->floorz, thing->ceilingz, and
 * possibly thing->z.
 *
 * This is called for all nearby monsters whenever a sector changes height.
 *
 * If the thing doesn't fit, the z will be set to the lowest value and false
 * will be returned.
 */

static boolean p_things_height_clip(mobj_t *thing)
{
  boolean onfloor;

  onfloor = (thing->z == thing->floorz);

  p_check_position(thing, thing->x, thing->y);

  /* what about stranding a monster partially off an edge? */

  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;

  if (onfloor)
    {
      /* walking monsters rise and fall with the floor */

      thing->z = thing->floorz;
    }
  else
    {
      /* don't adjust a floating monster unless forced to */

      if (thing->z + thing->height > thing->ceilingz)
        thing->z = thing->ceilingz - thing->height;
    }

  if (thing->ceilingz - thing->floorz < thing->height) return false;

  return true;
}

/* p_hit_slide_line
 * Adjusts the xmove / ymove so that the next move will slide along the wall.
 */

static void p_hit_slide_line(line_t *ld)
{
  int side;

  angle_t lineangle;
  angle_t moveangle;
  angle_t deltaangle;

  fixed_t movelen;
  fixed_t newlen;

  if (ld->slopetype == ST_HORIZONTAL)
    {
      tmymove = 0;
      return;
    }

  if (ld->slopetype == ST_VERTICAL)
    {
      tmxmove = 0;
      return;
    }

  side = p_point_on_line_side(slidemo->x, slidemo->y, ld);

  lineangle = r_point_to_angle2(0, 0, ld->dx, ld->dy);

  if (side == 1) lineangle += ANG180;

  moveangle = r_point_to_angle2(0, 0, tmxmove, tmymove);
  deltaangle = moveangle - lineangle;

  if (deltaangle > ANG180) deltaangle += ANG180;

  lineangle >>= ANGLETOFINESHIFT;
  deltaangle >>= ANGLETOFINESHIFT;

  movelen = p_approx_distance(tmxmove, tmymove);
  newlen = fixed_mul(movelen, finecosine[deltaangle]);

  tmxmove = fixed_mul(newlen, finecosine[lineangle]);
  tmymove = fixed_mul(newlen, finesine[lineangle]);
}

/* ptr_slide_traverse */

static boolean ptr_slide_traverse(intercept_t *in)
{
  line_t *li;

  if (!in->isaline) i_error("ptr_slide_traverse: not a line?");

  li = in->d.line;

  if (!(li->flags & ML_TWOSIDED))
    {
      if (p_point_on_line_side(slidemo->x, slidemo->y, li))
        {
          return true; /* don't hit the back side */
        }

      goto isblocking;
    }

  /* set openrange, opentop, openbottom */

  p_line_opening(li);

  if (openrange < slidemo->height)
    {
      goto isblocking; /* doesn't fit */
    }

  if (opentop - slidemo->z < slidemo->height)
    {
      goto isblocking; /* mobj is too high */
    }

  if (openbottom - slidemo->z > 24 * FRACUNIT)
    {
      goto isblocking; /* too big a step up */
    }

  /* this line doesn't block movement */

  return true;

  /* the line does block movement,
   * see if it is closer than best so far
   */

isblocking:
  if (in->frac < bestslidefrac)
    {
      secondslidefrac = bestslidefrac;
      secondslideline = bestslideline;
      bestslidefrac = in->frac;
      bestslideline = li;
    }

  return false; /* stop */
}

static boolean ptr_use_traverse(intercept_t *in)
{
  int side;

  if (!in->d.line->special)
    {
      p_line_opening(in->d.line);
      if (openrange <= 0)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(usething, SFX_NOWAY);
#endif

          /* can't use through a wall */

          return false;
        }

      /* not a special line, but keep checking */

      return true;
    }

  side = 0;
  if (p_point_on_line_side(usething->x, usething->y, in->d.line) == 1)
    {
      side = 1;
    }

  /* return false; don't use back side */

  p_use_special_line(usething, in->d.line, side);

  /* can't use for than one special line in a row */

  return false;
}

/* ptr_aim_traverse
 * Sets linetaget and aimslope when a target is aimed at.
 */

static boolean ptr_aim_traverse(intercept_t *in)
{
  line_t *li;
  mobj_t *th;
  fixed_t slope;
  fixed_t thingtopslope;
  fixed_t thingbottomslope;
  fixed_t dist;

  if (in->isaline)
    {
      li = in->d.line;

      if (!(li->flags & ML_TWOSIDED)) return false; /* stop */

      /* Crosses a two sided line.
       * A two sided line will restrict
       * the possible target ranges.
       */

      p_line_opening(li);

      if (openbottom >= opentop) return false; /* stop */

      dist = fixed_mul(attackrange, in->frac);

      if (li->backsector == NULL ||
          li->frontsector->floorheight != li->backsector->floorheight)
        {
          slope = fixed_div(openbottom - shootz, dist);
          if (slope > bottomslope) bottomslope = slope;
        }

      if (li->backsector == NULL ||
          li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
          slope = fixed_div(opentop - shootz, dist);
          if (slope < topslope) topslope = slope;
        }

      if (topslope <= bottomslope) return false; /* stop */

      return true; /* shot continues */
    }

  /* shoot a thing */

  th = in->d.thing;
  if (th == shootthing) return true; /* can't shoot self */

  if (!(th->flags & MF_SHOOTABLE)) return true; /* corpse or something */

  /* check angles to see if the thing can be aimed at */

  dist = fixed_mul(attackrange, in->frac);
  thingtopslope = fixed_div(th->z + th->height - shootz, dist);

  if (thingtopslope < bottomslope) return true; /* shot over the thing */

  thingbottomslope = fixed_div(th->z - shootz, dist);

  if (thingbottomslope > topslope) return true; /* shot under the thing */

  /* this thing can be hit! */

  if (thingtopslope > topslope) thingtopslope = topslope;

  if (thingbottomslope < bottomslope) thingbottomslope = bottomslope;

  aimslope = (thingtopslope + thingbottomslope) / 2;
  linetarget = th;

  return false; /* don't go any farther */
}

/* ptr_shoot_traverse */

static boolean ptr_shoot_traverse(intercept_t *in)
{
  fixed_t x;
  fixed_t y;
  fixed_t z;
  fixed_t frac;

  line_t *li;

  mobj_t *th;

  fixed_t slope;
  fixed_t dist;
  fixed_t thingtopslope;
  fixed_t thingbottomslope;

  if (in->isaline)
    {
      li = in->d.line;

      if (li->special) p_shoot_special_line(shootthing, li);

      if (!(li->flags & ML_TWOSIDED)) goto hitline;

      /* crosses a two sided line */

      p_line_opening(li);

      dist = fixed_mul(attackrange, in->frac);

      /* e6y: emulation of missed back side on two-sided lines.
       * backsector can be NULL when emulating missing back side.
       */

      if (li->backsector == NULL)
        {
          slope = fixed_div(openbottom - shootz, dist);
          if (slope > aimslope) goto hitline;

          slope = fixed_div(opentop - shootz, dist);
          if (slope < aimslope) goto hitline;
        }
      else
        {
          if (li->frontsector->floorheight
                  != li->backsector->floorheight)
            {
              slope = fixed_div(openbottom - shootz, dist);
              if (slope > aimslope) goto hitline;
            }

          if (li->frontsector->ceilingheight
                  != li->backsector->ceilingheight)
            {
              slope = fixed_div(opentop - shootz, dist);
              if (slope < aimslope) goto hitline;
            }
        }

      /* shot continues */

      return true;

      /* hit line */

    hitline:

      /* position a bit closer */

      frac = in->frac - fixed_div(4 * FRACUNIT, attackrange);
      x = trace.x + fixed_mul(trace.dx, frac);
      y = trace.y + fixed_mul(trace.dy, frac);
      z = shootz + fixed_mul(aimslope, fixed_mul(frac, attackrange));

      if (li->frontsector->ceilingpic == skyflatnum)
        {
          /* don't shoot the sky! */

          if (z > li->frontsector->ceilingheight) return false;

          /* it's a sky hack wall */

          if (li->backsector &&
              li->backsector->ceilingpic == skyflatnum)
            {
              return false;
            }
        }

      /* Spawn bullet puffs. */

      p_spawn_puff(x, y, z);

      /* don't go any farther */

      return false;
    }

  /* shoot a thing */

  th = in->d.thing;
  if (th == shootthing) return true; /* can't shoot self */

  if (!(th->flags & MF_SHOOTABLE)) return true; /* corpse or something */

  /* check angles to see if the thing can be aimed at */

  dist = fixed_mul(attackrange, in->frac);
  thingtopslope = fixed_div(th->z + th->height - shootz, dist);

  if (thingtopslope < aimslope) return true; /* shot over the thing  */

  thingbottomslope = fixed_div(th->z - shootz, dist);

  if (thingbottomslope > aimslope) return true; /* shot under the thing */

  /* hit thing position a bit closer */

  frac = in->frac - fixed_div(10 * FRACUNIT, attackrange);

  x = trace.x + fixed_mul(trace.dx, frac);
  y = trace.y + fixed_mul(trace.dy, frac);
  z = shootz + fixed_mul(aimslope, fixed_mul(frac, attackrange));

  /* Spawn bullet puffs or blod spots,
   * depending on target type.
   */

  if (in->d.thing->flags & MF_NOBLOOD)
    p_spawn_puff(x, y, z);
  else
    p_spawn_blood(x, y, z, la_damage);

  if (la_damage) p_damage_mobj(th, shootthing, shootthing, la_damage);

  /* don't go any farther */

  return false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* p_teleport_move */

boolean p_teleport_move(mobj_t *thing, fixed_t x, fixed_t y)
{
  int xl;
  int xh;
  int yl;
  int yh;
  int bx;
  int by;

  subsector_t *newsubsec;

  /* kill anything occupying the position */

  tmthing = thing;
  tmflags = thing->flags;

  tmx = x;
  tmy = y;

  tmbbox[BOXTOP] = y + tmthing->radius;
  tmbbox[BOXBOTTOM] = y - tmthing->radius;
  tmbbox[BOXRIGHT] = x + tmthing->radius;
  tmbbox[BOXLEFT] = x - tmthing->radius;

  newsubsec = r_point_in_subsector(x, y);
  ceilingline = NULL;

  /* The base floor/ceiling is from the subsector
   * that contains the point.
   * Any contacted lines the step closer together
   * will adjust them.
   */

  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;

  validcount++;
  numspechit = 0;

  /* stomp on any things contacted */

  xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

  for (bx = xl; bx <= xh; bx++)
    {
      for (by = yl; by <= yh; by++)
        {
          if (!p_block_things_iterator(bx, by, pit_stomp_thing))
            {
              return false;
            }
        }
    }

  /* the move is ok,
   * so link the thing into its new position
   */

  p_unset_thing_position(thing);

  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;
  thing->x = x;
  thing->y = y;

  p_set_thing_position(thing);

  return true;
}

boolean pit_check_thing(mobj_t *thing)
{
  fixed_t blockdist;
  boolean solid;
  int damage;

  if (!(thing->flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE))) return true;

  blockdist = thing->radius + tmthing->radius;

  if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
    {
      return true; /* didn't hit it */
    }

  /* don't clip against self */

  if (thing == tmthing) return true;

  /* check for skulls slamming into things */

  if (tmthing->flags & MF_SKULLFLY)
    {
      damage = ((p_random() % 8) + 1) * tmthing->info->damage;

      p_damage_mobj(thing, tmthing, tmthing, damage);

      tmthing->flags &= ~MF_SKULLFLY;
      tmthing->momx = tmthing->momy = tmthing->momz = 0;

      p_set_mobj_state(tmthing, tmthing->info->spawnstate);

      return false; /* stop moving */
    }

  /* missiles can hit other things */

  if (tmthing->flags & MF_MISSILE)
    {
      /* see if it went over / under */

      if (tmthing->z > thing->z + thing->height)
        {
          return true; /* overhead */
        }

      if (tmthing->z + tmthing->height < thing->z)
        {
          return true; /* underneath */
        }

      if (tmthing->target &&
          (tmthing->target->type == thing->type ||
           (tmthing->target->type == MT_KNIGHT &&
            thing->type == MT_BRUISER) ||
           (tmthing->target->type == MT_BRUISER &&
            thing->type == MT_KNIGHT)))
        {
          /* Don't hit same species as originator. */

          if (thing == tmthing->target) return true;

          /* sdh: Add deh_species_infighting here.  We can override the
           * "monsters of the same species can't hurt each other" behavior
           * through dehacked patches
           */

          if (thing->type != MT_PLAYER && !deh_species_infighting)
            {
              /* Explode, but do no damage.
               * Let players missile other players.
               */

              return false;
            }
        }

      if (!(thing->flags & MF_SHOOTABLE))
        {
          /* didn't do any damage */

          return !(thing->flags & MF_SOLID);
        }

      /* damage / explode */

      damage = ((p_random() % 8) + 1) * tmthing->info->damage;
      p_damage_mobj(thing, tmthing, tmthing->target, damage);

      /* don't traverse any more */

      return false;
    }

  /* check for special pickup */

  if (thing->flags & MF_SPECIAL)
    {
      solid = (thing->flags & MF_SOLID) != 0;
      if (tmflags & MF_PICKUP)
        {
          /* can remove thing */

          p_touch_special_thing(thing, tmthing);
        }

      return !solid;
    }

  return !(thing->flags & MF_SOLID);
}

/* MOVEMENT CLIPPING */

/* p_check_position
 * This is purely informative, nothing is modified
 * (except things picked up).
 *
 * in:
 *  a mobj_t (can be valid or invalid)
 *  a position to be checked
 *   (doesn't need to be related to the mobj_t->x,y)
 *
 * during:
 *  special things are touched if MF_PICKUP
 *  early out on solid lines?
 *
 * out:
 *  newsubsec
 *  floorz
 *  ceilingz
 *  tmdropoffz
 *   the lowest point contacted
 *   (monsters won't move to a dropoff)
 *  speciallines[]
 *  numspeciallines
 */

boolean p_check_position(mobj_t *thing, fixed_t x, fixed_t y)
{
  int xl;
  int xh;
  int yl;
  int yh;
  int bx;
  int by;
  subsector_t *newsubsec;

  tmthing = thing;
  tmflags = thing->flags;

  tmx = x;
  tmy = y;

  tmbbox[BOXTOP] = y + tmthing->radius;
  tmbbox[BOXBOTTOM] = y - tmthing->radius;
  tmbbox[BOXRIGHT] = x + tmthing->radius;
  tmbbox[BOXLEFT] = x - tmthing->radius;

  newsubsec = r_point_in_subsector(x, y);
  ceilingline = NULL;

  /* The base floor / ceiling is from the subsector
   * that contains the point.
   * Any contacted lines the step closer together
   * will adjust them.
   */

  tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
  tmceilingz = newsubsec->sector->ceilingheight;

  validcount++;
  numspechit = 0;

  if (tmflags & MF_NOCLIP) return true;

  /* Check things first, possibly picking things up.
   * The bounding box is extended by MAXRADIUS
   * because mobj_ts are grouped into mapblocks
   * based on their origin point, and can overlap
   * into adjacent blocks by up to MAXRADIUS units.
   */

  xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

  for (bx = xl; bx <= xh; bx++)
    {
      for (by = yl; by <= yh; by++)
        {
          if (!p_block_things_iterator(bx, by, pit_check_thing))
            {
              return false;
            }
        }
    }

  /* check lines */

  xl = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
  xh = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
  yl = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
  yh = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

  for (bx = xl; bx <= xh; bx++)
    {
      for (by = yl; by <= yh; by++)
        {
          if (!p_block_lines_iterator(bx, by, pit_check_line))
            {
              return false;
            }
        }
    }

  return true;
}

/* p_try_move
 * Attempt to move to a new position,
 * crossing special lines unless MF_TELEPORT is set.
 */

boolean p_try_move(mobj_t *thing, fixed_t x, fixed_t y)
{
  fixed_t oldx;
  fixed_t oldy;
  int side;
  int oldside;
  line_t *ld;

  floatok = false;
  if (!p_check_position(thing, x, y))
    {
      return false; /* solid wall or thing */
    }

  if (!(thing->flags & MF_NOCLIP))
    {
      if (tmceilingz - tmfloorz < thing->height)
        {
          return false; /* doesn't fit */
        }

      floatok = true;

      if (!(thing->flags & MF_TELEPORT) &&
          tmceilingz - thing->z < thing->height)
        {
          return false; /* mobj must lower itself to fit */
        }

      if (!(thing->flags & MF_TELEPORT) &&
          tmfloorz - thing->z > 24 * FRACUNIT)
        {
          return false; /* too big a step up */
        }

      if (!(thing->flags & (MF_DROPOFF | MF_FLOAT)) &&
          tmfloorz - tmdropoffz > 24 * FRACUNIT)
        {
          return false; /* don't stand over a dropoff */
        }
    }

  /* the move is ok,
   * so link the thing into its new position
   */

  p_unset_thing_position(thing);

  oldx = thing->x;
  oldy = thing->y;
  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;
  thing->x = x;
  thing->y = y;

  p_set_thing_position(thing);

  /* if any special lines were hit, do the effect */

  if (!(thing->flags & (MF_TELEPORT | MF_NOCLIP)))
    {
      while (numspechit--)
        {
          /* see if the line was crossed */

          ld = spechit[numspechit];
          side = p_point_on_line_side(thing->x, thing->y, ld);
          oldside = p_point_on_line_side(oldx, oldy, ld);
          if (side != oldside)
            {
              if (ld->special)
                {
                  p_cross_special_line(ld - lines, oldside, thing);
                }
            }
        }
    }

  return true;
}

/* p_slide_move
 * The momx / momy move is bad, so try to slide
 * along a wall.
 * Find the first line hit, move flush to it,
 * and slide along it
 *
 * This is a kludgy mess.
 */

void p_slide_move(mobj_t *mo)
{
  fixed_t leadx;
  fixed_t leady;
  fixed_t trailx;
  fixed_t traily;
  fixed_t newx;
  fixed_t newy;
  int hitcount;

  slidemo = mo;
  hitcount = 0;

retry:
  if (++hitcount == 3) goto stairstep; /* don't loop forever */

  /* trace along the three leading corners */

  if (mo->momx > 0)
    {
      leadx = mo->x + mo->radius;
      trailx = mo->x - mo->radius;
    }
  else
    {
      leadx = mo->x - mo->radius;
      trailx = mo->x + mo->radius;
    }

  if (mo->momy > 0)
    {
      leady = mo->y + mo->radius;
      traily = mo->y - mo->radius;
    }
  else
    {
      leady = mo->y - mo->radius;
      traily = mo->y + mo->radius;
    }

  bestslidefrac = FRACUNIT + 1;

  p_path_traverse(leadx, leady, leadx + mo->momx, leady + mo->momy,
                 PT_ADDLINES, ptr_slide_traverse);
  p_path_traverse(trailx, leady, trailx + mo->momx, leady + mo->momy,
                 PT_ADDLINES, ptr_slide_traverse);
  p_path_traverse(leadx, traily, leadx + mo->momx, traily + mo->momy,
                 PT_ADDLINES, ptr_slide_traverse);

  /* move up to the wall */

  if (bestslidefrac == FRACUNIT + 1)
    {
      /* the move most have hit the middle, so stairstep */

    stairstep:
      if (!p_try_move(mo, mo->x, mo->y + mo->momy))
        p_try_move(mo, mo->x + mo->momx, mo->y);
      return;
    }

  /* fudge a bit to make sure it doesn't hit */

  bestslidefrac -= 0x800;
  if (bestslidefrac > 0)
    {
      newx = fixed_mul(mo->momx, bestslidefrac);
      newy = fixed_mul(mo->momy, bestslidefrac);

      if (!p_try_move(mo, mo->x + newx, mo->y + newy)) goto stairstep;
    }

  /* Now continue along the wall.
   * First calculate remainder.
   */

  bestslidefrac = FRACUNIT - (bestslidefrac + 0x800);

  if (bestslidefrac > FRACUNIT) bestslidefrac = FRACUNIT;

  if (bestslidefrac <= 0) return;

  tmxmove = fixed_mul(mo->momx, bestslidefrac);
  tmymove = fixed_mul(mo->momy, bestslidefrac);

  p_hit_slide_line(bestslideline); /* clip the moves */

  mo->momx = tmxmove;
  mo->momy = tmymove;

  if (!p_try_move(mo, mo->x + tmxmove, mo->y + tmymove))
    {
      goto retry;
    }
}

/* p_aim_line_attack */

fixed_t p_aim_line_attack(mobj_t *t1, angle_t angle, fixed_t distance)
{
  fixed_t x2;
  fixed_t y2;

  t1 = p_subst_null_mobj(t1);

  angle >>= ANGLETOFINESHIFT;
  shootthing = t1;

  x2 = t1->x + (distance >> FRACBITS) * finecosine[angle];
  y2 = t1->y + (distance >> FRACBITS) * finesine[angle];
  shootz = t1->z + (t1->height >> 1) + 8 * FRACUNIT;

  /* can't shoot outside view angles */

  topslope = (SCREENHEIGHT / 2) * FRACUNIT / (SCREENWIDTH / 2);
  bottomslope = -(SCREENHEIGHT / 2) * FRACUNIT / (SCREENWIDTH / 2);

  attackrange = distance;
  linetarget = NULL;

  p_path_traverse(t1->x, t1->y, x2, y2, PT_ADDLINES | PT_ADDTHINGS,
                 ptr_aim_traverse);

  if (linetarget) return aimslope;

  return 0;
}

/* p_line_attack
 *
 * If damage == 0, it is just a test trace that will leave linetarget set.
 */

void p_line_attack(mobj_t *t1, angle_t angle, fixed_t distance,
        fixed_t slope, int damage)
{
  fixed_t x2;
  fixed_t y2;

  angle >>= ANGLETOFINESHIFT;
  shootthing = t1;
  la_damage = damage;
  x2 = t1->x + (distance >> FRACBITS) * finecosine[angle];
  y2 = t1->y + (distance >> FRACBITS) * finesine[angle];
  shootz = t1->z + (t1->height >> 1) + 8 * FRACUNIT;
  attackrange = distance;
  aimslope = slope;

  p_path_traverse(t1->x, t1->y, x2, y2, PT_ADDLINES | PT_ADDTHINGS,
                 ptr_shoot_traverse);
}

/* p_use_lines
 * Looks for special lines in front of the player to activate.
 */

void p_use_lines(player_t *player)
{
  int angle;
  fixed_t x1;
  fixed_t y1;
  fixed_t x2;
  fixed_t y2;

  usething = player->mo;

  angle = player->mo->angle >> ANGLETOFINESHIFT;

  x1 = player->mo->x;
  y1 = player->mo->y;
  x2 = x1 + (USERANGE >> FRACBITS) * finecosine[angle];
  y2 = y1 + (USERANGE >> FRACBITS) * finesine[angle];

  p_path_traverse(x1, y1, x2, y2, PT_ADDLINES, ptr_use_traverse);
}

/* p_radius_attack
 * Source is the creature that caused the explosion at spot.
 */

void p_radius_attack(mobj_t *spot, mobj_t *source, int damage)
{
  int x;
  int y;

  int xl;
  int xh;
  int yl;
  int yh;

  fixed_t dist;

  dist = (damage + MAXRADIUS) << FRACBITS;
  yh = (spot->y + dist - bmaporgy) >> MAPBLOCKSHIFT;
  yl = (spot->y - dist - bmaporgy) >> MAPBLOCKSHIFT;
  xh = (spot->x + dist - bmaporgx) >> MAPBLOCKSHIFT;
  xl = (spot->x - dist - bmaporgx) >> MAPBLOCKSHIFT;
  bombspot = spot;
  bombsource = source;
  bombdamage = damage;

  for (y = yl; y <= yh; y++)
    {
      for (x = xl; x <= xh; x++)
        {
          p_block_things_iterator(x, y, pit_radius_attack);
        }
    }
}

/* pit_change_sector */

static boolean pit_change_sector(mobj_t *thing)
{
  mobj_t *mo;

  if (p_things_height_clip(thing))
    {
      return true; /* keep checking */
    }

  /* crunch bodies to giblets */

  if (thing->health <= 0)
    {
      p_set_mobj_state(thing, S_GIBS);

      if (gameversion > exe_doom_1_2) thing->flags &= ~MF_SOLID;
      thing->height = 0;
      thing->radius = 0;

      /* keep checking */

      return true;
    }

  /* crunch dropped items */

  if (thing->flags & MF_DROPPED)
    {
      p_remove_mobj(thing);

      return true; /* keep checking */
    }

  if (!(thing->flags & MF_SHOOTABLE))
    {
      return true; /* assume it is bloody gibs or something */
    }

  nofit = true;

  if (crushchange && !(leveltime & 3))
    {
      p_damage_mobj(thing, NULL, NULL, 10);

      /* spray blood in a random direction */

      mo = p_spawn_mobj(thing->x, thing->y, thing->z + thing->height / 2,
                       MT_BLOOD);

      mo->momx = p_sub_random() << 12;
      mo->momy = p_sub_random() << 12;
    }

  /* keep checking (crush other things) */

  return true;
}

/* p_change_sector */

boolean p_change_sector(sector_t *sector, boolean crunch)
{
  int x;
  int y;

  nofit = false;
  crushchange = crunch;

  /* re-check heights for all things near the moving sector */

  for (x = sector->blockbox[BOXLEFT]; x <= sector->blockbox[BOXRIGHT]; x++)
    {
      for (y = sector->blockbox[BOXBOTTOM]; y <= sector->blockbox[BOXTOP];
           y++)
        {
          p_block_things_iterator(x, y, pit_change_sector);
        }
    }

  return nofit;
}
