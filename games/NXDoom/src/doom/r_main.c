/****************************************************************************
 * apps/games/NXDoom/src/doom/r_main.c
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
 *  Rendering main loop and setup functions, utility functions (BSP,
 *  geometry, trigonometry). See tables.c, too.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "d_loop.h"
#include "doomdef.h"

#include "m_bbox.h"
#include "m_menu.h"

#include "r_local.h"
#include "r_sky.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Fineangles in the SCREENWIDTH wide window. */

#define FIELDOFVIEW 2048

#define DISTMAP 2

/****************************************************************************
 * Public Data
 ****************************************************************************/

int viewangleoffset;

/* increment every time a check is made */

int validcount = 1;

lighttable_t *fixedcolormap;

int centerx;
int centery;

fixed_t centerxfrac;
fixed_t centeryfrac;
fixed_t projection;

/* just for profiling purposes */

int framecount;

int sscount;
int linecount;
int loopcount;

fixed_t viewx;
fixed_t viewy;
fixed_t viewz;

angle_t viewangle;

fixed_t viewcos;
fixed_t viewsin;

player_t *viewplayer;

/* 0 = high, 1 = low */

int detailshift;

/* precalculated math tables */

angle_t clipangle;

/* The viewangletox[viewangle + FINEANGLES/4] lookup
 * maps the visible view angles to screen X coordinates,
 * flattening the arc to a flat projection plane.
 * There will be many angles mapped to the same X.
 */

int viewangletox[FINEANGLES / 2];

/* The xtoviewangleangle[] table maps a screen pixel
 * to the lowest viewangle that maps back to x ranges
 * from clipangle to -clipangle.
 */

angle_t xtoviewangle[SCREENWIDTH + 1];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

/* bumped light from gun blasts */

int extralight;

void (*colfunc)(void);
void (*basecolfunc)(void);
void (*fuzzcolfunc)(void);
void (*transcolfunc)(void);
void (*spanfunc)(void);

boolean setsizeneeded;
int setblocks;
int setdetail;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: r_add_point_to_box
 *
 * Description:
 *  Expand a given bbox so that it encloses a given point.
 *
 ****************************************************************************/

#if 0 /* UNUSED */
static void r_add_point_to_box(int x, int y, fixed_t *box)
{
  if (x < box[BOXLEFT]) box[BOXLEFT] = x;
  if (x > box[BOXRIGHT]) box[BOXRIGHT] = x;
  if (y < box[BOXBOTTOM]) box[BOXBOTTOM] = y;
  if (y > box[BOXTOP]) box[BOXTOP] = y;
}
#endif

/****************************************************************************
 * Name: r_setup_frame
 ****************************************************************************/

static void r_setup_frame(player_t *player)
{
  int i;

  viewplayer = player;
  viewx = player->mo->x;
  viewy = player->mo->y;
  viewangle = player->mo->angle + viewangleoffset;
  extralight = player->extralight;

  viewz = player->viewz;

  viewsin = finesine[viewangle >> ANGLETOFINESHIFT];
  viewcos = finecosine[viewangle >> ANGLETOFINESHIFT];

  sscount = 0;

  if (player->fixedcolormap)
    {
      fixedcolormap = colormaps + player->fixedcolormap * 256;

      walllights = scalelightfixed;

      for (i = 0; i < MAXLIGHTSCALE; i++)
        scalelightfixed[i] = fixedcolormap;
    }
  else
    fixedcolormap = 0;

  framecount++;
  validcount++;
}

/****************************************************************************
 * Name: r_init_light_tables
 *
 * Description:
 *  Only inits the zlight table, because the scalelight table changes with
 *  view size.
 *
 ****************************************************************************/

static void r_init_light_tables(void)
{
  int i;
  int j;
  int level;
  int startmap;
  int scale;

  /* Calculate the light levels to use for each level / distance combination.
   */

  for (i = 0; i < LIGHTLEVELS; i++)
    {
      startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
      for (j = 0; j < MAXLIGHTZ; j++)
        {
          scale = fixed_div((SCREENWIDTH / 2 * FRACUNIT),
                  (j + 1) << LIGHTZSHIFT);
          scale >>= LIGHTSCALESHIFT;
          level = startmap - scale / DISTMAP;

          if (level < 0) level = 0;

          if (level >= NUMCOLORMAPS) level = NUMCOLORMAPS - 1;

          zlight[i][j] = colormaps + level * 256;
        }
    }
}

