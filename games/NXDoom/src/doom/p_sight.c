/****************************************************************************
 * apps/games/NXDoom/src/doom/p_sight.c
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
 *  LineOfSight/Visibility checks, uses REJECT Lookup Table.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomdef.h"
#include "doomstat.h"

#include "i_system.h"
#include "p_local.h"

/* State. */

#include "r_state.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* p_check_sight */

fixed_t sightzstart; /* eye z of looker */
fixed_t topslope;
fixed_t bottomslope; /* slopes to top and bottom of target */

divline_t strace; /* from t1 to t2 */
fixed_t t2x;
fixed_t t2y;

int sightcounts[2];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* PTR_SightTraverse() for Doom 1.2 sight calculations
 * taken from prboom-plus/src/p_sight.c:69-102
 */

static boolean ptr_sight_traverse(intercept_t *in)
{
  line_t *li;
  fixed_t slope;

  li = in->d.line;

  /* crosses a two sided line */

  p_line_opening(li);

  /* quick test for totally closed doors */

  if (openbottom >= opentop)
    {
      return false; /* stop */
    }

  if (li->frontsector->floorheight != li->backsector->floorheight)
    {
      slope = fixed_div(openbottom - sightzstart, in->frac);
      if (slope > bottomslope) bottomslope = slope;
    }

  if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
    {
      slope = fixed_div(opentop - sightzstart, in->frac);
      if (slope < topslope) topslope = slope;
    }

  if (topslope <= bottomslope) return false; /* stop */

  return true; /* keep going */
}

/* p_divline_side
 * Returns side 0 (front), 1 (back), or 2 (on).
 */

static int p_divline_side(fixed_t x, fixed_t y, divline_t *node)
{
  fixed_t dx;
  fixed_t dy;
  fixed_t left;
  fixed_t right;

  if (!node->dx)
    {
      if (x == node->x) return 2;

      if (x <= node->x) return node->dy > 0;

      return node->dy < 0;
    }

  if (!node->dy)
    {
      if (x == node->y) return 2;

      if (y <= node->y) return node->dx < 0;

      return node->dx > 0;
    }

  dx = (x - node->x);
  dy = (y - node->y);

  left = (node->dy >> FRACBITS) * (dx >> FRACBITS);
  right = (dy >> FRACBITS) * (node->dx >> FRACBITS);

  if (right < left) return 0; /* front side */

  if (left == right) return 2;
  return 1; /* back side */
}

/* p_intercept_vector2
 * Returns the fractional intercept point along the first divline.
 * This is only called by the addthings and addlines traversers.
 */

static fixed_t p_intercept_vector2(divline_t *v2, divline_t *v1)
{
  fixed_t frac;
  fixed_t num;
  fixed_t den;

  den = fixed_mul(v1->dy >> 8, v2->dx) - fixed_mul(v1->dx >> 8, v2->dy);

  if (den == 0) return 0;

  num = fixed_mul((v1->x - v2->x) >> 8, v1->dy) +
        fixed_mul((v2->y - v1->y) >> 8, v1->dx);
  frac = fixed_div(num, den);

  return frac;
}

/* p_cross_subsector
 * Returns true if strace crosses the given subsector successfully.
 */

