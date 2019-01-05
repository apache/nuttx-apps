/****************************************************************************
 * apps/graphics/pdcurses/pdc_color.c
 * Public Domain Curses
 * RCSID("$Id: color.c,v 1.83 2008/07/13 16:08:18 wmcbrine Exp $")
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Adapted by: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted from the original public domain pdcurses by Gregory Nutt and
 * released as part of NuttX under the 3-clause BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/* Name: color
 *
 * Synopsis:
 *       int start_color(void);
 *       int init_pair(short pair, short fg, short bg);
 *       int init_color(short color, short red, short green, short blue);
 *       bool has_colors(void);
 *       bool can_change_color(void);
 *       int color_content(short color, short *red, short *green, short *blue);
 *       int pair_content(short pair, short *fg, short *bg);
 *
 *       int assume_default_colors(int f, int b);
 *       int use_default_colors(void);
 *
 *       int PDC_set_line_color(short color);
 *
 * Description:
 *       To use these routines, start_color() must be called, usually
 *       immediately after initscr(). Colors are always used in pairs,
 *       referred to as color-pairs. A color-pair consists of a
 *       foreground color and a background color. A color-pair is
 *       initialized via init_pair(). After initialization, COLOR_PAIR(n)
 *       can be used like any other video attribute.
 *
 *       start_color() initializes eight basic colors (black, red, green,
 *       yellow, blue, magenta, cyan, and white), and two global
 *       variables; COLORS and COLOR_PAIRS (respectively defining the
 *       maximum number of colors and color-pairs the terminal is capable
 *       of displaying).
 *
 *       init_pair() changes the definition of a color-pair. It takes
 *       three arguments: the number of the color-pair to be redefined,
 *       and the new values of the foreground and background colors. The
 *       pair number must be between 0 and COLOR_PAIRS - 1, inclusive.
 *       The foreground and background must be between 0 and COLORS - 1,
 *       inclusive. If the color pair was previously initialized, the
 *       screen is refreshed, and all occurrences of that color-pair are
 *       changed to the new definition.
 *
 *       has_colors() indicates if the terminal supports, and can
 *       maniplulate color. It returns true or false.
 *
 *       can_change_color() indicates if the terminal has the capability
 *       to change the definition of its colors.
 *
 *       pair_content() is used to determine what the colors of a given
 *       color-pair consist of.
 *
 *       assume_default_colors() and use_default_colors() emulate the
 *       ncurses extensions of the same names.  assume_default_colors(f,
 *       b) is essentially the same as init_pair(0, f, b) (which isn't
 *       allowed); it redefines the default colors.  use_default_colors()
 *       allows the use of -1 as a foreground or background color with
 *       init_pair(), and calls assume_default_colors(-1, -1); -1
 *       represents the foreground or background color that the terminal
 *       had at startup.  If the environment variable PDC_ORIGINAL_COLORS
 *       is set at the time start_color() is called, that's equivalent to
 *       calling use_default_colors().
 *
 *       PDC_set_line_color() is used to set the color, globally, for
 *       the color of the lines drawn for the attributes: A_UNDERLINE,
 *       A_OVERLINE, A_LEFTLINE and A_RIGHTLINE. A value of -1 (the
 *       default) indicates that the current foreground color should be
 *       used.
 *
 *       NOTE: COLOR_PAIR() and PAIR_NUMBER() are implemented as macros.
 *
 * Return Value:
 *       All functions return OK on success and ERR on error, except for
 *       has_colors() and can_change_colors(), which return true or false.
 *
 * Portability                                X/Open    BSD    SYS V
 *       start_color                             Y       -      3.2
 *       init_pair                               Y       -      3.2
 *       init_color                              Y       -      3.2
 *       has_colors                              Y       -      3.2
 *       can_change_color                        Y       -      3.2
 *       color_content                           Y       -      3.2
 *       pair_content                            Y       -      3.2
 *       assume_default_colors                   -       -       -
 *       use_default_colors                      -       -       -
 *       PDC_set_line_color                      -       -       -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "curspriv.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
int COLORS = 0;
int COLOR_PAIRS = PDC_COLOR_PAIRS;

bool pdc_color_started = false;
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* pair_set[] tracks whether a pair has been set via init_pair() */

#ifndef CONFIG_PDCURSES_MULTITHREAD
static bool pair_set[PDC_COLOR_PAIRS];
static bool default_colors = false;
static short first_col = 0;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int start_color(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("start_color() - called\n"));

  if (SP->mono)
    {
      return ERR;
    }

  pdc_color_started = true;
  PDC_set_blink(false);         /* Also sets COLORS, to 8 or 16 */