/****************************************************************************
 * Name: r_init_tables
 ****************************************************************************/

static void r_init_tables(void)
{
#if 0 /* UNUSED: now getting from tables.c */
  int i;
  float a;
  float fv;
  int t;

  /* viewangle tangent table */

  for (i = 0; i < FINEANGLES / 2; i++)
    {
      a = (i - FINEANGLES / 4 + 0.5) * PI * 2 / FINEANGLES;
      fv = FRACUNIT * tan(a);
      t = fv;
      finetangent[i] = t;
    }

  /* finesine table */

  for (i = 0; i < 5 * FINEANGLES / 4; i++)
    {
      /* OPTIMIZE: mirror... */

      a = (i + 0.5) * PI * 2 / FINEANGLES;
      t = FRACUNIT * sin(a);
      finesine[i] = t;
    }
#endif
}

/****************************************************************************
 * Name: r_init_point_to_angle
 ****************************************************************************/

static void r_init_point_to_angle(void)
{
#if 0 /* UNUSED - now getting from tables.c */
  int i;
  long t;
  float f;

  /* slope (tangent) to angle lookup */

  for (i = 0; i <= SLOPERANGE; i++)
    {
      f = atan((float)i / SLOPERANGE) / (3.141592657 * 2);
      t = 0xffffffff * f;
      tantoangle[i] = t;
    }
#endif
}

/****************************************************************************
 * Name: r_init_texture_mapping
 ****************************************************************************/

