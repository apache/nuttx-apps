/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_spinctrl.c
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

#include "doomkeys.h"

#include "txt_gui.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_spinctrl.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_spin_control_size_calc(TXT_UNCAST_ARG(spincontrol));
static void txt_spin_control_drawer(TXT_UNCAST_ARG(spincontrol));
static int txt_spincontrol_keypress(TXT_UNCAST_ARG(spincontrol), int key);
static void txt_spin_control_destructor(TXT_UNCAST_ARG(spincontrol));
static void txt_spincontrol_mousepress(TXT_UNCAST_ARG(spincontrol), int x,
                                      int y, int b);
static void txt_spincontrol_focused(TXT_UNCAST_ARG(spincontrol),
                                    int focused);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_spincontrol_class =
{
  txt_always_selectable,
  txt_spin_control_size_calc,
  txt_spin_control_drawer,
  txt_spincontrol_keypress,
  txt_spin_control_destructor,
  txt_spincontrol_mousepress,
  NULL,
  txt_spincontrol_focused,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Generate the format string to be used for displaying floats */

static void float_format_string(float step, char *buf, size_t buf_len)
{
  int precision;

  precision = (int)ceil(-log(step) / log(10));

  if (precision > 0)
    {
      txt_snprintf(buf, buf_len, "%%.%if", precision);
    }
  else
    {
      txt_string_copy(buf, "%.1f", buf_len);
    }
}

/* Number of characters needed to represent a character */

static unsigned int int_width(int val)
{
  char buf[25];

  txt_snprintf(buf, sizeof(buf), "%i", val);

  return strlen(buf);
}

static unsigned int float_width(float val, float step)
{
  unsigned int precision;
  unsigned int result;

  /* Calculate the width of the int value */

  result = int_width((int)val);

  /* Add a decimal part if the precision specifies it */

  precision = (unsigned int)ceil(-log(step) / log(10));

  if (precision > 0)
    {
      result += precision + 1;
    }

  return result;
}

/* Returns the minimum width of the input box */

static unsigned int spin_control_width(txt_spincontrol_t *spincontrol)
{
  unsigned int minw;
  unsigned int maxw;

  switch (spincontrol->type)
    {
    case TXT_SPINCONTROL_FLOAT:
      minw = float_width(spincontrol->min.f, spincontrol->step.f);
      maxw = float_width(spincontrol->max.f, spincontrol->step.f);
      break;

    default:
    case TXT_SPINCONTROL_INT:
      minw = int_width(spincontrol->min.i);
      maxw = int_width(spincontrol->max.i);
      break;
    }

  /* Choose the wider of the two values.  Add one so that there is always
   * space for the cursor when editing.
   */

  if (minw > maxw)
    {
      return minw;
    }
  else
    {
      return maxw;
    }
}

static void txt_spin_control_size_calc(TXT_UNCAST_ARG(spincontrol))
{
  TXT_CAST_ARG(txt_spincontrol_t, spincontrol);

  spincontrol->widget.w = spin_control_width(spincontrol) + 5;
  spincontrol->widget.h = 1;
}

static void set_buffer(txt_spincontrol_t *spincontrol)
{
  char format[25];

  switch (spincontrol->type)
    {
    case TXT_SPINCONTROL_INT:
      txt_snprintf(spincontrol->buffer, spincontrol->buffer_len, "%i",
                   spincontrol->value->i);
      break;

    case TXT_SPINCONTROL_FLOAT:
      float_format_string(spincontrol->step.f, format, sizeof(format));
      txt_snprintf(spincontrol->buffer, spincontrol->buffer_len, format,
                   spincontrol->value->f);
      break;
    }
}

static void txt_spin_control_drawer(TXT_UNCAST_ARG(spincontrol))
{
  TXT_CAST_ARG(txt_spincontrol_t, spincontrol);
  unsigned int i;
  unsigned int padding;
  txt_saved_colors_t colors;
  int bw;
  int focused;

  focused = spincontrol->widget.focused;

  txt_save_colours(&colors);

  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);
  txt_draw_code_page_string("\x1b ");

  txt_restore_colours(&colors);

  /* Choose background color */

  if (focused && spincontrol->editing)
    {
      txt_bgcolour(TXT_COLOR_BLACK, 0);
    }
  else
    {
      txt_set_widget_bg(spincontrol);
    }

  if (!spincontrol->editing)
    {
      set_buffer(spincontrol);
    }

  i = 0;

  bw = txt_utf8_strlen(spincontrol->buffer);
  padding = spincontrol->widget.w - bw - 4;

  while (i < padding)
    {
      txt_draw_string(" ");
      ++i;
    }

  txt_draw_string(spincontrol->buffer);
  i += bw;

  while (i < spincontrol->widget.w - 4)
    {
      txt_draw_string(" ");
      ++i;
    }

  txt_restore_colours(&colors);
  txt_fgcolour(TXT_COLOR_BRIGHT_CYAN);
  txt_draw_code_page_string(" \x1a");
}

static void txt_spin_control_destructor(TXT_UNCAST_ARG(spincontrol))
{
  TXT_CAST_ARG(txt_spincontrol_t, spincontrol);

  free(spincontrol->buffer);
}

static void add_character(txt_spincontrol_t *spincontrol, int key)
{
  if (txt_utf8_strlen(spincontrol->buffer) <
          spin_control_width(spincontrol) &&
      strlen(spincontrol->buffer) < spincontrol->buffer_len - 2)
    {
      spincontrol->buffer[strlen(spincontrol->buffer) + 1] = '\0';
      spincontrol->buffer[strlen(spincontrol->buffer)] = key;
    }
}

