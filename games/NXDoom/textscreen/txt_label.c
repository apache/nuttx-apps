/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_label.c
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_label.h"
#include "txt_main.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_label_size_calc(TXT_UNCAST_ARG(label));
static void txt_label_drawer(TXT_UNCAST_ARG(label));
static void txt_label_destructor(TXT_UNCAST_ARG(label));

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_label_class =
{
  txt_never_selectable,
  txt_label_size_calc,
  txt_label_drawer,
  NULL,
  txt_label_destructor,
  NULL,
  NULL,
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_label_size_calc(TXT_UNCAST_ARG(label))
{
  TXT_CAST_ARG(txt_label_t, label);

  label->widget.w = label->w;
  label->widget.h = label->h;
}

static void txt_label_drawer(TXT_UNCAST_ARG(label))
{
  TXT_CAST_ARG(txt_label_t, label);
  unsigned int x;
  unsigned int y;
  int origin_x;
  int origin_y;
  unsigned int align_indent = 0;
  unsigned int w;
  unsigned int sw;

  w = label->widget.w;

  if (label->bgcolor >= 0)
    {
      txt_bgcolour(label->bgcolor, 0);
    }
  if (label->fgcolor >= 0)
    {
      txt_fgcolour(label->fgcolor);
    }

  txt_get_xy(&origin_x, &origin_y);

  for (y = 0; y < label->h; ++y)
    {
      /* Calculate the amount to indent this line due to the align
       * setting
       */

      sw = txt_utf8_strlen(label->lines[y]);
      switch (label->widget.align)
        {
        case TXT_HORIZ_LEFT:
          align_indent = 0;
          break;
        case TXT_HORIZ_CENTER:
          align_indent = (label->w - sw) / 2;
          break;
        case TXT_HORIZ_RIGHT:
          align_indent = label->w - sw;
          break;
        }

      /* Draw this line */

      txt_goto_xy(origin_x, origin_y + y);

      /* Gap at the start */

      for (x = 0; x < align_indent; ++x)
        {
          txt_draw_string(" ");
        }

      /* The string itself */

      txt_draw_string(label->lines[y]);
      x += sw;

      /* Gap at the end */

      for (; x < w; ++x)
        {
          txt_draw_string(" ");
        }
    }
}

static void txt_label_destructor(TXT_UNCAST_ARG(label))
{
  TXT_CAST_ARG(txt_label_t, label);

  free(label->label);
  free(label->lines);
}

#if 0 /* UNUSED */
static void txt_set_bg_colour(txt_label_t *label, txt_color_t color)
{
  label->bgcolor = color;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void txt_set_label(txt_label_t *label, const char *value)
{
  char *p;
  unsigned int y;

  /* Free back the old label */

  free(label->label);
  free(label->lines);

  /* Set the new value */

  label->label = strdup(value);

  /* Work out how many lines in this label */

  label->h = 1;

  for (p = label->label; *p != '\0'; ++p)
    {
      if (*p == '\n')
        {
          ++label->h;
        }
    }

  /* Split into lines */

  label->lines = malloc(sizeof(char *) * label->h);
  label->lines[0] = label->label;
  y = 1;

  for (p = label->label; *p != '\0'; ++p)
    {
      if (*p == '\n')
        {
          label->lines[y] = p + 1;
          *p = '\0';
          ++y;
        }
    }

  label->w = 0;

  for (y = 0; y < label->h; ++y)
    {
      unsigned int line_len;

      line_len = txt_utf8_strlen(label->lines[y]);

      if (line_len > label->w) label->w = line_len;
    }
}

txt_label_t *txt_new_label(const char *text)
{
  txt_label_t *label;

  label = malloc(sizeof(txt_label_t));

  txt_init_widget(label, &txt_label_class);
  label->label = NULL;
  label->lines = NULL;

  /* Default colors */

  label->bgcolor = -1;
  label->fgcolor = -1;

  txt_set_label(label, text);

  return label;
}

void txt_set_fg_colour(txt_label_t *label, txt_color_t color)
{
  label->fgcolor = color;
}