static void r_init_texture_mapping(void)
{
  int i;
  int x;
  int t;
  fixed_t focallength;

  /* Use tangent table to generate viewangletox:
   *  viewangletox will give the next greatest x
   *  after the view angle.
   *
   * Calc focallength
   *  so FIELDOFVIEW angles covers SCREENWIDTH.
   */

  focallength =
      fixed_div(centerxfrac, finetangent[FINEANGLES / 4 + FIELDOFVIEW / 2]);

  for (i = 0; i < FINEANGLES / 2; i++)
    {
      if (finetangent[i] > FRACUNIT * 2)
        t = -1;
      else if (finetangent[i] < -FRACUNIT * 2)
        t = viewwidth + 1;
      else
        {
          t = fixed_mul(finetangent[i], focallength);
          t = (centerxfrac - t + FRACUNIT - 1) >> FRACBITS;

          if (t < -1)
            t = -1;
          else if (t > viewwidth + 1)
            t = viewwidth + 1;
        }

      viewangletox[i] = t;
    }

  /* Scan viewangletox[] to generate xtoviewangle[]:
   *  xtoviewangle will give the smallest view angle
   *  that maps to x.
   */

  for (x = 0; x <= viewwidth; x++)
    {
      i = 0;
      while (viewangletox[i] > x)
        i++;
      xtoviewangle[x] = (i << ANGLETOFINESHIFT) - ANG90;
    }

  /* Take out the fencepost cases from viewangletox. */

  for (i = 0; i < FINEANGLES / 2; i++)
    {
      t = fixed_mul(finetangent[i], focallength);
      t = centerx - t;

      if (viewangletox[i] == -1)
        viewangletox[i] = 0;
      else if (viewangletox[i] == viewwidth + 1)
        viewangletox[i] = viewwidth;
    }

  clipangle = xtoviewangle[0];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: r_point_on_side
 *
 * Description:
 *  Traverse BSP (sub) tree, check point against partition plane.
 *
 * Returns:
 *   Side 0 (front) or 1 (back).
 ****************************************************************************/

int r_point_on_side(fixed_t x, fixed_t y, node_t *node)
{
  fixed_t dx;
  fixed_t dy;
  fixed_t left;
  fixed_t right;

  if (!node->dx)
    {
      if (x <= node->x) return node->dy > 0;

      return node->dy < 0;
    }

  if (!node->dy)
    {
      if (y <= node->y) return node->dx < 0;

      return node->dx > 0;
    }

  dx = (x - node->x);
  dy = (y - node->y);

  /* Try to quickly decide by looking at sign bits. */

  if ((node->dy ^ node->dx ^ dx ^ dy) & 0x80000000)
    {
      if ((node->dy ^ dx) & 0x80000000)
        {
          return 1; /* (left is negative) */
        }

      return 0;
    }

  left = fixed_mul(node->dy >> FRACBITS, dx);
  right = fixed_mul(dy, node->dx >> FRACBITS);

  if (right < left)
    {
      return 0; /* front side */
    }

  return 1; /* back side */
}

/****************************************************************************
 * Name: r_point_on_seg_side
 ****************************************************************************/

int r_point_on_seg_side(fixed_t x, fixed_t y, seg_t *line)
{
  fixed_t lx;
  fixed_t ly;
  fixed_t ldx;
  fixed_t ldy;
  fixed_t dx;
  fixed_t dy;
  fixed_t left;
  fixed_t right;

  lx = line->v1->x;
  ly = line->v1->y;

  ldx = line->v2->x - lx;
  ldy = line->v2->y - ly;

  if (!ldx)
    {
      if (x <= lx) return ldy > 0;

      return ldy < 0;
    }

  if (!ldy)
    {
      if (y <= ly) return ldx < 0;

      return ldx > 0;
    }

  dx = (x - lx);
  dy = (y - ly);

  /* Try to quickly decide by looking at sign bits. */

  if ((ldy ^ ldx ^ dx ^ dy) & 0x80000000)
    {
      if ((ldy ^ dx) & 0x80000000)
        {
          return 1; /* (left is negative) */
        }

      return 0;
    }

  left = fixed_mul(ldy >> FRACBITS, dx);
  right = fixed_mul(dy, ldx >> FRACBITS);

  if (right < left)
    {
      return 0; /* front side */
    }

  return 1; /* back side */
}

/****************************************************************************
 * Name: r_point_to_angle
 *
 * Description:
 *  To get a global angle from cartesian coordinates, the coordinates are
 *  flipped until they are in the first octant of the coordinate system, then
 *  the y (<=x) is scaled and divided by x to get a tangent (slope) value
 * which is looked up in the tantoangle[] table.
 *
 ****************************************************************************/

angle_t r_point_to_angle(fixed_t x, fixed_t y)
{
  x -= viewx;
  y -= viewy;

  if ((!x) && (!y)) return 0;

  if (x >= 0)
    {
      /* x >=0 */

      if (y >= 0)
        {
          /* y>= 0 */

          if (x > y)
            {
              return tantoangle[slope_div(y, x)]; /* octant 0 */
            }
          else
            {
              return ANG90 - 1 - tantoangle[slope_div(x, y)]; /* octant 1 */
            }
        }
      else
        {
          /* y<0 */

          y = -y;

          if (x > y)
            {
              return -tantoangle[slope_div(y, x)]; /* octant 8 */
            }
          else
            {
              return ANG270 + tantoangle[slope_div(x, y)]; /* octant 7 */
            }
        }
    }
  else
    {
      /* x<0 */

      x = -x;

      if (y >= 0)
        {
          /* y>= 0 */

          if (x > y)
            {
              return ANG180 - 1 - tantoangle[slope_div(y, x)]; /* octant 3 */
            }
          else
            {
              return ANG90 + tantoangle[slope_div(x, y)]; /* octant 2 */
            }
        }
      else
        {
          y = -y; /* y<0 */

          if (x > y)
            {
              return ANG180 + tantoangle[slope_div(y, x)]; /* octant 4 */
            }
          else
            {
              return ANG270 - 1 - tantoangle[slope_div(x, y)]; /* octant 5 */
            }
        }
    }

  return 0;
}

/****************************************************************************
 * Name: r_point_to_angle2
 ****************************************************************************/

angle_t r_point_to_angle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
  viewx = x1;
  viewy = y1;

  return r_point_to_angle(x2, y2);
}

