/****************************************************************************
 * apps/graphics/pdcurses/pdc_panel.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Adapted from the original public domain pdcurses by Gregory Nutt
 ****************************************************************************/

/* Name: panel
 *
 * Synopsis:
 *       int bottom_panel(PANEL *pan);
 *       int del_panel(PANEL *pan);
 *       int hide_panel(PANEL *pan);
 *       int move_panel(PANEL *pan, int starty, int startx);
 *       PANEL *new_panel(WINDOW *win);
 *       PANEL *panel_above(const PANEL *pan);
 *       PANEL *panel_below(const PANEL *pan);
 *       int panel_hidden(const PANEL *pan);
 *       const void *panel_userptr(const PANEL *pan);
 *       WINDOW *panel_window(const PANEL *pan);
 *       int replace_panel(PANEL *pan, WINDOW *win);
 *       int set_panel_userptr(PANEL *pan, const void *uptr);
 *       int show_panel(PANEL *pan);
 *       int top_panel(PANEL *pan);
 *       void update_panels(void);
 *
 * Description:
 *       The panel library is built using the curses library, and any
 *       program using panels routines must call one of the curses
 *       initialization routines such as initscr(). A program using these
 *       routines must be linked with the panels and curses libraries.
 *       The header panel.h includes the header curses.h.
 *
 *       The panels package gives the applications programmer a way to
 *       have depth relationships between curses windows; a curses window
 *       is associated with every panel. The panels routines allow curses
 *       windows to overlap without making visible the overlapped
 *       portions of underlying windows. The initial curses window,
 *       stdscr, lies beneath all panels. The set of currently visible
 *       panels is the 'deck' of panels.
 *
 *       The panels package allows the applications programmer to create
 *       panels, fetch and set their associated windows, shuffle panels
 *       in the deck, and manipulate panels in other ways.
 *
 *       bottom_panel() places pan at the bottom of the deck. The size,
 *       location and contents of the panel are unchanged.
 *
 *       del_panel() deletes pan, but not its associated winwow.
 *
 *       hide_panel() removes a panel from the deck and thus hides it
 *       from view.
 *
 *       move_panel() moves the curses window associated with pan, so
 *       that its upper lefthand corner is at the supplied coordinates.
 *       (Do not use mvwin() on the window.)
 *
 *       new_panel() creates a new panel associated with win and returns
 *       the panel pointer. The new panel is placed at the top of the
 *       deck.
 *
 *       panel_above() returns a pointer to the panel in the deck above
 *       pan, or NULL if pan is the top panel. If the value of pan passed
 *       is NULL, this function returns a pointer to the bottom panel in
 *       the deck.
 *
 *       panel_below() returns a pointer to the panel in the deck below
 *       pan, or NULL if pan is the bottom panel. If the value of pan
 *       passed is NULL, this function returns a pointer to the top panel
 *       in the deck.
 *
 *       panel_hidden() returns OK if pan is hidden and ERR if it is not.
 *
 *       panel_userptr() - Each panel has a user pointer available for
 *       maintaining relevant information. This function returns a
 *       pointer to that information previously set up by
 *       set_panel_userptr().
 *
 *       panel_window() returns a pointer to the curses window associated
 *       with the panel.
 *
 *       replace_panel() replaces the current window of pan with win.
 *
 *       set_panel_userptr() - Each panel has a user pointer available
 *       for maintaining relevant information. This function sets the
 *       value of that information.
 *
 *       show_panel() makes a previously hidden panel visible and places
 *       it back in the deck on top.
 *
 *       top_panel() places pan on the top of the deck. The size,
 *       location and contents of the panel are unchanged.
 *
 *       update_panels() refreshes the virtual screen to reflect the
 *       depth relationships between the panels in the deck. The user
 *       must use doupdate() to refresh the physical screen.
 *
 * Return Value:
 *       Each routine that returns a pointer to an object returns NULL if
 *       an error occurs. Each panel routine that returns an integer,
 *       returns OK if it executes successfully and ERR if it does not.
 *
 * Portability                                X/Open    BSD    SYS V
 *       bottom_panel                            -       -       Y
 *       del_panel                               -       -       Y
 *       hide_panel                              -       -       Y
 *       move_panel                              -       -       Y
 *       new_panel                               -       -       Y
 *       panel_above                             -       -       Y
 *       panel_below                             -       -       Y
 *       panel_hidden                            -       -       Y
 *       panel_userptr                           -       -       Y
 *       panel_window                            -       -       Y
 *       replace_panel                           -       -       Y
 *       set_panel_userptr                       -       -       Y
 *       show_panel                              -       -       Y
 *       top_panel                               -       -       Y
 *       update_panels                           -       -       Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>

#include "graphics/panel.h"
#include "curspriv.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_PDCURSES_MULTITHREAD
PANEL *_bottom_panel = (PANEL *) 0;
PANEL *_top_panel = (PANEL *) 0;
PANEL _stdscr_pseudo_panel = { (WINDOW *) 0 };
#else
typedef struct panel_ctx_s
{
  PANEL         *bottom_panel;
  PANEL         *top_panel;
  PANEL         stdscr_pseudo_panel;
} PANEL_CTX;
#endif

#ifdef CONFIG_PDCURSES_MULTITHREAD
void *pdc_alloc_panel_ctx()
{
  return (void *) zalloc(sizeof(struct panel_ctx_s));
}
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_PDCURSES_PANEL_DEBUG
static void dpanel(char *text, PANEL *pan)
{
  PDC_LOG(("%s id=%s b=%s a=%s y=%d x=%d", text, pan->user,
           pan->below ? pan->below->user : "--",
           pan->above ? pan->above->user : "--", pan->wstarty, pan->wstartx));
}

static void dstack(char *fmt, int num, PANEL *pan)
{
  char s80[80];

  sprintf(s80, fmt, num, pan);
  PDC_LOG(("%s b=%s t=%s", s80, _bottom_panel ? _bottom_panel->user : "--",
           _top_panel ? _top_panel->user : "--"));

  if (pan)
    {
      PDC_LOG(("pan id=%s", pan->user));
    }

  pan = _bottom_panel;

  while (pan)
    {
      dpanel("stk", pan);
      pan = pan->above;
    }
}

/* Debugging hook for wnoutrefresh */

