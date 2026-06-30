/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_scrollpane.c
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

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_scrollpane.h"
#include "txt_table.h"

#include "doomkeys.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SCROLLBAR_VERTICAL (1 << 0)
#define SCROLLBAR_HORIZONTAL (1 << 1)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int txt_scroll_pane_selectable(TXT_UNCAST_ARG(scrollpane));
static void txt_scroll_pane_size_calc(TXT_UNCAST_ARG(scrollpane));
static void txt_scroll_pane_drawer(TXT_UNCAST_ARG(scrollpane));
static int txt_scroll_pane_keypress(TXT_UNCAST_ARG(scrollpane), int key);
static void txt_scroll_pane_destructor(TXT_UNCAST_ARG(scrollpane));
static void txt_scroll_pane_mousepress(TXT_UNCAST_ARG(scrollpane), int x,
                                       int y, int b);
static void txt_scroll_pane_layout(TXT_UNCAST_ARG(scrollpane));
static void txt_scroll_pane_focus(TXT_UNCAST_ARG(scrollpane), int focused);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_scrollpane_class =
{
  txt_scroll_pane_selectable, txt_scroll_pane_size_calc,
  txt_scroll_pane_drawer,     txt_scroll_pane_keypress,
  txt_scroll_pane_destructor, txt_scroll_pane_mousepress,
  txt_scroll_pane_layout,     txt_scroll_pane_focus,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int full_width(txt_scrollpane_t *scrollpane)
{
  if (scrollpane->child != NULL)
    {
      return scrollpane->child->w;
    }
  else
    {
      return 0;
    }
}

static int full_height(txt_scrollpane_t *scrollpane)
{
  if (scrollpane->child != NULL)
    {
      return scrollpane->child->h;
    }
  else
    {
      return 0;
    }
}

/* Calculate which scroll bars the pane needs. */

static int needs_scroll_bars(txt_scrollpane_t *scrollpane)
{
  int result;

  result = 0;

  if (full_width(scrollpane) > scrollpane->w)
    {
      result |= SCROLLBAR_HORIZONTAL;
    }

  if (full_height(scrollpane) > scrollpane->h)
    {
      result |= SCROLLBAR_VERTICAL;
    }

  return result;
}

/* If a scrollbar isn't needed, the scroll position is reset. */

static void sanity_check_scrollbars(txt_scrollpane_t *scrollpane)
{
  int scrollbars;
  int max_x;
  int max_y;

  scrollbars = needs_scroll_bars(scrollpane);

  if ((scrollbars & SCROLLBAR_HORIZONTAL) == 0)
    {
      scrollpane->x = 0;
    }

  if ((scrollbars & SCROLLBAR_VERTICAL) == 0)
    {
      scrollpane->y = 0;
    }

  max_x = full_width(scrollpane) - scrollpane->w;
  max_y = full_height(scrollpane) - scrollpane->h;

  if (scrollpane->x < 0)
    {
      scrollpane->x = 0;
    }
  else if (scrollpane->x > max_x)
    {
      scrollpane->x = max_x;
    }

  if (scrollpane->y < 0)
    {
      scrollpane->y = 0;
    }
  else if (scrollpane->y > max_y)
    {
      scrollpane->y = max_y;
    }
}

static void txt_scroll_pane_size_calc(TXT_UNCAST_ARG(scrollpane))
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);
  int scrollbars;

  if (scrollpane->child != NULL)
    {
      txt_calc_widget_size(scrollpane->child);
    }

  /* Expand as necessary (to ensure that no scrollbars are needed)? */

  if (scrollpane->expand_w)
    {
      scrollpane->w = full_width(scrollpane);
    }

  if (scrollpane->expand_h)
    {
      scrollpane->h = full_height(scrollpane);
    }

  scrollpane->widget.w = scrollpane->w;
  scrollpane->widget.h = scrollpane->h;

  /* If we have scroll bars, we need to expand slightly to
   * accommodate them. Eg. if we have a vertical scrollbar, we
   * need to be an extra character wide.
   */

  scrollbars = needs_scroll_bars(scrollpane);

  if (scrollbars & SCROLLBAR_HORIZONTAL)
    {
      ++scrollpane->widget.h;
    }

  if (scrollbars & SCROLLBAR_VERTICAL)
    {
      ++scrollpane->widget.w;
    }

  if (scrollpane->child != NULL)
    {
      if (scrollpane->child->w < scrollpane->w)
        {
          scrollpane->child->w = scrollpane->w;
        }

      if (scrollpane->child->h < scrollpane->h)
        {
          scrollpane->child->h = scrollpane->h;
        }
    }
}

