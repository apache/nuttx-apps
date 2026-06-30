/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_conditional.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2016 Simon Howard
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

#include "txt_conditional.h"
#include "txt_strut.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct txt_conditional_s
{
  txt_widget_t widget;
  int *var;
  int expected_value;
  txt_widget_t *child;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int condition_true(txt_conditional_t *conditional)
{
  return *conditional->var == conditional->expected_value;
}

static int txt_cond_selectable(TXT_UNCAST_ARG(conditional))
{
  TXT_CAST_ARG(txt_conditional_t, conditional);
  return condition_true(conditional) &&
         txt_selectable_widget(conditional->child);
}

static void txt_cond_size_calc(TXT_UNCAST_ARG(conditional))
{
  TXT_CAST_ARG(txt_conditional_t, conditional);

  if (!condition_true(conditional))
    {
      conditional->widget.w = 0;
      conditional->widget.h = 0;
    }
  else
    {
      txt_calc_widget_size(conditional->child);
      conditional->widget.w = conditional->child->w;
      conditional->widget.h = conditional->child->h;
    }
}

static void txt_cond_layout(TXT_UNCAST_ARG(conditional))
{
  TXT_CAST_ARG(txt_conditional_t, conditional);

  if (condition_true(conditional))
    {
      conditional->child->x = conditional->widget.x;
      conditional->child->y = conditional->widget.y;
      txt_layout_widget(conditional->child);
    }
}

static void txt_cond_drawer(TXT_UNCAST_ARG(conditional))
{
  TXT_CAST_ARG(txt_conditional_t, conditional);

  if (condition_true(conditional))
    {
      txt_draw_widget(conditional->child);
    }
}

static void txt_cond_destructor(TXT_UNCAST_ARG(conditional))
{
  TXT_CAST_ARG(txt_conditional_t, conditional);
  txt_destroy_widget(conditional->child);
}

static void txt_cond_focused(TXT_UNCAST_ARG(conditional), int focused)
{
  TXT_CAST_ARG(txt_conditional_t, conditional);

  if (condition_true(conditional))
    {
      txt_set_widget_focus(conditional->child, focused);
    }
}

static int txt_cond_key_press(TXT_UNCAST_ARG(conditional), int key)
{
  TXT_CAST_ARG(txt_conditional_t, conditional);

  if (condition_true(conditional))
    {
      return txt_widget_key_press(conditional->child, key);
    }

  return 0;
}

static void txt_cond_mouse_press(TXT_UNCAST_ARG(conditional), int x, int y,
                                 int b)
{
  TXT_CAST_ARG(txt_conditional_t, conditional);

  if (condition_true(conditional))
    {
      txt_widget_mouse_press(conditional->child, x, y, b);
    }
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_conditional_class =
{
  txt_cond_selectable, txt_cond_size_calc,  txt_cond_drawer,
  txt_cond_key_press,  txt_cond_destructor, txt_cond_mouse_press,
  txt_cond_layout,     txt_cond_focused,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_conditional_t *txt_new_conidtional(int *var, int expected_value,
                                       TXT_UNCAST_ARG(child))
{
  TXT_CAST_ARG(txt_widget_t, child);
  txt_conditional_t *conditional;

  conditional = malloc(sizeof(txt_conditional_t));

  txt_init_widget(conditional, &txt_conditional_class);
  conditional->var = var;
  conditional->expected_value = expected_value;
  conditional->child = child;

  child->parent = &conditional->widget;

  return conditional;
}

/* "Static" conditional that returns an empty strut if the given static
 * value is false. Kind of like a conditional but we only evaluate it at
 * creation time.
 */

txt_widget_t *txt_if(int conditional, TXT_UNCAST_ARG(child))
{
  TXT_CAST_ARG(txt_widget_t, child);

  if (conditional)
    {
      return child;
    }
  else
    {
      txt_strut_t *nullwidget;
      txt_destroy_widget(child);
      nullwidget = txt_new_strut(0, 0);
      return &nullwidget->widget;
    }
}