static void dwnoutrefresh(PANEL *pan)
{
  dpanel("wnoutrefresh", pan);
  wnoutrefresh(pan->win);
}

static void dtouchwin(PANEL *pan)
{
  dpanel("dtouchwin", pan);
  touchwin(pan->win);
}

static void dtouchline(PANEL *pan, int start, int count)
{
  char s80[80];

  sprintf(s80, "dtouchline s=%d c=%d", start, count);
  dpanel(s80, pan);
  touchline(pan->win, start, count);
}

#else
#  define dpanel(text, pan)
#  define dstack(fmt, num, pan)
#  define dwnoutrefresh(pan) wnoutrefresh((pan)->win)
#  define dtouchwin(pan) touchwin((pan)->win)
#  define dtouchline(pan, start, count) touchline((pan)->win, start, count)
#endif

static bool _panels_overlapped(PANEL * pan1, PANEL * pan2)
{
  if (!pan1 || !pan2)
    {
      return false;
    }

  return ((pan1->wstarty >= pan2->wstarty && pan1->wstarty < pan2->wendy)
          || (pan2->wstarty >= pan1->wstarty && pan2->wstarty < pan1->wendy))
    && ((pan1->wstartx >= pan2->wstartx && pan1->wstartx < pan2->wendx)
        || (pan2->wstartx >= pan1->wstartx && pan2->wstartx < pan1->wendx));
}

static void _free_obscure(PANEL *pan)
{
  PANELOBS *tobs = pan->obscure;  /* "this" one */
  PANELOBS *nobs;                 /* "next" one */

  while (tobs)
    {
      nobs = tobs->above;
      free((char *)tobs);
      tobs = nobs;
    }

  pan->obscure = (PANELOBS *) 0;
}

static void _override(PANEL *pan, int show)
{
  int y;
  PANEL *pan2;
  PANELOBS *tobs = pan->obscure;        /* "this" one */
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  if (show == 1)
    {
      dtouchwin(pan);
    }
  else if (!show)
    {
      dtouchwin(pan);
      dtouchwin(&_stdscr_pseudo_panel);
    }
  else if (show == -1)
    {
      while (tobs && (tobs->pan != pan))
        {
          tobs = tobs->above;
        }
    }

  while (tobs)
    {
      if ((pan2 = tobs->pan) != pan)
        {
          for (y = pan->wstarty; y < pan->wendy; y++)
            {
              if ((y >= pan2->wstarty) && (y < pan2->wendy) &&
                  ((is_linetouched(pan->win, y - pan->wstarty)) ||
                   (is_linetouched(stdscr, y))))
                {
                  dtouchline(pan2, y - pan2->wstarty, 1);
                }
            }
        }

      tobs = tobs->above;
    }
}