static void txt_scroll_pane_drawer(TXT_UNCAST_ARG(scrollpane))
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);
  int x1;
  int y1;
  int x2;
  int y2;
  int scrollbars;

  /* We set a clipping area of the scroll pane. */

  x1 = scrollpane->widget.x;
  y1 = scrollpane->widget.y;
  x2 = x1 + scrollpane->w;
  y2 = y1 + scrollpane->h;

  scrollbars = needs_scroll_bars(scrollpane);

  if (scrollbars & SCROLLBAR_HORIZONTAL)
    {
      txt_draw_horiz_scrollbar(x1, y1 + scrollpane->h, scrollpane->w,
                             scrollpane->x,
                             full_width(scrollpane) - scrollpane->w);
    }

  if (scrollbars & SCROLLBAR_VERTICAL)
    {
      txt_draw_vert_scrollbar(x1 + scrollpane->w, y1, scrollpane->h,
                            scrollpane->y,
                            full_height(scrollpane) - scrollpane->h);
    }

  txt_push_clip_area(x1, x2, y1, y2);

  /* Draw the child widget */

  if (scrollpane->child != NULL)
    {
      txt_draw_widget(scrollpane->child);
    }

  /* Restore old clipping area. */

  txt_pop_clip_area();
}

static void txt_scroll_pane_destructor(TXT_UNCAST_ARG(scrollpane))
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);

  if (scrollpane->child != NULL)
    {
      txt_destroy_widget(scrollpane->child);
    }
}

static void txt_scroll_pane_focus(TXT_UNCAST_ARG(scrollpane), int focused)
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);

  /* Whether the child is focused depends only on whether the scroll pane
   * itself is focused. Pass through focus to the child.
   */

  if (scrollpane->child != NULL)
    {
      txt_set_widget_focus(scrollpane->child, focused);
    }
}

/* Hack for tables - when browsing a table inside a scroll pane,
 * automatically scroll the window to show the newly-selected
 * item.
 */

static void show_selected_widget(txt_scrollpane_t *scrollpane)
{
  txt_widget_t *selected;

  selected = txt_get_selected_widget(scrollpane->child);

  /* Scroll up or down? */

  if (selected->y <= scrollpane->widget.y)
    {
      scrollpane->y -= scrollpane->widget.y - selected->y;
    }
  else if ((signed)(selected->y + selected->h) >
           (signed)(scrollpane->widget.y + scrollpane->h))
    {
      scrollpane->y += (selected->y + selected->h) -
                       (scrollpane->widget.y + scrollpane->h);
    }

  /* Scroll left or right? */

  if (selected->x <= scrollpane->widget.x)
    {
      scrollpane->x -= scrollpane->widget.x - selected->x;
    }
  else if ((signed)(selected->x + selected->w) >
           (signed)(scrollpane->widget.x + scrollpane->w))
    {
      scrollpane->x += (selected->x + selected->w) -
                       (scrollpane->widget.x + scrollpane->w);
    }
}

/* Another hack for tables - when scrolling in 'pages', the normal key press
 * event does not provide children with enough information to know how far
 * to move their selection to reach a new page. This function does so.
 * Note that it *only* affects scrolling in pages, not with arrows!
 * A side-effect of this, rather than 'pulling' the selection to fit within
 * the new page, is that we will jump straight over ranges of unselectable
 * items longer than a page, but that is also true of arrow-key scrolling.
 * The other unfortunate effect of doing things this way is that page keys
 * have no effect on tables _not_ in scrollpanes: not even home/end.
 */

static int page_selected_widget(txt_scrollpane_t *scrollpane, int key)
{
  int pagex = 0; /* No page left/right yet, but some keyboards have them */
  int pagey = 0;

  /* Subtract one from the absolute page distance as this is slightly more
   * intuitive: a page down first jumps to the bottom of the current page,
   * then proceeds to scroll onwards.
   */

  switch (key)
    {
    case KEY_PGUP:
      pagey = 1 - scrollpane->h;
      break;

    case KEY_PGDN:
      pagey = scrollpane->h - 1;
      break;

    default: /* We shouldn't even be in this function */
      return 0;
    }

  if (scrollpane->child->widget_class == &txt_table_class)
    {
      return txt_page_table(scrollpane->child, pagex, pagey);
    }

  return 0;
}

/* Interpret arrow key presses as scroll commands */

static int interpret_scroll_key(txt_scrollpane_t *scrollpane, int key)
{
  int maxy;

  switch (key)
    {
    case KEY_UPARROW:
      if (scrollpane->y > 0)
        {
          --scrollpane->y;
          return 1;
        }
      break;

    case KEY_DOWNARROW:
      if (scrollpane->y < full_height(scrollpane) - scrollpane->h)
        {
          ++scrollpane->y;
          return 1;
        }
      break;

    case KEY_LEFTARROW:
      if (scrollpane->x > 0)
        {
          --scrollpane->x;
          return 1;
        }
      break;

    case KEY_RIGHTARROW:
      if (scrollpane->x < full_width(scrollpane) - scrollpane->w)
        {
          ++scrollpane->x;
          return 1;
        }
      break;

    case KEY_PGUP:
      if (scrollpane->y > 0)
        {
          scrollpane->y -= scrollpane->h;
          if (scrollpane->y < 0)
            {
              scrollpane->y = 0;
            }

          return 1;
        }
      break;

    case KEY_PGDN:
      maxy = full_height(scrollpane) - scrollpane->h;
      if (scrollpane->y < maxy)
        {
          scrollpane->y += scrollpane->h;
          if (scrollpane->y > maxy)
            {
              scrollpane->y = maxy;
            }

          return 1;
        }
      break;

    default:
      break;
    }

  return 0;
}

