/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_sdl.c
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
 *
 * Text mode emulation in SDL
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

#include "txt_main.h"
#include "txt_sdl.h"
#include "txt_utf8.h"

/* Fonts: */

#include "txt_font.h"
#include "fonts/codepage.h"
#include "fonts/large.h"
#include "fonts/normal.h"
#include "fonts/small.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Time between character blinks in ms */

#define BLINK_PERIOD 250

/* XXX: duplicate from doomtype.h */

#define arrlen(array) (sizeof(array) / sizeof(*array))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned char *screendata;

/* Unicode key mapping; see codepage.h. */

static const short g_code_page_to_unicode[] = CODE_PAGE_TO_UNICODE;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Searches the desktop screen buffer to determine whether there are any
 * blinking characters.
 */

#if 0 /* UNUSED */
static int txt_has_blinking_chars(void)
{
  int x;
  int y;
  unsigned char *p;

  /* Check all characters in screen buffer */

  for (y = 0; y < TXT_SCREEN_H; ++y)
    {
      for (x = 0; x < TXT_SCREEN_W; ++x)
        {
          p = &screendata[(y * TXT_SCREEN_W + x) * 2];

          if (p[1] & 0x80)
            {
              return 1; /* This character is blinking */
            }
        }
    }

  /* None found */

  return 0;
}

static void txt_string_concat(char *dest, const char *src, size_t dest_len)
{
  size_t offset;

  offset = strlen(dest);
  if (offset > dest_len)
    {
      offset = dest_len;
    }

  txt_string_copy(dest + offset, src, dest_len - offset);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Initialize text mode screen
 *
 * Returns 1 if successful, 0 if an error occurred
 */

int txt_init(void)
{
  return 1;
}

void txt_shutdown(void)
{
}

void txt_set_colour(txt_color_t color, int r, int g, int b)
{
}

unsigned char *txt_get_screen_data(void)
{
  return screendata;
}

void txt_update_screen_area(int x, int y, int w, int h)
{
}

void txt_update_screen(void)
{
  txt_update_screen_area(0, 0, TXT_SCREEN_W, TXT_SCREEN_H);
}

void txt_get_mouse_position(int *x, int *y)
{
}

signed int txt_getchar(void)
{
  return -1;
}

int txt_get_modifier_state(txt_modifier_t mod)
{
  return 0;
}

int txt_unicode_character(unsigned int c)
{
  unsigned int i;

  /* Check the code page mapping to see if this character maps
   * to anything.
   */

  for (i = 0; i < arrlen(g_code_page_to_unicode); ++i)
    {
      if (g_code_page_to_unicode[i] == c)
        {
          return i;
        }
    }

  return -1;
}

void txt_get_key_description(int key, char *buf, size_t buf_len)
{
}

/* Sleeps until an event is received, the screen needs to be redrawn,
 * or until timeout expires (if timeout != 0)
 */

void txt_sleep(int timeout)
{
}

void txt_set_input_mode(txt_input_mode_t mode)
{
}

void txt_set_window_title(const char *title)
{
}

void txt_sdl_set_event_callback(void *user_data)
{
}

/* Safe string functions. */

void txt_string_copy(char *dest, const char *src, size_t dest_len)
{
  if (dest_len < 1)
    {
      return;
    }

  dest[dest_len - 1] = '\0';
  strncpy(dest, src, dest_len - 1);
}

/* Safe, portable vsnprintf(). */

int txt_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args)
{
  int result;

  if (buf_len < 1)
    {
      return 0;
    }

  /* Windows (and other OSes?) has a vsnprintf() that doesn't always
   * append a trailing \0. So we must do it, and write into a buffer
   * that is one byte shorter; otherwise this function is unsafe.
   */

  result = vsnprintf(buf, buf_len, s, args);

  /* If truncated, change the final char in the buffer to a \0.
   * A negative result indicates a truncated buffer on Windows.
   */

  if (result < 0 || result >= buf_len)
    {
      buf[buf_len - 1] = '\0';
      result = buf_len - 1;
    }

  return result;
}

/* Safe, portable snprintf(). */

int txt_snprintf(char *buf, size_t buf_len, const char *s, ...)
{
  va_list args;
  int result;
  va_start(args, s);
  result = txt_vsnprintf(buf, buf_len, s, args);
  va_end(args);
  return result;
}