static void _calculate_obscure(void)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PANEL *pan, *pan2;
  PANELOBS *tobs;               /* "this" one */
  PANELOBS *lobs;               /* last one */

  pan = _bottom_panel;

  while (pan)
    {
      if (pan->obscure)
        {
          _free_obscure(pan);
        }

      lobs = (PANELOBS *) 0;
      pan2 = _bottom_panel;

      while (pan2)
        {
          if (_panels_overlapped(pan, pan2))
            {
              if ((tobs = malloc(sizeof(PANELOBS))) == NULL)
                {
                  return;
                }

              tobs->pan = pan2;
              dpanel("obscured", pan2);
              tobs->above = (PANELOBS *) 0;

              if (lobs)
                {
                  lobs->above = tobs;
                }
              else
                {
                  pan->obscure = tobs;
                }

              lobs = tobs;
            }

          pan2 = pan2->above;
        }

      _override(pan, 1);
      pan = pan->above;
    }
}

/* Check to see if panel is in the stack */

static bool _panel_is_linked(const PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PANEL *pan2 = _bottom_panel;

  while (pan2)
    {
      if (pan2 == pan)
        {
          return true;
        }

      pan2 = pan2->above;
    }

  return false;
}

/* Link panel into stack at top */

static void _panel_link_top(PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
#ifdef CONFIG_PDCURSES_PANEL_DEBUG
  dstack("<lt%d>", 1, pan);
  if (_panel_is_linked(pan))
    {
      return;
    }
#endif

  pan->above = (PANEL *) 0;
  pan->below = (PANEL *) 0;

  if (_top_panel)
    {
      _top_panel->above = pan;
      pan->below = _top_panel;
    }

  _top_panel = pan;

  if (!_bottom_panel)
    {
      _bottom_panel = pan;
    }

  _calculate_obscure();
  dstack("<lt%d>", 9, pan);
}

/* Link panel into stack at bottom */

static void _panel_link_bottom(PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
#ifdef CONFIG_PDCURSES_PANEL_DEBUG
  dstack("<lb%d>", 1, pan);
  if (_panel_is_linked(pan))
    {
      return;
    }
#endif

  pan->above = (PANEL *)0;
  pan->below = (PANEL *)0;

  if (_bottom_panel)
    {
      _bottom_panel->below = pan;
      pan->above = _bottom_panel;
    }

  _bottom_panel = pan;

  if (!_top_panel)
    {
      _top_panel = pan;
    }

  _calculate_obscure();
  dstack("<lb%d>", 9, pan);
}