/****************************************************************************
 * Name: r_point_to_dist
 ****************************************************************************/

fixed_t r_point_to_dist(fixed_t x, fixed_t y)
{
  int angle;
  fixed_t dx;
  fixed_t dy;
  fixed_t temp;
  fixed_t dist;
  fixed_t frac;

  dx = abs(x - viewx);
  dy = abs(y - viewy);

  if (dy > dx)
    {
      temp = dx;
      dx = dy;
      dy = temp;
    }

  /* Fix crashes in udm1.wad */

  if (dx != 0)
    {
      frac = fixed_div(dy, dx);
    }
  else
    {
      frac = 0;
    }

  angle = (tantoangle[frac >> DBITS] + ANG90) >> ANGLETOFINESHIFT;

  dist = fixed_div(dx, finesine[angle]); /* use as cosine */

  return dist;
}

/****************************************************************************
 * Name: r_scale_from_global_angle
 *
 * Description:
 *  Returns the texture mapping scale for the current line (horizontal span)
 *  at the given angle. rw_distance must be calculated first.
 *
 ****************************************************************************/

fixed_t r_scale_from_global_angle(angle_t visangle)
{
  fixed_t scale;
  angle_t anglea;
  angle_t angleb;
  int sinea;
  int sineb;
  fixed_t num;
  int den;

#if 0 /* UNUSED */
  fixed_t dist;
  fixed_t z;
  fixed_t sinv;
  fixed_t cosv;

  sinv = finesine[(visangle - rw_normalangle) >> ANGLETOFINESHIFT];
  dist = fixed_div(rw_distance, sinv);
  cosv = finecosine[(viewangle - visangle) >> ANGLETOFINESHIFT];
  z = abs(fixed_mul(dist, cosv));
  scale = fixed_div(projection, z);
  return scale;
#endif

  anglea = ANG90 + (visangle - viewangle);
  angleb = ANG90 + (visangle - rw_normalangle);

  /* both sines are always positive */

  sinea = finesine[anglea >> ANGLETOFINESHIFT];
  sineb = finesine[angleb >> ANGLETOFINESHIFT];
  num = fixed_mul(projection, sineb) << detailshift;
  den = fixed_mul(rw_distance, sinea);

  if (den > num >> FRACBITS)
    {
      scale = fixed_div(num, den);

      if (scale > 64 * FRACUNIT)
        scale = 64 * FRACUNIT;
      else if (scale < 256)
        scale = 256;
    }
  else
    scale = 64 * FRACUNIT;

  return scale;
}

/****************************************************************************
 * Name: r_set_view_size
 *
 * Description:
 *  Do not really change anything here, because it might be in the middle
 *  of a refresh. The change will take effect next refresh.
 *
 ****************************************************************************/

void r_set_view_size(int blocks, int detail)
{
  /* screenblocks is only ever meant to hold 3..11 (set that way by the
   * options menu and by the config default of 9).  The renderer's view
   * geometry math divides by values derived from it - notably
   * pspriteiscale = FRACUNIT * SCREENWIDTH / viewwidth in
   * r_execute_set_view_size() - so a 0 or otherwise out-of-range value
   * turns into a divide-by-zero hardware exception (EXCCAUSE=6), which on
   * this flat-memory build takes the whole board down rather than just
   * this task.  Clamp defensively so a bad/missing config value degrades
   * to the default screen size instead of a system crash.
   */

  if (blocks < 3 || blocks > 11)
    {
      printf("r_set_view_size: screenblocks=%d out of range, using 10\n",
            blocks);
      blocks = 10;
    }

  setsizeneeded = true;
  setblocks = blocks;
  setdetail = detail;
}

/****************************************************************************
 * Name: r_execute_set_view_size
 ****************************************************************************/

