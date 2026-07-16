/****************************************************************************
 * apps/games/NXDoom/src/doom/r_plane.c
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
 *  Here is a core component: drawing the floors and ceilings, while
 *  maintaining a per column clipping list only. Moreover, the sky areas
 *  have to be determined.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXOPENINGS (SCREENWIDTH * CONFIG_GAMES_NXDOOM_MAXOPENINGS)

/****************************************************************************
 * Public Data
 ****************************************************************************/

planefunction_t floorfunc;
planefunction_t ceilingfunc;

/* opening */

/* Here comes the obnoxious "visplane". */

#ifdef CONFIG_GAMES_NXDOOM_HEAP_BUFFERS
visplane_t *visplanes;
short *openings;
#else
visplane_t visplanes[CONFIG_GAMES_NXDOOM_MAXVISPLANES];
short openings[MAXOPENINGS];
#endif
visplane_t *lastvisplane;
visplane_t *floorplane;
visplane_t *ceilingplane;

short *lastopening;

/* Clip values are the solid pixel bounding the range. floorclip starts out
 * SCREENHEIGHT ceilingclip starts out -1
 */

short floorclip[SCREENWIDTH];
short ceilingclip[SCREENWIDTH];

/* spanstart holds the start of a plane span initialized to 0 at start */

int spanstart[SCREENHEIGHT];
int spanstop[SCREENHEIGHT];

/* texture mapping */

lighttable_t **planezlight;
fixed_t planeheight;

fixed_t yslope[SCREENHEIGHT];
fixed_t distscale[SCREENWIDTH];
fixed_t basexscale;
fixed_t baseyscale;

fixed_t cachedheight[SCREENHEIGHT];
fixed_t cacheddistance[SCREENHEIGHT];
fixed_t cachedxstep[SCREENHEIGHT];
fixed_t cachedystep[SCREENHEIGHT];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Uses global vars:
 *  planeheight
 *  ds_source
 *  basexscale
 *  baseyscale
 *  viewx
 *  viewy
 *
 * BASIC PRIMITIVE
 */

static void r_map_plane(int y, int x1, int x2)
{
  angle_t angle;
  fixed_t distance;
  fixed_t length;
  unsigned index;

  /* y indexes cachedheight[]/cacheddistance[]/cachedxstep[]/cachedystep[]
   * below, all sized SCREENHEIGHT - a y outside that range (observed on
   * this port: y=255 against a 200-entry array, well past even
   * viewheight) is an out-of-bounds array write, not just a "debug
   * assertion".  This used to be gated behind CONFIG_GAMES_NXDOOM_
   * RANGECHECK and fatal (i_error(), which tears down the whole process
   * on what vanilla Doom would just render as one glitched span) - both
   * wrong: the memory-safety check must not be optional, and killing the
   * entire game over one bad plane span is worse than just not drawing
   * it.  Clamp y into range instead of touching memory outside the
   * buffers' real bounds - this still renders the span (as one glitched
   * row, the same "wrong but visible" failure mode vanilla DOOM has) so
   * a bad plane doesn't leave a blank gap on screen either.
   */

  if (x2 < x1 || x1 < 0 || x2 >= viewwidth)
    {
      return;
    }

  if (y < 0)
    {
      y = 0;
    }
  else if (y >= SCREENHEIGHT)
    {
      y = SCREENHEIGHT - 1;
    }

  if (planeheight != cachedheight[y])
    {
      cachedheight[y] = planeheight;
      distance = cacheddistance[y] = fixed_mul(planeheight, yslope[y]);
      ds_xstep = cachedxstep[y] = fixed_mul(distance, basexscale);
      ds_ystep = cachedystep[y] = fixed_mul(distance, baseyscale);
    }
  else
    {
      distance = cacheddistance[y];
      ds_xstep = cachedxstep[y];
      ds_ystep = cachedystep[y];
    }

  length = fixed_mul(distance, distscale[x1]);
  angle = (viewangle + xtoviewangle[x1]) >> ANGLETOFINESHIFT;
  ds_xfrac = viewx + fixed_mul(finecosine[angle], length);
  ds_yfrac = -viewy - fixed_mul(finesine[angle], length);

  if (fixedcolormap)
    ds_colormap = fixedcolormap;
  else
    {
      index = distance >> LIGHTZSHIFT;

      if (index >= MAXLIGHTZ) index = MAXLIGHTZ - 1;

      ds_colormap = planezlight[index];
    }

  ds_y = y;
  ds_x1 = x1;
  ds_x2 = x2;

  /* high or low detail */

  spanfunc();
}

