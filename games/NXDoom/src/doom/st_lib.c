/****************************************************************************
 * apps/games/NXDoom/src/doom/st_lib.c
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
 *  The status bar widget code.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>

#include "deh_main.h"
#include "doomdef.h"

#include "v_video.h"
#include "z_zone.h"

#include "i_swap.h"
#include "i_system.h"

#include "w_wad.h"

#include "r_local.h"
#include "st_lib.h"
#include "st_stuff.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Hack display negative frags. Loads and store the stminus lump. */

patch_t *sttminus;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stlib_draw_num
 *
 * Description:
 *  A fairly efficient way to draw a number based on differences from the old
 *  number. Note: worth the trouble?
 *
 ****************************************************************************/

static void stlib_draw_num(st_number_t *n, boolean refresh)
{
  int numdigits = n->width;
  int num = *n->num;

  int w = SHORT(n->p[0]->width);
  int h = SHORT(n->p[0]->height);
  int x = n->x;

  int neg;

  n->oldnum = *n->num;

  neg = num < 0;

  if (neg)
    {
      if (numdigits == 2 && num < -9)
        num = -9;
      else if (numdigits == 3 && num < -99)
        num = -99;

      num = -num;
    }

  /* clear the area */

  x = n->x - numdigits * w;

  if (n->y - ST_Y < 0) i_error("drawNum: n->y - ST_Y < 0");

  v_copy_rect(x, n->y - ST_Y, st_backing_screen, w * numdigits, h, x, n->y);

  if (num == 1994) return; /* if non-number, do not draw it */

  x = n->x;

  /* in the special case of 0, you draw 0 */

  if (!num) v_draw_patch(x - w, n->y, n->p[0]);

  /* draw the new number */

  while (num && numdigits--)
    {
      x -= w;
      v_draw_patch(x, n->y, n->p[num % 10]);
      num /= 10;
    }

  /* draw a minus sign if necessary */

  if (neg && sttminus) v_draw_patch(x - 8, n->y, sttminus);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stlib_init(void)
{
  if (w_check_num_for_name(("STTMINUS")) >= 0)
    sttminus = (patch_t *)w_cache_lump_name(("STTMINUS"), PU_STATIC);
  else
    sttminus = NULL;
}

/* ? */

void stlib_init_num(st_number_t *n, int x, int y, patch_t **pl, int *num,
                    boolean *on, int width)
{
  n->x = x;
  n->y = y;
  n->oldnum = 0;
  n->width = width;
  n->num = num;
  n->on = on;
  n->p = pl;
}

void stlib_update_num(st_number_t *n, boolean refresh)
{
  if (*n->on) stlib_draw_num(n, refresh);
}

void stlib_init_percent(st_percent_t *p, int x, int y, patch_t **pl,
        int *num, boolean *on, patch_t *percent)
{
  stlib_init_num(&p->n, x, y, pl, num, on, 3);
  p->p = percent;
}

void stlib_update_percent(st_percent_t *per, int refresh)
{
  if (refresh && *per->n.on) v_draw_patch(per->n.x, per->n.y, per->p);

  stlib_update_num(&per->n, refresh);
}

void stlib_init_mutl_icon(st_multicon_t *i, int x, int y, patch_t **il,
                          int *inum, boolean *on)
{
  i->x = x;
  i->y = y;
  i->oldinum = -1;
  i->inum = inum;
  i->on = on;
  i->p = il;
}

void stlib_update_mult_icon(st_multicon_t *mi, boolean refresh)
{
  int w;
  int h;
  int x;
  int y;

  if (*mi->on && (mi->oldinum != *mi->inum || refresh) && (*mi->inum != -1))
    {
      if (mi->oldinum != -1)
        {
          x = mi->x - SHORT(mi->p[mi->oldinum]->leftoffset);
          y = mi->y - SHORT(mi->p[mi->oldinum]->topoffset);
          w = SHORT(mi->p[mi->oldinum]->width);
          h = SHORT(mi->p[mi->oldinum]->height);

          if (y - ST_Y < 0) i_error("updateMultIcon: y - ST_Y < 0");

          v_copy_rect(x, y - ST_Y, st_backing_screen, w, h, x, y);
        }

      v_draw_patch(mi->x, mi->y, mi->p[*mi->inum]);
      mi->oldinum = *mi->inum;
    }
}

void stlib_init_bin_icon(st_binicon_t *b, int x, int y, patch_t *i,
                         boolean *val, boolean *on)
{
  b->x = x;
  b->y = y;
  b->oldval = false;
  b->val = val;
  b->on = on;
  b->p = i;
}

void stlib_update_bin_icon(st_binicon_t *bi, boolean refresh)
{
  int x;
  int y;
  int w;
  int h;

  if (*bi->on && (bi->oldval != *bi->val || refresh))
    {
      x = bi->x - SHORT(bi->p->leftoffset);
      y = bi->y - SHORT(bi->p->topoffset);
      w = SHORT(bi->p->width);
      h = SHORT(bi->p->height);

      if (y - ST_Y < 0) i_error("updateBinIcon: y - ST_Y < 0");

      if (*bi->val)
        v_draw_patch(bi->x, bi->y, bi->p);
      else
        v_copy_rect(x, y - ST_Y, st_backing_screen, w, h, x, y);

      bi->oldval = *bi->val;
    }
}
