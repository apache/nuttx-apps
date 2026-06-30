/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_inputbox.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomkeys.h"

#include "txt_gui.h"
#include "txt_inputbox.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_utf8.h"
#include "txt_window.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_input_box_size_calc(TXT_UNCAST_ARG(inputbox));
static void txt_input_box_drawer(TXT_UNCAST_ARG(inputbox));
static int txt_input_box_keypress(TXT_UNCAST_ARG(inputbox), int key);
static void txt_input_box_destructor(TXT_UNCAST_ARG(inputbox));
static void txt_input_box_mousepress(TXT_UNCAST_ARG(inputbox), int x, int y,
                                     int b);
static void txt_input_box_focused(TXT_UNCAST_ARG(inputbox), int focused);

/****************************************************************************
 * Public Data
 ****************************************************************************/

txt_widget_class_t txt_inputbox_class;
txt_widget_class_t txt_int_inputbox_class;

txt_widget_class_t txt_inputbox_class =
{
  txt_always_selectable,
  txt_input_box_size_calc,
  txt_input_box_drawer,
  txt_input_box_keypress,
  txt_input_box_destructor,
  txt_input_box_mousepress,
  NULL,
  txt_input_box_focused,
};

txt_widget_class_t txt_int_inputbox_class =
{
  txt_always_selectable,
  txt_input_box_size_calc,
  txt_input_box_drawer,
  txt_input_box_keypress,
  txt_input_box_destructor,
  txt_input_box_mousepress,
  NULL,
  txt_input_box_focused,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void set_buffer_from_value(txt_inputbox_t *inputbox)
{
  if (inputbox->widget.widget_class == &txt_inputbox_class)
    {
      char **value = (char **)inputbox->value;

      if (*value != NULL)
        {
          txt_string_copy(inputbox->buffer, *value,
                         strnlen(*value, inputbox->buffer_len) + 1);
        }
      else
        {
          txt_string_copy(inputbox->buffer, "", inputbox->buffer_len);
        }
    }
  else if (inputbox->widget.widget_class == &txt_int_inputbox_class)
    {
      int *value = (int *)inputbox->value;
      txt_snprintf(inputbox->buffer, inputbox->buffer_len, "%i", *value);
    }
}

static void start_editing(txt_inputbox_t *inputbox)
{
  /* Integer input boxes start from an empty buffer: */

  if (inputbox->widget.widget_class == &txt_int_inputbox_class)
    {
      txt_string_copy(inputbox->buffer, "", inputbox->buffer_len);
    }
  else
    {
      set_buffer_from_value(inputbox);
    }

  /* Switch to text input mode so we get shifted input. */

  txt_set_input_mode(TXT_INPUT_TEXT);
  inputbox->editing = 1;
}

static void stop_editing(txt_inputbox_t *inputbox)
{
  if (inputbox->editing)
    {
      /* Switch back to normal input mode. */

      txt_set_input_mode(TXT_INPUT_NORMAL);
      inputbox->editing = 0;
    }
}

static void finish_editing(txt_inputbox_t *inputbox)
{
  if (!inputbox->editing)
    {
      return;
    }

  /* Save the new value back to the variable. */

  if (inputbox->widget.widget_class == &txt_inputbox_class)
    {
      free(*((char **)inputbox->value));
      *((char **)inputbox->value) = strdup(inputbox->buffer);
    }
  else if (inputbox->widget.widget_class == &txt_int_inputbox_class)
    {
      *((int *)inputbox->value) = atoi(inputbox->buffer);
    }

  txt_emit_signal(&inputbox->widget, "changed");

  stop_editing(inputbox);
}

static void txt_input_box_size_calc(TXT_UNCAST_ARG(inputbox))
{
  TXT_CAST_ARG(txt_inputbox_t, inputbox);

  /* Enough space for the box + cursor */

  inputbox->widget.w = inputbox->size + 1;
  inputbox->widget.h = 1;
}

static void txt_input_box_drawer(TXT_UNCAST_ARG(inputbox))
{
  TXT_CAST_ARG(txt_inputbox_t, inputbox);
  int focused;
  int i;
  int chars;
  int w;

  focused = inputbox->widget.focused;
  w = inputbox->widget.w;

  /* Select the background color based on whether we are currently
   * editing, and if not, whether the widget is focused.
   */

  if (inputbox->editing && focused)
    {
      txt_bgcolour(TXT_COLOR_BLACK, 0);
    }
  else
    {
      txt_set_widget_bg(inputbox);
    }

  if (!inputbox->editing)
    {
      /* If not editing, use the current value from inputbox->value. */

      set_buffer_from_value(inputbox);
    }

  /* If string size exceeds the widget's width, show only the end. */

  if (txt_utf8_strlen(inputbox->buffer) > w - 1)
    {
      txt_draw_code_page_string("\xae");
      txt_draw_string(txt_utf8_skip_chars(
          inputbox->buffer, txt_utf8_strlen(inputbox->buffer) - w + 2));
      chars = w - 1;
    }
  else
    {
      txt_draw_string(inputbox->buffer);
      chars = txt_utf8_strlen(inputbox->buffer);
    }

  if (chars < w && inputbox->editing && focused)
    {
      txt_bgcolour(TXT_COLOR_BLACK, 1);
      txt_draw_string("_");
      ++chars;
    }

  for (i = chars; i < w; ++i)
    {
      txt_draw_string(" ");
    }
}

static void txt_input_box_destructor(TXT_UNCAST_ARG(inputbox))
{
  TXT_CAST_ARG(txt_inputbox_t, inputbox);

  stop_editing(inputbox);
  free(inputbox->buffer);
}

static void backspace(txt_inputbox_t *inputbox)
{
  unsigned int len;
  char *p;

  len = txt_utf8_strlen(inputbox->buffer);

  if (len > 0)
    {
      p = txt_utf8_skip_chars(inputbox->buffer, len - 1);
      *p = '\0';
    }
}

static void add_character(txt_inputbox_t *inputbox, int key)
{
  char *end;
  char *p;

  if (txt_utf8_strlen(inputbox->buffer) < inputbox->size)
    {
      /* Add character to the buffer */

      end = inputbox->buffer + strlen(inputbox->buffer);
      p = txt_encode_utf8(end, key);
      *p = '\0';
    }
}

static int txt_input_box_keypress(TXT_UNCAST_ARG(inputbox), int key)
{
  TXT_CAST_ARG(txt_inputbox_t, inputbox);
  unsigned int c;

  if (!inputbox->editing)
    {
      if (key == KEY_ENTER)
        {
          start_editing(inputbox);
          return 1;
        }

      /* Backspace or delete erases the contents of the box. */

      if ((key == KEY_DEL || key == KEY_BACKSPACE) &&
          inputbox->widget.widget_class == &txt_inputbox_class)
        {
          free(*((char **)inputbox->value));
          *((char **)inputbox->value) = strdup("");
        }

      return 0;
    }

  if (key == KEY_ENTER)
    {
      finish_editing(inputbox);
    }

  if (key == KEY_ESCAPE)
    {
      stop_editing(inputbox);
    }

  if (key == KEY_BACKSPACE)
    {
      backspace(inputbox);
    }

  c = TXT_KEY_TO_UNICODE(key);

  /* Add character to the buffer, but only if it's a printable character
   * that we can represent on the screen.
   */

  if (isprint(c) || (c >= 128 && txt_unicode_character(c) >= 0))
    {
      add_character(inputbox, c);
    }

  return 1;
}

static void txt_input_box_mousepress(TXT_UNCAST_ARG(inputbox), int x, int y,
                                     int b)
{
  TXT_CAST_ARG(txt_inputbox_t, inputbox);

  if (b == TXT_MOUSE_LEFT)
    {
      /* Make mouse clicks start editing the box */

      if (!inputbox->editing)
        {
          /* Send a simulated keypress to start editing */

          txt_widget_key_press(inputbox, KEY_ENTER);
        }
    }
}

static void txt_input_box_focused(TXT_UNCAST_ARG(inputbox), int focused)
{
  TXT_CAST_ARG(txt_inputbox_t, inputbox);

  /* Stop editing when we lose focus. */

  if (inputbox->editing && !focused)
    {
      finish_editing(inputbox);
    }
}

static txt_inputbox_t *new_input_box(txt_widget_class_t *widget_class,
                                   void *value, int size)
{
  txt_inputbox_t *inputbox;

  inputbox = malloc(sizeof(txt_inputbox_t));

  txt_init_widget(inputbox, widget_class);
  inputbox->value = value;
  inputbox->size = size;

  /* 'size' is the maximum number of characters that can be entered,
   * but for a UTF-8 string, each character can take up to four
   * characters.
   */

  inputbox->buffer_len = size * 4 + 1;
  inputbox->buffer = malloc(inputbox->buffer_len);
  inputbox->editing = 0;

  return inputbox;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_inputbox_t *txt_new_input_box(char **value, int size)
{
  return new_input_box(&txt_inputbox_class, value, size);
}

txt_inputbox_t *txt_new_int_input_box(int *value, int size)
{
  return new_input_box(&txt_int_inputbox_class, value, size);
}