static void r_make_spans(int x, int t1, int b1, int t2, int b2)
{
  while (t1 < t2 && t1 <= b1)
    {
      r_map_plane(t1, spanstart[t1], x - 1);
      t1++;
    }
  while (b1 > b2 && b1 >= t1)
    {
      r_map_plane(b1, spanstart[b1], x - 1);
      b1--;
    }

  while (t2 < t1 && t2 <= b2)
    {
      spanstart[t2] = x;
      t2++;
    }
  while (b2 > b1 && b2 >= t2)
    {
      spanstart[b2] = x;
      b2--;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* r_init_planes
 * Only at game startup.
 */

void r_init_planes(void)
{
#ifdef CONFIG_GAMES_NXDOOM_HEAP_BUFFERS
  /* These renderer scratch buffers are sized for a comfortable margin
   * above vanilla DOOM's original limits and, on a DRAM-constrained
   * target, blow the internal DRAM budget as static arrays - opt-in
   * heap allocation instead (comes out of the PSRAM-backed user heap
   * on this target) via CONFIG_GAMES_NXDOOM_HEAP_BUFFERS.
   */

  visplanes = malloc(sizeof(visplane_t) * CONFIG_GAMES_NXDOOM_MAXVISPLANES);
  openings = malloc(sizeof(short) * MAXOPENINGS);
  drawsegs = malloc(sizeof(drawseg_t) * CONFIG_GAMES_NXDOOM_MAXDRAWSEGS);
  vissprites = malloc(sizeof(vissprite_t) *
                       CONFIG_GAMES_NXDOOM_MAXVISSPRITES);

  if (visplanes == NULL || openings == NULL || drawsegs == NULL ||
      vissprites == NULL)
    {
      i_error("r_init_planes: failed to allocate renderer buffers");
    }

  /* i_quit() can be followed by another r_init_planes() call within the
   * same boot (relaunching the game via nxpkg on this flat, single
   * address-space build), so these heap buffers must be freed on exit
   * or every relaunch leaks the previous allocation permanently.
   */

  i_at_exit(r_shutdown_planes, true);
#endif
}

/* r_shutdown_planes
 * Frees the renderer scratch buffers allocated by r_init_planes.  Only
 * registered as an exit handler when CONFIG_GAMES_NXDOOM_HEAP_BUFFERS is
 * set - the static-array buffers have nothing to free.
 */

void r_shutdown_planes(void)
{
#ifdef CONFIG_GAMES_NXDOOM_HEAP_BUFFERS
  free(visplanes);
  free(openings);
  free(drawsegs);
  free(vissprites);

  visplanes = NULL;
  openings = NULL;
  drawsegs = NULL;
  vissprites = NULL;
#endif
}

/* r_clear_planes
 * At beginning of frame.
 */

void r_clear_planes(void)
{
  int i;
  angle_t angle;

  /* opening / clipping determination */

  for (i = 0; i < viewwidth; i++)
    {
      floorclip[i] = viewheight;
      ceilingclip[i] = -1;
    }

  lastvisplane = visplanes;
  lastopening = openings;

  /* texture calculation */

  memset(cachedheight, 0, sizeof(cachedheight));

  /* left to right mapping */

  angle = (viewangle - ANG90) >> ANGLETOFINESHIFT;

  /* scale will be unit scale at SCREENWIDTH/2 distance */

  basexscale = fixed_div(finecosine[angle], centerxfrac);
  baseyscale = -fixed_div(finesine[angle], centerxfrac);
}

visplane_t *r_find_plane(fixed_t height, int picnum, int lightlevel)
{
  visplane_t *check;

  if (picnum == skyflatnum)
    {
      height = 0; /* all skys map together */
      lightlevel = 0;
    }

  for (check = visplanes; check < lastvisplane; check++)
    {
      if (height == check->height && picnum == check->picnum &&
          lightlevel == check->lightlevel)
        {
          break;
        }
    }

  if (check < lastvisplane) return check;

  if (lastvisplane - visplanes == CONFIG_GAMES_NXDOOM_MAXVISPLANES)
    i_error("r_find_plane: no more visplanes");

  lastvisplane++;

  check->height = height;
  check->picnum = picnum;
  check->lightlevel = lightlevel;
  check->minx = SCREENWIDTH;
  check->maxx = -1;

  memset(check->top, 0xff, sizeof(check->top));

  return check;
}

visplane_t *r_check_plane(visplane_t *pl, int start, int stop)
{
  int intrl;
  int intrh;
  int unionl;
  int unionh;
  int x;

  if (start < pl->minx)
    {
      intrl = pl->minx;
      unionl = start;
    }
  else
    {
      unionl = pl->minx;
      intrl = start;
    }

  if (stop > pl->maxx)
    {
      intrh = pl->maxx;
      unionh = stop;
    }
  else
    {
      unionh = pl->maxx;
      intrh = stop;
    }

  for (x = intrl; x <= intrh; x++)
    {
      if (pl->top[x] != 0xff) break;
    }

  if (x > intrh)
    {
      pl->minx = unionl;
      pl->maxx = unionh;

      return pl; /* use the same one */
    }

  /* make a new visplane */

  lastvisplane->height = pl->height;
  lastvisplane->picnum = pl->picnum;
  lastvisplane->lightlevel = pl->lightlevel;

  if (lastvisplane - visplanes == CONFIG_GAMES_NXDOOM_MAXVISPLANES)
    i_error("r_check_plane: no more visplanes");

  pl = lastvisplane++;
  pl->minx = start;
  pl->maxx = stop;

  memset(pl->top, 0xff, sizeof(pl->top));

  return pl;
}

/* At the end of each frame. */

void r_draw_planes(void)
{
  visplane_t *pl;
  int light;
  int x;
  int stop;
  int angle;
  int lumpnum;

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if (ds_p - drawsegs > CONFIG_GAMES_NXDOOM_MAXDRAWSEGS)
    i_error("r_draw_planes: drawsegs overflow (%td)", ds_p - drawsegs);

  if (lastvisplane - visplanes > CONFIG_GAMES_NXDOOM_MAXVISPLANES)
    i_error("r_draw_planes: visplane overflow (%td)",
            lastvisplane - visplanes);

  if (lastopening - openings > MAXOPENINGS)
    i_error("r_draw_planes: opening overflow (%td)", lastopening - openings);
#endif

  for (pl = visplanes; pl < lastvisplane; pl++)
    {
      if (pl->minx > pl->maxx) continue;

      /* sky flat */

      if (pl->picnum == skyflatnum)
        {
          dc_iscale = pspriteiscale >> detailshift;

          /* Sky is always drawn full bright, i.e. colormaps[0] is used.
           * Because of this hack, sky is not affected by INVUL inverse
           * mapping.
           */

          dc_colormap = colormaps;
          dc_texturemid = skytexturemid;
          for (x = pl->minx; x <= pl->maxx; x++)
            {
              dc_yl = pl->top[x];
              dc_yh = pl->bottom[x];

              if (dc_yl <= dc_yh)
                {
                  angle = (viewangle + xtoviewangle[x]) >> ANGLETOSKYSHIFT;
                  dc_x = x;
                  dc_source = r_get_column(skytexture, angle);
                  colfunc();
                }
            }

          continue;
        }

      /* regular flat */

      lumpnum = firstflat + flattranslation[pl->picnum];
      ds_source = w_cache_lump_num(lumpnum, PU_STATIC);

      planeheight = abs(pl->height - viewz);
      light = (pl->lightlevel >> LIGHTSEGSHIFT) + extralight;

      if (light >= LIGHTLEVELS) light = LIGHTLEVELS - 1;

      if (light < 0) light = 0;

      planezlight = zlight[light];

      pl->top[pl->maxx + 1] = 0xff;
      pl->top[pl->minx - 1] = 0xff;

      stop = pl->maxx + 1;

      for (x = pl->minx; x <= stop; x++)
        {
          r_make_spans(x, pl->top[x - 1], pl->bottom[x - 1], pl->top[x],
                       pl->bottom[x]);
        }

      w_release_lump_num(lumpnum);
    }
}