static int txt_scroll_pane_keypress(TXT_UNCAST_ARG(scrollpane), int key)
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);
  int result;

  result = 0;

  if (scrollpane->child != NULL)
    {
      result = txt_widget_key_press(scrollpane->child, key);

      /* Gross hack - if we're scrolling in a menu with the keyboard,
       * automatically move the scroll pane to show the new
       * selected item.
       */

      if ((key == KEY_UPARROW || key == KEY_DOWNARROW ||
           key == KEY_LEFTARROW || key == KEY_RIGHTARROW ||
           key == KEY_PGUP || key == KEY_PGDN || key == KEY_TAB) &&
          scrollpane->child->widget_class == &txt_table_class)
        {
          if (page_selected_widget(scrollpane, key))
            {
              result = 1;
            }

          show_selected_widget(scrollpane);
        }

      /* If the child widget didn't use the keypress, we can see
       * if it can be interpreted as a scrolling command.
       */

      if (result == 0)
        {
          result = interpret_scroll_key(scrollpane, key);
        }
    }

  return result;
}

static void txt_scroll_pane_mousepress(TXT_UNCAST_ARG(scrollpane), int x,
                                       int y, int b)
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);
  int scrollbars;
  int rel_x;
  int rel_y;

  scrollbars = needs_scroll_bars(scrollpane);

  if (b == TXT_MOUSE_SCROLLUP)
    {
      if (scrollbars & SCROLLBAR_VERTICAL)
        {
          scrollpane->y -= 3;
        }
      else if (scrollbars & SCROLLBAR_HORIZONTAL)
        {
          scrollpane->x -= 3;
        }

      return;
    }
  else if (b == TXT_MOUSE_SCROLLDOWN)
    {
      if (scrollbars & SCROLLBAR_VERTICAL)
        {
          scrollpane->y += 3;
        }
      else if (scrollbars & SCROLLBAR_HORIZONTAL)
        {
          scrollpane->x += 3;
        }

      return;
    }

  rel_x = x - scrollpane->widget.x;
  rel_y = y - scrollpane->widget.y;

  /* Click on the horizontal scrollbar? */

  if ((scrollbars & SCROLLBAR_HORIZONTAL) && rel_y == scrollpane->h)
    {
      if (rel_x == 0)
        {
          --scrollpane->x;
        }
      else if (rel_x == scrollpane->w - 1)
        {
          ++scrollpane->x;
        }
      else
        {
          int range = full_width(scrollpane) - scrollpane->w;
          int bar_max = scrollpane->w - 3;

          scrollpane->x = ((rel_x - 1) * range + bar_max - 1) / bar_max;
        }

      return;
    }

  /* Click on the vertical scrollbar? */

  if ((scrollbars & SCROLLBAR_VERTICAL) && rel_x == scrollpane->w)
    {
      if (rel_y == 0)
        {
          --scrollpane->y;
        }
      else if (rel_y == scrollpane->h - 1)
        {
          ++scrollpane->y;
        }
      else
        {
          int range = full_height(scrollpane) - scrollpane->h;
          int bar_max = scrollpane->h - 3;

          scrollpane->y = ((rel_y - 1) * range + bar_max - 1) / bar_max;
        }

      return;
    }

  if (scrollpane->child != NULL)
    {
      txt_widget_mouse_press(scrollpane->child, x, y, b);
    }
}

static void txt_scroll_pane_layout(TXT_UNCAST_ARG(scrollpane))
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);

  sanity_check_scrollbars(scrollpane);

  /* The child widget takes the same position as the scroll pane
   * itself, but is offset by the scroll position.
   */

  if (scrollpane->child != NULL)
    {
      scrollpane->child->x = scrollpane->widget.x - scrollpane->x;
      scrollpane->child->y = scrollpane->widget.y - scrollpane->y;

      txt_layout_widget(scrollpane->child);
    }
}

static int txt_scroll_pane_selectable(TXT_UNCAST_ARG(scrollpane))
{
  TXT_CAST_ARG(txt_scrollpane_t, scrollpane);

  /* If scroll bars are displayed, the scroll pane must be selectable
   * so that we can use the arrow keys to scroll around.
   */

  if (needs_scroll_bars(scrollpane))
    {
      return 1;
    }

  /* Otherwise, whether this is selectable depends on the child widget. */

  return txt_selectable_widget(scrollpane->child);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_scrollpane_t *txt_new_scrollpane(int w, int h, TXT_UNCAST_ARG(target))
{
  TXT_CAST_ARG(txt_widget_t, target);
  txt_scrollpane_t *scrollpane;

  scrollpane = malloc(sizeof(txt_scrollpane_t));
  txt_init_widget(scrollpane, &txt_scrollpane_class);
  scrollpane->w = w;
  scrollpane->h = h;
  scrollpane->x = 0;
  scrollpane->y = 0;
  scrollpane->child = target;
  scrollpane->expand_w = w <= 0;
  scrollpane->expand_h = h <= 0;

  /* Set parent pointer for inner widget. */

  target->parent = &scrollpane->widget;

  return scrollpane;
}