static void backspace(txt_spincontrol_t *spincontrol)
{
  if (txt_utf8_strlen(spincontrol->buffer) > 0)
    {
      spincontrol->buffer[strlen(spincontrol->buffer) - 1] = '\0';
    }
}

static void enforce_limits(txt_spincontrol_t *spincontrol)
{
  switch (spincontrol->type)
    {
    case TXT_SPINCONTROL_INT:
      if (spincontrol->value->i > spincontrol->max.i)
        spincontrol->value->i = spincontrol->max.i;
      else if (spincontrol->value->i < spincontrol->min.i)
        spincontrol->value->i = spincontrol->min.i;
      break;

    case TXT_SPINCONTROL_FLOAT:
      if (spincontrol->value->f > spincontrol->max.f)
        spincontrol->value->f = spincontrol->max.f;
      else if (spincontrol->value->f < spincontrol->min.f)
        spincontrol->value->f = spincontrol->min.f;
      break;
    }
}

static void finish_editing(txt_spincontrol_t *spincontrol)
{
  switch (spincontrol->type)
    {
    case TXT_SPINCONTROL_INT:
      spincontrol->value->i = atoi(spincontrol->buffer);
      break;

    case TXT_SPINCONTROL_FLOAT:
      spincontrol->value->f = (float)atof(spincontrol->buffer);
      break;
    }

  spincontrol->editing = 0;
  enforce_limits(spincontrol);
}

static int txt_spincontrol_keypress(TXT_UNCAST_ARG(spincontrol), int key)
{
  TXT_CAST_ARG(txt_spincontrol_t, spincontrol);

  /* Enter to enter edit mode */

  if (spincontrol->editing)
    {
      if (key == KEY_ENTER)
        {
          finish_editing(spincontrol);
          return 1;
        }

      if (key == KEY_ESCAPE)
        {
          /* Abort without saving value */

          spincontrol->editing = 0;
          return 1;
        }

      if (isdigit(key) || key == '-' || key == '.')
        {
          add_character(spincontrol, key);
          return 1;
        }

      if (key == KEY_BACKSPACE)
        {
          backspace(spincontrol);
          return 1;
        }
    }
  else
    {
      /* Non-editing mode */

      if (key == KEY_ENTER)
        {
          spincontrol->editing = 1;
          txt_string_copy(spincontrol->buffer, "", spincontrol->buffer_len);
          return 1;
        }

      if (key == KEY_LEFTARROW)
        {
          switch (spincontrol->type)
            {
            case TXT_SPINCONTROL_INT:
              spincontrol->value->i -= spincontrol->step.i;
              break;

            case TXT_SPINCONTROL_FLOAT:
              spincontrol->value->f -= spincontrol->step.f;
              break;
            }

          enforce_limits(spincontrol);

          return 1;
        }

      if (key == KEY_RIGHTARROW)
        {
          switch (spincontrol->type)
            {
            case TXT_SPINCONTROL_INT:
              spincontrol->value->i += spincontrol->step.i;
              break;

            case TXT_SPINCONTROL_FLOAT:
              spincontrol->value->f += spincontrol->step.f;
              break;
            }

          enforce_limits(spincontrol);

          return 1;
        }
    }

  return 0;
}

static void txt_spincontrol_mousepress(TXT_UNCAST_ARG(spincontrol), int x,
                                      int y, int b)
{
  TXT_CAST_ARG(txt_spincontrol_t, spincontrol);
  unsigned int rel_x;

  rel_x = x - spincontrol->widget.x;

  if (rel_x < 2)
    {
      txt_spincontrol_keypress(spincontrol, KEY_LEFTARROW);
    }
  else if (rel_x >= spincontrol->widget.w - 2)
    {
      txt_spincontrol_keypress(spincontrol, KEY_RIGHTARROW);
    }
}

static void txt_spincontrol_focused(TXT_UNCAST_ARG(spincontrol), int focused)
{
  TXT_CAST_ARG(txt_spincontrol_t, spincontrol);

  finish_editing(spincontrol);
}

static txt_spincontrol_t *txt_base_spincontrol(void)
{
  txt_spincontrol_t *spincontrol;

  spincontrol = malloc(sizeof(txt_spincontrol_t));

  txt_init_widget(spincontrol, &txt_spincontrol_class);
  spincontrol->buffer_len = 25;
  spincontrol->buffer = malloc(spincontrol->buffer_len);
  txt_string_copy(spincontrol->buffer, "", spincontrol->buffer_len);
  spincontrol->editing = 0;

  return spincontrol;
}

txt_spincontrol_t *txt_newspin_control(int *value, int min, int max)
{
  txt_spincontrol_t *spincontrol;

  spincontrol = txt_base_spincontrol();
  spincontrol->type = TXT_SPINCONTROL_INT;
  spincontrol->value = (void *)value;
  spincontrol->min.i = min;
  spincontrol->max.i = max;
  spincontrol->step.i = 1;
  set_buffer(spincontrol);

  return spincontrol;
}

txt_spincontrol_t *txt_new_float_spincontrol(float *value, float min,
                                             float max)
{
  txt_spincontrol_t *spincontrol;

  spincontrol = txt_base_spincontrol();
  spincontrol->type = TXT_SPINCONTROL_FLOAT;
  spincontrol->value = (void *)value;
  spincontrol->min.f = min;
  spincontrol->max.f = max;
  spincontrol->step.f = 0.1f;
  set_buffer(spincontrol);

  return spincontrol;
}