static boolean p_cross_subsector(int num)
{
  seg_t *seg;
  line_t *line;
  int s1;
  int s2;
  int count;
  subsector_t *sub;
  sector_t *front;
  sector_t *back;
  fixed_t l_opentop;
  fixed_t l_openbottom;
  divline_t divl;
  vertex_t *v1;
  vertex_t *v2;
  fixed_t frac;
  fixed_t slope;

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (num >= numsubsectors)
    i_error("p_cross_subsector: ss %i with numss = %i", num, numsubsectors);
#endif

  sub = &subsectors[num];

  /* check lines */

  count = sub->numlines;
  seg = &segs[sub->firstline];

  for (; count; seg++, count--)
    {
      line = seg->linedef;

      /* already checked other side? */

      if (line->validcount == validcount) continue;

      line->validcount = validcount;

      v1 = line->v1;
      v2 = line->v2;
      s1 = p_divline_side(v1->x, v1->y, &strace);
      s2 = p_divline_side(v2->x, v2->y, &strace);

      /* line isn't crossed? */

      if (s1 == s2) continue;

      divl.x = v1->x;
      divl.y = v1->y;
      divl.dx = v2->x - v1->x;
      divl.dy = v2->y - v1->y;
      s1 = p_divline_side(strace.x, strace.y, &divl);
      s2 = p_divline_side(t2x, t2y, &divl);

      /* line isn't crossed? */

      if (s1 == s2) continue;

      /* Backsector may be NULL if this is an "impassable
       * glass" hack line.
       */

      if (line->backsector == NULL)
        {
          return false;
        }

      /* stop because it is not two sided anyway
       * might do this after updating validcount?
       */

      if (!(line->flags & ML_TWOSIDED)) return false;

      /* crosses a two sided line */

      front = seg->frontsector;
      back = seg->backsector;

      /* no wall to block sight with? */

      if (front->floorheight == back->floorheight &&
          front->ceilingheight == back->ceilingheight)
        continue;

      /* possible occluder
       * because of ceiling height differences
       */

      if (front->ceilingheight < back->ceilingheight)
        l_opentop = front->ceilingheight;
      else
        l_opentop = back->ceilingheight;

      /* because of ceiling height differences */

      if (front->floorheight > back->floorheight)
        l_openbottom = front->floorheight;
      else
        l_openbottom = back->floorheight;

      /* quick test for totally closed doors */

      if (l_openbottom >= l_opentop) return false; /* stop */

      frac = p_intercept_vector2(&strace, &divl);

      if (front->floorheight != back->floorheight)
        {
          slope = fixed_div(l_openbottom - sightzstart, frac);
          if (slope > bottomslope) bottomslope = slope;
        }

      if (front->ceilingheight != back->ceilingheight)
        {
          slope = fixed_div(l_opentop - sightzstart, frac);
          if (slope < topslope) topslope = slope;
        }

      if (topslope <= bottomslope) return false; /* stop */
    }

  /* passed the subsector ok */

  return true;
}

/* p_cross_bsp_node
 * Returns true if strace crosses the given node successfully.
 */

static boolean p_cross_bsp_node(int bspnum)
{
  node_t *bsp;
  int side;

  if (bspnum & NF_SUBSECTOR)
    {
      if (bspnum == -1)
        {
          return p_cross_subsector(0);
        }
      else
        {
          return p_cross_subsector(bspnum & (~NF_SUBSECTOR));
        }
    }

  bsp = &nodes[bspnum];

  /* decide which side the start point is on */

  side = p_divline_side(strace.x, strace.y, (divline_t *)bsp);
  if (side == 2) side = 0; /* an "on" should cross both sides */

  /* cross the starting side */

  if (!p_cross_bsp_node(bsp->children[side])) return false;

  /* the partition plane is crossed here */

  if (side == p_divline_side(t2x, t2y, (divline_t *)bsp))
    {
      /* the line doesn't touch the other side */

      return true;
    }

  /* cross the ending side */

  return p_cross_bsp_node(bsp->children[side ^ 1]);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* p_check_sight
 * Returns true if a straight line between t1 and t2 is unobstructed.
 * Uses REJECT.
 */

boolean p_check_sight(mobj_t *t1, mobj_t *t2)
{
  int s1;
  int s2;
  int pnum;
  int bytenum;
  int bitnum;

  /* First check for trivial rejection. */

  /* Determine subsector entries in REJECT table. */

  s1 = (t1->subsector->sector - sectors);
  s2 = (t2->subsector->sector - sectors);
  pnum = s1 * numsectors + s2;
  bytenum = pnum >> 3;
  bitnum = 1 << (pnum & 7);

  /* Check in REJECT table. */

  if (rejectmatrix[bytenum] & bitnum)
    {
      sightcounts[0]++;

      /* can't possibly be connected */

      return false;
    }

  /* An unobstructed LOS is possible.
   * Now look from eyes of t1 to any part of t2.
   */

  sightcounts[1]++;

  validcount++;

  sightzstart = t1->z + t1->height - (t1->height >> 2);
  topslope = (t2->z + t2->height) - sightzstart;
  bottomslope = (t2->z) - sightzstart;

  if (gameversion <= exe_doom_1_2)
    {
      return p_path_traverse(t1->x, t1->y, t2->x, t2->y,
                             PT_EARLYOUT | PT_ADDLINES,
                             ptr_sight_traverse);
    }

  strace.x = t1->x;
  strace.y = t1->y;
  t2x = t2->x;
  t2y = t2->y;
  strace.dx = t2->x - t1->x;
  strace.dy = t2->y - t1->y;

  /* the head node is the last node output */

  return p_cross_bsp_node(numnodes - 1);
}
