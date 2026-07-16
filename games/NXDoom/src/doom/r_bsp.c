/****************************************************************************
 * apps/games/NXDoom/src/doom/r_bsp.c
 *
 * SPDX-License-Identifier: GPLv2
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
 *  BSP traversal, handling of LineSegs for rendering.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomdef.h"

#include "m_bbox.h"

#include "i_system.h"

#include "r_main.h"
#include "r_plane.h"
#include "r_things.h"

#include "doomstat.h"
#include "r_state.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* We must expand MAXSEGS to the theoretical limit of the number of solidsegs
 * that can be generated in a scene by the DOOM engine. This was determined
 * by Lee Killough during BOOM development to be a function of the
 * screensize. The simplest thing we can do, other than fix this bug, is to
 * let the game render overage and then bomb out by detecting the overflow
 * after the fact. -haleyjd
 */

/* #define MAXSEGS 32 */

#define MAXSEGS (SCREENWIDTH / 2 + 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* ClipWallSegment
 * Clips the given range of columns and includes it in the new clip list.
 */

typedef struct
{
  int first;
  int last;
} cliprange_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

seg_t *curline;
side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

#ifdef CONFIG_GAMES_NXDOOM_HEAP_BUFFERS
drawseg_t *drawsegs;
#else
drawseg_t drawsegs[CONFIG_GAMES_NXDOOM_MAXDRAWSEGS];
#endif
drawseg_t *ds_p;

/* newend is one past the last valid seg */

cliprange_t *newend;
cliprange_t solidsegs[MAXSEGS];