#ifndef CONFIG_DISABLE_ENVIRON
  /* If the environment variable PDC_ORIGINAL_COLORS is set at the time
   * start_color() is called, that's equivalent to calling use_default_colors().
   */

  if (!default_colors && SP->orig_attr && getenv("PDC_ORIGINAL_COLORS"))
    {
      default_colors = true;
    }
#endif

  PDC_init_atrtab();
  memset(pair_set, 0, PDC_COLOR_PAIRS);
  return OK;
}

static void _normalize(short *fg, short *bg)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  if (*fg == -1)
    {
      *fg = SP->orig_attr ? SP->orig_fore : COLOR_WHITE;
    }

  if (*bg == -1)
    {
      *bg = SP->orig_attr ? SP->orig_back : COLOR_BLACK;
    }
}

int init_pair(short pair, short fg, short bg)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("init_pair() - called: pair %d fg %d bg %d\n", pair, fg, bg));

  if (!pdc_color_started || pair < 1 || pair >= COLOR_PAIRS ||
      fg < first_col || fg >= COLORS || bg < first_col || bg >= COLORS)
    {
      return ERR;
    }

  _normalize(&fg, &bg);

  /* To allow the PDC_PRESERVE_SCREEN option to work, we only reset curscr if
   * this call to init_pair() alters a color pair created by the user. */

  if (pair_set[pair])
    {
      short oldfg, oldbg;

      PDC_pair_content(pair, &oldfg, &oldbg);

      if (oldfg != fg || oldbg != bg)
        {
          curscr->_clear = true;
        }
    }

  PDC_init_pair(pair, fg, bg);
  pair_set[pair] = true;

  return OK;
}

bool has_colors(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("has_colors() - called\n"));

  return !(SP->mono);
}

int init_color(short color, short red, short green, short blue)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("init_color() - called\n"));

  if (color < 0 || color >= COLORS || !PDC_can_change_color() ||
      red < 0 || red > 1000 || green < 0 || green > 1000 ||
      blue < 0 || blue > 1000)
    {
      return ERR;
    }

  return PDC_init_color(color, red, green, blue);
}

int color_content(short color, short *red, short *green, short *blue)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("color_content() - called\n"));

  if (color < 0 || color >= COLORS || !red || !green || !blue)
    {
      return ERR;
    }

  if (PDC_can_change_color())
    {
      return PDC_color_content(color, red, green, blue);
    }
  else
    {
      /* Simulated values for platforms that don't support palette changing */

      short maxval = (color & 8) ? 1000 : 680;

      *red = (color & COLOR_RED) ? maxval : 0;
      *green = (color & COLOR_GREEN) ? maxval : 0;
      *blue = (color & COLOR_BLUE) ? maxval : 0;

      return OK;
    }
}

bool can_change_color(void)
{
  PDC_LOG(("can_change_color() - called\n"));

  return PDC_can_change_color();
}

int pair_content(short pair, short *fg, short *bg)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("pair_content() - called\n"));

  if (pair < 0 || pair >= COLOR_PAIRS || !fg || !bg)
    {
      return ERR;
    }

  return PDC_pair_content(pair, fg, bg);
}

int assume_default_colors(int f, int b)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("assume_default_colors() - called: f %d b %d\n", f, b));

  if (f < -1 || f >= COLORS || b < -1 || b >= COLORS)
    {
      return ERR;
    }

  if (pdc_color_started)
    {
      short fg, bg, oldfg, oldbg;

      fg = f;
      bg = b;

      _normalize(&fg, &bg);

      PDC_pair_content(0, &oldfg, &oldbg);

      if (oldfg != fg || oldbg != bg)
        {
          curscr->_clear = true;
        }

      PDC_init_pair(0, fg, bg);
    }

  return OK;
}

int use_default_colors(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("use_default_colors() - called\n"));

  default_colors = true;
  first_col = -1;

  return assume_default_colors(-1, -1);
}

int PDC_set_line_color(short color)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("PDC_set_line_color() - called: %d\n", color));

  if (color < -1 || color >= COLORS)
    {
      return ERR;
    }

  SP->line_color = color;
  return OK;
}

void PDC_init_atrtab(void)
{
  int i;
  short fg;
  short bg;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  if (pdc_color_started && !default_colors)
    {
      fg = COLOR_WHITE;
      bg = COLOR_BLACK;
    }
  else
    {
      fg = bg = -1;
    }

  _normalize(&fg, &bg);

  for (i = 0; i < PDC_COLOR_PAIRS; i++)
    {
      PDC_init_pair(i, fg, bg);
    }
}