static void _panel_unlink(PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PANEL *prev;
  PANEL *next;

#ifdef CONFIG_PDCURSES_PANEL_DEBUG
  dstack("<u%d>", 1, pan);
  if (!_panel_is_linked(pan))
    {
      return;
    }
#endif

  _override(pan, 0);
  _free_obscure(pan);

  prev = pan->below;
  next = pan->above;

  /* If non-zero, we will not update the list head */

  if (prev)
    {
      prev->above = next;
      if (next)
        {
          next->below = prev;
        }
    }
  else if (next)
    {
      next->below = prev;
    }

  if (pan == _bottom_panel)
    {
      _bottom_panel = next;
    }

  if (pan == _top_panel)
    {
      _top_panel = prev;
    }

  _calculate_obscure();

  pan->above = (PANEL *) 0;
  pan->below = (PANEL *) 0;
  dstack("<u%d>", 9, pan);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bottom_panel(PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  if (!pan)
    {
      return ERR;
    }

  if (pan == _bottom_panel)
    {
      return OK;
    }

  if (_panel_is_linked(pan))
    {
      hide_panel(pan);
    }

  _panel_link_bottom(pan);
  return OK;
}

int del_panel(PANEL *pan)
{
  if (pan)
    {
      if (_panel_is_linked(pan))
        {
          hide_panel(pan);
        }

      free((char *)pan);
      return OK;
    }

  return ERR;
}

int hide_panel(PANEL *pan)
{
  if (!pan)
    {
      return ERR;
    }

  if (!_panel_is_linked(pan))
    {
      pan->above = (PANEL *) 0;
      pan->below = (PANEL *) 0;
      return ERR;
    }

  _panel_unlink(pan);
  return OK;
}

int move_panel(PANEL *pan, int starty, int startx)
{
  WINDOW *win;
  int maxx;
  int maxy;

  if (!pan)
    {
      return ERR;
    }

  if (_panel_is_linked(pan))
    {
      _override(pan, 0);
    }

  win = pan->win;

  if (mvwin(win, starty, startx) == ERR)
    {
      return ERR;
    }

  getbegyx(win, pan->wstarty, pan->wstartx);
  getmaxyx(win, maxy, maxx);
  pan->wendy = pan->wstarty + maxy;
  pan->wendx = pan->wstartx + maxx;

  if (_panel_is_linked(pan))
    {
      _calculate_obscure();
    }

  return OK;
}

PANEL *new_panel(WINDOW *win)
{
  PANEL *pan = malloc(sizeof(PANEL));
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  if (!_stdscr_pseudo_panel.win)
    {
      _stdscr_pseudo_panel.win = stdscr;
      _stdscr_pseudo_panel.wstarty = 0;
      _stdscr_pseudo_panel.wstartx = 0;
      _stdscr_pseudo_panel.wendy = LINES;
      _stdscr_pseudo_panel.wendx = COLS;
      _stdscr_pseudo_panel.user = "stdscr";
      _stdscr_pseudo_panel.obscure = (PANELOBS *) 0;
    }

  if (pan)
    {
      int maxx;
      int maxy;

      pan->win = win;
      pan->above = (PANEL *) 0;
      pan->below = (PANEL *) 0;
      getbegyx(win, pan->wstarty, pan->wstartx);
      getmaxyx(win, maxy, maxx);
      pan->wendy = pan->wstarty + maxy;
      pan->wendx = pan->wstartx + maxx;
#ifdef CONFIG_PDCURSES_PANEL_DEBUG
      pan->user = "new";
#else
      pan->user = (char *)0;
#endif
      pan->obscure = (PANELOBS *) 0;
      show_panel(pan);
    }

  return pan;
}

PANEL *panel_above(const PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  return pan ? pan->above : _bottom_panel;
}

PANEL *panel_below(const PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  return pan ? pan->below : _top_panel;
}

int panel_hidden(const PANEL *pan)
{
  if (!pan)
    {
      return ERR;
    }

  return _panel_is_linked(pan) ? ERR : OK;
}

const void *panel_userptr(const PANEL *pan)
{
  return pan ? pan->user : NULL;
}

WINDOW *panel_window(const PANEL *pan)
{
  PDC_LOG(("panel_window() - called\n"));

  return pan->win;
}

int replace_panel(PANEL *pan, WINDOW *win)
{
  int maxy, maxx;

  if (!pan)
    {
      return ERR;
    }

  if (_panel_is_linked(pan))
    {
      _override(pan, 0);
    }

  pan->win = win;
  getbegyx(win, pan->wstarty, pan->wstartx);
  getmaxyx(win, maxy, maxx);
  pan->wendy = pan->wstarty + maxy;
  pan->wendx = pan->wstartx + maxx;

  if (_panel_is_linked(pan))
    {
      _calculate_obscure();
    }

  return OK;
}

int set_panel_userptr(PANEL *pan, const void *uptr)
{
  if (!pan)
    {
      return ERR;
    }

  pan->user = uptr;
  return OK;
}

int show_panel(PANEL *pan)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  if (!pan)
    {
      return ERR;
    }

  if (pan == _top_panel)
    {
      return OK;
    }

  if (_panel_is_linked(pan))
    {
      hide_panel(pan);
    }

  _panel_link_top(pan);
  return OK;
}

int top_panel(PANEL *pan)
{
  return show_panel(pan);
}

void update_panels(void)
{
  PANEL *pan;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("update_panels() - called\n"));

  pan = _bottom_panel;

  while (pan)
    {
      _override(pan, -1);
      pan = pan->above;
    }

  if (is_wintouched(stdscr))
    {
      dwnoutrefresh(&_stdscr_pseudo_panel);
    }

  pan = _bottom_panel;

  while (pan)
    {
      if (is_wintouched(pan->win) || !pan->above)
        {
          dwnoutrefresh(pan);
        }

      pan = pan->above;
    }
}
