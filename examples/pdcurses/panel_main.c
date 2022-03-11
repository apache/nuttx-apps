/****************************************************************************
 * apps/examples/pdcurses/panel_main.c
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
 ****************************************************************************/

/****************************************************************************
 * Adapted from the original public domain pdcurses by Gregory Nutt
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>

#include "graphics/curses.h"
#include "graphics/panel.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static PANEL *p1;
static PANEL *p2;
static PANEL *p3;
static PANEL *p4;
static PANEL *p5;
static WINDOW *w4;
static WINDOW *w5;

static long nap_msec = 1;

static char *mod[] =
{
  "test ", "TEST ", "(**) ", "*()* ", "<--> ", "LAST "
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pflush(void)
{
  update_panels();
  doupdate();
}

static void backfill(void)
{
  int x;
  int y;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  erase();

  for (y = 0; y < LINES - 1; y++)
    {
      for (x = 0; x < COLS; x++)
        {
          printw("%d", (y + x) % 10);
        }
    }
}

static void wait_a_while(long msec)
{
  int c;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  if (msec != 1)
    {
      timeout(msec);
    }

  c = getch();
  if (c == 'q')
    {
      endwin();
      exit(1);
    }
}

static void saywhat(const char *text)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  mvprintw(LINES - 1, 0, "%-20.20s", text);
}

/* mkpanel - alloc a win and panel and associate them */

static PANEL *mkpanel(int rows, int cols, int tly, int tlx)
{
  WINDOW *win = newwin(rows, cols, tly, tlx);
  PANEL *pan = (PANEL *) 0;

  if (win)
    {
      pan = new_panel(win);

      if (!pan)
        {
          delwin(win);
        }
    }

  return pan;
}

static void rmpanel(PANEL *pan)
{
  WINDOW *win = pan->win;

  del_panel(pan);
  delwin(win);
}

static void fill_panel(PANEL *pan)
{
  WINDOW *win = pan->win;
  char num = *((char *)pan->user + 1);
  int x;
  int y;
  int maxx;
  int maxy;

  box(win, 0, 0);
  mvwprintw(win, 1, 1, "-pan%c-", num);
  getmaxyx(win, maxy, maxx);

  for (y = 2; y < maxy - 1; y++)
    {
      for (x = 1; x < maxx - 1; x++)
        {
          mvwaddch(win, y, x, num);
        }
    }
}

int main(int argc, FAR char *argv[])
{
  int itmp, y;

  if (argc > 1 && atol(argv[1]))
    {
      nap_msec = atol(argv[1]);
    }

  traceon();
  initscr();
  backfill();

  for (y = 0; y < 5; y++)
    {
      p1 = mkpanel(10, 10, 0, 0);
      set_panel_userptr(p1, "p1");

      p2 = mkpanel(14, 14, 5, 5);
      set_panel_userptr(p2, "p2");

      p3 = mkpanel(6, 8, 12, 12);
      set_panel_userptr(p3, "p3");

      p4 = mkpanel(10, 10, 10, 30);
      w4 = panel_window(p4);
      set_panel_userptr(p4, "p4");

      p5 = mkpanel(10, 10, 13, 37);
      w5 = panel_window(p5);
      set_panel_userptr(p5, "p5");

      fill_panel(p1);
      fill_panel(p2);
      fill_panel(p3);
      fill_panel(p4);
      fill_panel(p5);
      hide_panel(p4);
      hide_panel(p5);
      pflush();
      wait_a_while(nap_msec);

      saywhat("h3 s1 s2 s4 s5;");
      move_panel(p1, 0, 0);
      hide_panel(p3);
      show_panel(p1);
      show_panel(p2);
      show_panel(p4);
      show_panel(p5);
      pflush();
      wait_a_while(nap_msec);

      saywhat("s1;");
      show_panel(p1);
      pflush();
      wait_a_while(nap_msec);

      saywhat("s2;");
      show_panel(p2);
      pflush();
      wait_a_while(nap_msec);

      saywhat("m2;");
      move_panel(p2, 10, 10);
      pflush();
      wait_a_while(nap_msec);

      saywhat("s3;");
      show_panel(p3);
      pflush();
      wait_a_while(nap_msec);

      saywhat("m3;");
      move_panel(p3, 5, 5);
      pflush();
      wait_a_while(nap_msec);

      saywhat("b3;");
      bottom_panel(p3);
      pflush();
      wait_a_while(nap_msec);

      saywhat("s4;");
      show_panel(p4);
      pflush();
      wait_a_while(nap_msec);

      saywhat("s5;");
      show_panel(p5);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t3;");
      top_panel(p3);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t1;");
      top_panel(p1);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t2;");
      top_panel(p2);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t3;");
      top_panel(p3);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t4;");
      top_panel(p4);
      pflush();
      wait_a_while(nap_msec);

      for (itmp = 0; itmp < 6; itmp++)
        {
          saywhat("m4;");
          mvwaddstr(w4, 3, 1, mod[itmp]);
          move_panel(p4, 4, itmp * 10);
          mvwaddstr(w5, 4, 1, mod[itmp]);
          pflush();
          wait_a_while(nap_msec);

          saywhat("m5;");
          mvwaddstr(w4, 4, 1, mod[itmp]);
          move_panel(p5, 7, itmp * 10 + 6);
          mvwaddstr(w5, 3, 1, mod[itmp]);
          pflush();
          wait_a_while(nap_msec);
        }

      saywhat("m4;");
      move_panel(p4, 4, itmp * 10);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t5;");
      top_panel(p5);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t2;");
      top_panel(p2);
      pflush();
      wait_a_while(nap_msec);

      saywhat("t1;");
      top_panel(p1);
      pflush();
      wait_a_while(nap_msec);

      saywhat("d2;");
      rmpanel(p2);
      pflush();
      wait_a_while(nap_msec);

      saywhat("h3;");
      hide_panel(p3);
      pflush();
      wait_a_while(nap_msec);

      saywhat("d1;");
      rmpanel(p1);
      pflush();
      wait_a_while(nap_msec);

      saywhat("d4; ");
      rmpanel(p4);
      pflush();
      wait_a_while(nap_msec);

      saywhat("d5; ");
      rmpanel(p5);
      pflush();
      wait_a_while(nap_msec);

      if (nap_msec == 1)
        {
          break;
        }

      nap_msec = 100L;
    }

  endwin();
  return 0;
}