void r_execute_set_view_size(void)
{
  fixed_t cosadj;
  fixed_t dy;
  int i;
  int j;
  int level;
  int startmap;

  setsizeneeded = false;

  if (setblocks == 11)
    {
      scaledviewwidth = SCREENWIDTH;
      viewheight = SCREENHEIGHT;
    }
  else
    {
      scaledviewwidth = setblocks * 32;
      viewheight = (setblocks * 168 / 10) & ~7;
    }

  detailshift = setdetail;
  viewwidth = scaledviewwidth >> detailshift;

  centery = viewheight / 2;
  centerx = viewwidth / 2;
  centerxfrac = centerx << FRACBITS;
  centeryfrac = centery << FRACBITS;
  projection = centerxfrac;

  if (!detailshift)
    {
      colfunc = basecolfunc = r_draw_column;
      fuzzcolfunc = r_draw_fuzz_column;
      transcolfunc = r_draw_translated_column;
      spanfunc = r_draw_span;
    }
  else
    {
      colfunc = basecolfunc = r_draw_column_low;
      fuzzcolfunc = r_draw_fuzz_column_low;
      transcolfunc = r_draw_translated_column_low;
      spanfunc = r_draw_span_low;
    }

  r_init_buffer(scaledviewwidth, viewheight);

  r_init_texture_mapping();

  /* psprite scales */

  pspritescale = FRACUNIT * viewwidth / SCREENWIDTH;
  pspriteiscale = FRACUNIT * SCREENWIDTH / viewwidth;

  /* thing clipping */

  for (i = 0; i < viewwidth; i++)
    screenheightarray[i] = viewheight;

  /* planes */

  for (i = 0; i < viewheight; i++)
    {
      dy = ((i - viewheight / 2) << FRACBITS) + FRACUNIT / 2;
      dy = abs(dy);
      yslope[i] = fixed_div((viewwidth << detailshift) / 2 * FRACUNIT, dy);
    }

  for (i = 0; i < viewwidth; i++)
    {
      cosadj = abs(finecosine[xtoviewangle[i] >> ANGLETOFINESHIFT]);
      distscale[i] = fixed_div(FRACUNIT, cosadj);
    }

  /* Calculate the light levels to use for each level / scale combination. */

  for (i = 0; i < LIGHTLEVELS; i++)
    {
      startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
      for (j = 0; j < MAXLIGHTSCALE; j++)
        {
          level = startmap -
                  j * SCREENWIDTH / (viewwidth << detailshift) / DISTMAP;

          if (level < 0) level = 0;

          if (level >= NUMCOLORMAPS) level = NUMCOLORMAPS - 1;

          scalelight[i][j] = colormaps + level * 256;
        }
    }
}

/****************************************************************************
 * Name: r_init
 ****************************************************************************/

void r_init(void)
{
  r_init_data();
  printf(".");
  r_init_point_to_angle();
  printf(".");
  r_init_tables();

  /* viewwidth / viewheight / g_detail_level are set by the defaults */

  printf(".");

  r_set_view_size(screenblocks, g_detail_level);
  r_init_planes();
  printf(".");
  r_init_light_tables();
  printf(".");
  r_init_sky_map();
  r_init_translation_table();
  printf(".");

  framecount = 0;
}

/****************************************************************************
 * Name: r_point_in_subsector
 ****************************************************************************/

subsector_t *r_point_in_subsector(fixed_t x, fixed_t y)
{
  node_t *node;
  int side;
  int nodenum;

  /* single subsector is a special case */

  if (!numnodes) return subsectors;

  nodenum = numnodes - 1;

  while (!(nodenum & NF_SUBSECTOR))
    {
      node = &nodes[nodenum];
      side = r_point_on_side(x, y, node);
      nodenum = node->children[side];
    }

  return &subsectors[nodenum & ~NF_SUBSECTOR];
}

/****************************************************************************
 * Name: r_render_player_view
 ****************************************************************************/

void r_render_player_view(player_t *player)
{
  r_setup_frame(player);

  /* Clear buffers. */

  r_clear_clip_segs();
  r_clear_draw_segs();
  r_clear_planes();
  r_clear_sprites();

  /* check for new console commands. */

  net_update();

  /* The head node is the last node output. */

  r_render_bsp_node(numnodes - 1);

  /* Check for new console commands. */

  net_update();

  r_draw_planes();

  /* Check for new console commands. */

  net_update();

  r_draw_masked();

  /* Check for new console commands. */

  net_update();
}
