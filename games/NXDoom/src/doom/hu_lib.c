/****************************************************************************
 * apps/games/NXDoom/src/doom/hu_lib.c
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
 * DESCRIPTION:  heads-up text and input code
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"

#include "i_swap.h"
#include "v_video.h"

#include "hu_lib.h"
#include "r_draw.h"
#include "r_local.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* boolean : whether the screen is always erased */

#define noterased viewwindowx

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void hu_lib_clear_text(hu_textline_t *t)
{
  t->len = 0;
  t->l[0] = 0;
  t->needsupdate = true;
}

void hu_lib_init_text_line(hu_textline_t *t, int x, int y, patch_t **f,
                           int sc)
{
  t->x = x;
  t->y = y;
  t->f = f;
  t->sc = sc;
  hu_lib_clear_text(t);
}

boolean hu_lib_add_char_to_text_line(hu_textline_t *t, char ch)
{
  if (t->len == HU_MAXLINELENGTH)
    {
      return false;
    }
  else
    {
      t->l[t->len++] = ch;
      t->l[t->len] = 0;
      t->needsupdate = 4;
      return true;
    }
}

boolean hu_lib_del_char_from_text_line(hu_textline_t *t)
{
  if (!t->len)
    {
      return false;
    }
  else
    {
      t->l[--t->len] = 0;
      t->needsupdate = 4;
      return true;
    }
}

void hu_lib_draw_text_line(hu_textline_t *l, boolean drawcursor)
{
  int i;
  int w;
  int x;
  unsigned char c;

  /* draw the new stuff */

  x = l->x;
  for (i = 0; i < l->len; i++)
    {
      c = toupper(l->l[i]);
      if (c != ' ' && c >= l->sc && c <= '_')
        {
          w = SHORT(l->f[c - l->sc]->width);
          if (x + w > SCREENWIDTH) break;
          v_draw_patch_direct(x, l->y, l->f[c - l->sc]);
          x += w;
        }
      else
        {
          x += 4;
          if (x >= SCREENWIDTH) break;
        }
    }

  /* draw the cursor if requested */

  if (drawcursor && x + SHORT(l->f['_' - l->sc]->width) <= SCREENWIDTH)
    {
      v_draw_patch_direct(x, l->y, l->f['_' - l->sc]);
    }
}

/* sorta called by hu_erase and just better darn get things straight */

void hu_lib_erase_text_line(hu_textline_t *l)
{
  int lh;
  int y;
  int yoffset;

  /* Only erases when NOT in automap and the screen is reduced, and the text
   * must either need updating or refreshing (because of a recent change back
   * from the automap)
   */

  if (!automapactive && viewwindowx && l->needsupdate)
    {
      lh = SHORT(l->f[0]->height) + 1;
      for (y = l->y, yoffset = y * SCREENWIDTH; y < l->y + lh;
           y++, yoffset += SCREENWIDTH)
        {
          if (y < viewwindowy || y >= viewwindowy + viewheight)
            r_video_erase(yoffset, SCREENWIDTH); /* erase entire line */
          else
            {
              r_video_erase(yoffset, viewwindowx); /* erase left border */
              r_video_erase(yoffset + viewwindowx + viewwidth, viewwindowx);

              /* erase right border */
            }
        }
    }

  if (l->needsupdate) l->needsupdate--;
}

void hu_lib_init_stext(hu_stext_t *s, int x, int y, int h, patch_t **font,
                     int startchar, boolean *on)
{
  int i;

  s->h = h;
  s->on = on;
  s->laston = true;
  s->cl = 0;
  for (i = 0; i < h; i++)
    hu_lib_init_text_line(&s->l[i], x, y - i * (SHORT(font[0]->height) + 1),
                       font, startchar);
}

void hu_lib_add_line_to_stext(hu_stext_t *s)
{
  int i;

  /* add a clear line */

  if (++s->cl == s->h) s->cl = 0;
  hu_lib_clear_text(&s->l[s->cl]);

  /* everything needs updating */

  for (i = 0; i < s->h; i++)
    {
      s->l[i].needsupdate = 4;
    }
}

void hu_lib_add_messsage_to_stext(hu_stext_t *s, const char *prefix,
                             const char *msg)
{
  hu_lib_add_line_to_stext(s);

  if (prefix)
    {
      while (*prefix)
        {
          hu_lib_add_char_to_text_line(&s->l[s->cl], *(prefix++));
        }
    }

  while (*msg)
    {
      hu_lib_add_char_to_text_line(&s->l[s->cl], *(msg++));
    }
}

void hu_lib_draw_stext(hu_stext_t *s)
{
  int i;
  int idx;
  hu_textline_t *l;

  if (!*s->on) return; /* if not on, don't draw */

  /* draw everything */

  for (i = 0; i < s->h; i++)
    {
      idx = s->cl - i;
      if (idx < 0) idx += s->h; /* handle queue of lines */

      l = &s->l[idx];

      /* need a decision made here on whether to skip the draw */

      hu_lib_draw_text_line(l, false); /* no cursor, please */
    }
}

void hu_lib_erase_stext(hu_stext_t *s)
{
  int i;

  for (i = 0; i < s->h; i++)
    {
      if (s->laston && !*s->on) s->l[i].needsupdate = 4;
      hu_lib_erase_text_line(&s->l[i]);
    }

  s->laston = *s->on;
}

void hu_lib_init_itext(hu_itext_t *it, int x, int y, patch_t **font,
                     int startchar, boolean *on)
{
  it->lm = 0; /* default left margin is start of text */
  it->on = on;
  it->laston = true;
  hu_lib_init_text_line(&it->l, x, y, font, startchar);
}

/* The following deletion routines adhere to the left margin restriction */

void hu_lib_del_char_from_itext(hu_itext_t *it)
{
  if (it->l.len != it->lm) hu_lib_del_char_from_text_line(&it->l);
}

/* Resets left margin as well */

void hu_lib_reset_itext(hu_itext_t *it)
{
  it->lm = 0;
  hu_lib_clear_text(&it->l);
}

/* wrapper function for handling general keyed input.
 * returns true if it ate the key
 */

boolean hu_lib_key_in_itext(hu_itext_t *it, unsigned char ch)
{
  ch = toupper(ch);

  if (ch >= ' ' && ch <= '_')
    {
      hu_lib_add_char_to_text_line(&it->l, (char)ch);
    }
  else if (ch == KEY_BACKSPACE)
    {
      hu_lib_del_char_from_itext(it);
    }
  else if (ch != KEY_ENTER)
    {
      return false; /* did not eat key */
    }

  return true; /* ate the key */
}

void hu_lib_draw_itext(hu_itext_t *it)
{
  hu_textline_t *l = &it->l;

  if (!*it->on) return;
  hu_lib_draw_text_line(l, true); /* draw the line w/ cursor */
}

void hu_lib_erase_itext(hu_itext_t *it)
{
  if (it->laston && !*it->on) it->l.needsupdate = 4;
  hu_lib_erase_text_line(&it->l);
  it->laston = *it->on;
}