int checkcoord[12][4] =
{
    {
        3,
        0,
        2,
        1,
    },
    {
        3,
        0,
        2,
        0,
    },
    {
        3,
        1,
        2,
        0,
    },
    {
        0,
    },
    {
        2,
        0,
        2,
        1,
    },
    {
        0,
        0,
        0,
        0,
    },
    {
        3,
        1,
        3,
        0,
    },
    {
        0,
    },
    {
        2,
        0,
        3,
        1,
    },
    {
        2,
        1,
        3,
        1,
    },
    {
        2,
        1,
        3,
        0,
    },
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void r_store_wall_range(int start, int stop);
void r_clip_solid_wall_segment(int first, int last);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: r_clip_pass_wall_segment
 *
 * Description:
 *  Clips the given range of columns,
 *   but does not includes it in the clip list.
 *  Does handle windows,
 *   e.g. LineDefs with upper and lower texture.
 *
 ****************************************************************************/

static void r_clip_pass_wall_segment(int first, int last)
{
  cliprange_t *start;

  /* Find the first range that touches the range
   *  (adjacent pixels are touching).
   */

  start = solidsegs;
  while (start->last < first - 1)
    start++;

  if (first < start->first)
    {
      if (last < start->first - 1)
        {
          /* Post is entirely visible (above start). */

          r_store_wall_range(first, last);
          return;
        }

      /* There is a fragment above *start. */

      r_store_wall_range(first, start->first - 1);
    }

  /* Bottom contained in start? */

  if (last <= start->last) return;

  while (last >= (start + 1)->first - 1)
    {
      /* There is a fragment between two posts. */

      r_store_wall_range(start->last + 1, (start + 1)->first - 1);
      start++;

      if (last <= start->last) return;
    }

  /* There is a fragment after *next. */

  r_store_wall_range(start->last + 1, last);
}

/* r_check_b_box
 * Checks BSP node/subtree bounding box.
 * Returns true if some part of the bbox might be visible.
 */

static boolean r_check_b_box(fixed_t *bspcoord)
{
  int boxx;
  int boxy;
  int boxpos;

  fixed_t x1;
  fixed_t y1;
  fixed_t x2;
  fixed_t y2;

  angle_t angle1;
  angle_t angle2;
  angle_t span;
  angle_t tspan;

  cliprange_t *start;

  int sx1;
  int sx2;

  /* Find the corners of the box
   * that define the edges from current viewpoint.
   */

  if (viewx <= bspcoord[BOXLEFT])
    boxx = 0;
  else if (viewx < bspcoord[BOXRIGHT])
    boxx = 1;
  else
    boxx = 2;

  if (viewy >= bspcoord[BOXTOP])
    boxy = 0;
  else if (viewy > bspcoord[BOXBOTTOM])
    boxy = 1;
  else
    boxy = 2;

  boxpos = (boxy << 2) + boxx;
  if (boxpos == 5) return true;

  x1 = bspcoord[checkcoord[boxpos][0]];
  y1 = bspcoord[checkcoord[boxpos][1]];
  x2 = bspcoord[checkcoord[boxpos][2]];
  y2 = bspcoord[checkcoord[boxpos][3]];

  /* check clip list for an open space */

  angle1 = r_point_to_angle(x1, y1) - viewangle;
  angle2 = r_point_to_angle(x2, y2) - viewangle;

  span = angle1 - angle2;

  /* Sitting on a line? */

  if (span >= ANG180) return true;

  tspan = angle1 + clipangle;

  if (tspan > 2 * clipangle)
    {
      tspan -= 2 * clipangle;

      /* Totally off the left edge? */

      if (tspan >= span) return false;

      angle1 = clipangle;
    }

  tspan = clipangle - angle2;
  if (tspan > 2 * clipangle)
    {
      tspan -= 2 * clipangle;

      /* Totally off the left edge? */

      if (tspan >= span) return false;

      angle2 = -clipangle;
    }

  /* Find the first clippost
   *  that touches the source post
   *  (adjacent pixels are touching).
   */

  angle1 = (angle1 + ANG90) >> ANGLETOFINESHIFT;
  angle2 = (angle2 + ANG90) >> ANGLETOFINESHIFT;
  sx1 = viewangletox[angle1];
  sx2 = viewangletox[angle2];

  /* Does not cross a pixel. */

  if (sx1 == sx2) return false;
  sx2--;

  start = solidsegs;
  while (start->last < sx2)
    start++;

  if (sx1 >= start->first && sx2 <= start->last)
    {
      /* The clippost contains the new span. */

      return false;
    }

  return true;
}

/****************************************************************************
 * Name: r_add_line
 *
 * Description:
 *  Clips the given segment
 *  and adds any visible pieces to the line list.
 *
 ****************************************************************************/

static void r_add_line(seg_t *line)
{
  int x1;
  int x2;
  angle_t angle1;
  angle_t angle2;
  angle_t span;
  angle_t tspan;

  curline = line;

  /* OPTIMIZE: quickly reject orthogonal back sides. */

  angle1 = r_point_to_angle(line->v1->x, line->v1->y);
  angle2 = r_point_to_angle(line->v2->x, line->v2->y);

  /* Clip to view edges.
   * OPTIMIZE: make constant out of 2*clipangle (FIELDOFVIEW).
   */

  span = angle1 - angle2;

  /* Back side? I.e. backface culling? */

  if (span >= ANG180) return;

  /* Global angle needed by segcalc. */

  rw_angle1 = angle1;
  angle1 -= viewangle;
  angle2 -= viewangle;

  tspan = angle1 + clipangle;
  if (tspan > 2 * clipangle)
    {
      tspan -= 2 * clipangle;

      /* Totally off the left edge? */

      if (tspan >= span) return;

      angle1 = clipangle;
    }

  tspan = clipangle - angle2;
  if (tspan > 2 * clipangle)
    {
      tspan -= 2 * clipangle;

      /* Totally off the left edge? */

      if (tspan >= span) return;
      angle2 = -clipangle;
    }

  /* The seg is in the view range, but not necessarily visible. */

  angle1 = (angle1 + ANG90) >> ANGLETOFINESHIFT;
  angle2 = (angle2 + ANG90) >> ANGLETOFINESHIFT;
  x1 = viewangletox[angle1];
  x2 = viewangletox[angle2];

  /* Does not cross a pixel? */

  if (x1 == x2) return;

  backsector = line->backsector;

  /* Single sided line? */

  if (!backsector) goto clipsolid;

  /* Closed door. */

  if (backsector->ceilingheight <= frontsector->floorheight ||
      backsector->floorheight >= frontsector->ceilingheight)
    {
      goto clipsolid;
    }

  /* Window. */

  if (backsector->ceilingheight != frontsector->ceilingheight ||
      backsector->floorheight != frontsector->floorheight)
    {
      goto clippass;
    }

  /* Reject empty lines used for triggers
   *  and special events.
   * Identical floor and ceiling on both sides,
   * identical light levels on both sides,
   * and no middle texture.
   */

  if (backsector->ceilingpic == frontsector->ceilingpic &&
      backsector->floorpic == frontsector->floorpic &&
      backsector->lightlevel == frontsector->lightlevel &&
      curline->sidedef->midtexture == 0)
    {
      return;
    }

clippass:
  r_clip_pass_wall_segment(x1, x2 - 1);
  return;

clipsolid:
  r_clip_solid_wall_segment(x1, x2 - 1);
}

/* r_subsector
 * Determine floor/ceiling planes.
 * Add sprites of things in sector.
 * Draw one or more line segments.
 */

static void r_subsector(int num)
{
  int count;
  seg_t *line;
  subsector_t *sub;

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (num >= numsubsectors)
    i_error("R_Subsector: ss %i with numss = %i", num, numsubsectors);
#endif

  sscount++;
  sub = &subsectors[num];
  frontsector = sub->sector;
  count = sub->numlines;
  line = &segs[sub->firstline];

  if (frontsector->floorheight < viewz)
    {
      floorplane =
          r_find_plane(frontsector->floorheight, frontsector->floorpic,
                       frontsector->lightlevel);
    }
  else
    floorplane = NULL;

  if (frontsector->ceilingheight > viewz ||
      frontsector->ceilingpic == skyflatnum)
    {
      ceilingplane =
          r_find_plane(frontsector->ceilingheight, frontsector->ceilingpic,
                       frontsector->lightlevel);
    }
  else
    ceilingplane = NULL;

  r_add_sprites(frontsector);

  while (count--)
    {
      r_add_line(line);
      line++;
    }

  /* check for solidsegs overflow - extremely unsatisfactory! */

  if (newend > &solidsegs[32])
    {
      i_error("R_Subsector: solidsegs overflow (vanilla may crash here)\n");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: r_clear_draw_segs
 ****************************************************************************/

void r_clear_draw_segs(void)
{
  ds_p = drawsegs;
}

/****************************************************************************
 * Name: r_clip_solid_wall_segment
 *
 * Description:
 *  Does handle solid walls, e.g. single sided LineDefs (middle texture)
 *  that entirely block the view.
 *
 ****************************************************************************/

void r_clip_solid_wall_segment(int first, int last)
{
  cliprange_t *next;
  cliprange_t *start;

  /* Find the first range that touches the range
   * (adjacent pixels are touching).
   */

  start = solidsegs;
  while (start->last < first - 1)
    start++;

  if (first < start->first)
    {
      if (last < start->first - 1)
        {
          /* Post is entirely visible (above start),
           * so insert a new clippost.
           */

          r_store_wall_range(first, last);
          next = newend;
          newend++;

          while (next != start)
            {
              *next = *(next - 1);
              next--;
            }

          next->first = first;
          next->last = last;
          return;
        }

      /* There is a fragment above *start. */

      r_store_wall_range(first, start->first - 1);

      /* Now adjust the clip size. */

      start->first = first;
    }

  /* Bottom contained in start? */

  if (last <= start->last) return;

  next = start;
  while (last >= (next + 1)->first - 1)
    {
      /* There is a fragment between two posts. */

      r_store_wall_range(next->last + 1, (next + 1)->first - 1);
      next++;

      if (last <= next->last)
        {
          /* Bottom is contained in next.
           * Adjust the clip size.
           */

          start->last = next->last;
          goto crunch;
        }
    }

  /* There is a fragment after *next. */

  r_store_wall_range(next->last + 1, last);

  /* Adjust the clip size. */

  start->last = last;

  /* Remove start+1 to next from the clip list,
   * because start now covers their area.
   */

crunch:
  if (next == start)
    {
      /* Post just extended past the bottom of one post. */

      return;
    }

  while (next++ != newend)
    {
      /* Remove a post. */

      *++start = *next;
    }

  newend = start + 1;
}

/****************************************************************************
 * Name: r_clear_clip_segs
 ****************************************************************************/

void r_clear_clip_segs(void)
{
  solidsegs[0].first = -0x7fffffff;
  solidsegs[0].last = -1;
  solidsegs[1].first = viewwidth;
  solidsegs[1].last = 0x7fffffff;
  newend = solidsegs + 2;
}

/* RenderBSPNode
 * Renders all subsectors below a given node, traversing subtree recursively.
 * Just call with BSP root.
 */

void r_render_bsp_node(int bspnum)
{
  node_t *bsp;
  int side;

  /* Found a subsector? */

  if (bspnum & NF_SUBSECTOR)
    {
      if (bspnum == -1)
        r_subsector(0);
      else
        r_subsector(bspnum & (~NF_SUBSECTOR));
      return;
    }

  bsp = &nodes[bspnum];

  /* Decide which side the view point is on. */

  side = r_point_on_side(viewx, viewy, bsp);

  /* Recursively divide front space. */

  r_render_bsp_node(bsp->children[side]);

  /* Possibly divide back space. */

  if (r_check_b_box(bsp->bbox[side ^ 1]))
    r_render_bsp_node(bsp->children[side ^ 1]);
}
