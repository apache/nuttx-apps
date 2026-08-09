/****************************************************************************
 * apps/examples/nanoxterm/nxterm.c
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
 * nxterm - terminal emulator for Nano-X
 *
 * Ported from the Microwindows nxterm demo (src/demos/nanox/nxterm.c):
 *
 * (C) 1994,95,96 by Torsten Scherer (TeSche)
 * itschere@techfak.uni-bielefeld.de
 *
 * - quite some changes for W1R1
 * - yet more changes for W1R2
 *
 * TeSche 01/96:
 * - supports W_ICON & W_CLOSE
 * - supports /etc/utmp logging for SunOS4 and Linux
 * - supports catching of console output for SunOS4
 * Phx 02-06/96:
 * - supports NetBSD-Amiga
 * Eero 11/97:
 * - unsetenv(DISPLAY), setenv(LINES, COLUMNS).
 * - Add new text modes (you need to use terminfo...).
 * Eero 2/98:
 * - Implemented fg/bgcolor setting.  With monochrome server it changes
 *   bgmode variable, which tells in which mode to draw to screen
 *   (M_CLEAR/M_DRAW) and affects F_REVERSE settings.
 * - Added a couple of checks.
 * 1/23/10 ghaerr
 * - added support for UNIX98 ptys (Linux default)
 * - added ngterm terminal type and environment variable
 *
 * TODO:
 * - Allocate and set sensible window palette for fg/bg color setting.
 * - add scroll-region ('cs') command.  Fairly many programs
 *   can take advantage of that.
 * - Add xterm like mouse event to terminfo key event conversion... :)
 *
 * Georg 16th Nov 2013:
 * - Added ANSI emulation with color support and scrolling region support
 *   tested the emulation with the Nano editor.
 *   made ANSI the default, select vt52 with -5 command line switch
 * - 8th Dec 2013: improved ANSI emulation and Nano support
 * - 15th Dec 2013: added reading program from command line for Linux
 *   use double quotes when calling from a script e.g.:
 *   bin/nano-X & bin/nxterm "ls -l *.sh >test.log" & sleep 10000
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>

#define MWINCLUDECOLORS
#include "nano-X.h"
#include "uni_std.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define GR_COLOR_WHITESMOKE         MWRGB(245, 245, 245)
#define GR_COLOR_GAINSBORO          MWRGB(220, 220, 220)
#define GR_COLOR_ANTIQUEWHITE       MWRGB(250, 235, 215)
#define GR_COLOR_BLANCHEDALMOND     MWRGB(255, 235, 205)
#define GR_COLOR_LAVENDER           MWRGB(230, 230, 250)
#define GR_COLOR_WHITE              MWRGB(255, 255, 255)
#define _XOPEN_SOURCE               600
#define stdcol          80
#define stdrow          50
#define KBDBUF          10240
#define TITLE           "nxterm"
#define stdforeground   BLACK
#define stdbackground   LTGRAY
#define LINEBUF         stdcol
#define fonh    fi.height
#define fonw    fi.maxwidth
#define ANSI    0
#define VT52    1
#define _   ((unsigned)0)
#define X   ((unsigned)1)
#define MASK7(a, b, c, d, e, f,                                            \
              g)  (((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) + e) * \
                       2) + f) * 2) + g) << 9)
/* NuttX has no filesystem shell binary, so the pty child runs an NSH
 * instance directly instead of exec'ing /bin/sh.
 */

extern int nsh_consolemain(int argc, char *argv[]);

/* NuttX fork() does not preserve the child's stack-local variables (the
 * child may only exec/_exit per POSIX), so the pty name must live in
 * static storage to be readable by the forked child.
 */

static char ptyname[50];

/* globals
 */

GR_WINDOW_ID w1;
GR_GC_ID gc1;
GR_FONT_ID regFont;

/* GR_FONT_ID boldFont; */

GR_SCREEN_INFO si;
GR_FONT_INFO fi;

GR_WINDOW_INFO wi;
GR_GC_INFO gi;
GR_BOOL havefocus = GR_FALSE;
pid_t pid;
short winw;
short winh;
int termfd;
int visualbell;
int fgcolor[12] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11
};

int bgcolor[12] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11
};

int scrolledFlag;

int scrolltop;
int scrollbottom;
int ReverseMode = 0;
int semicolonflag = 0;
int nobracket = 0;
int roundbracket = 0;
int savex;
int savey;
char *startprogram;

/* the terminal code, almost like VT52 */

int termtype = ANSI;
int bgmode;
int escstate;
int curx;
int cury;
int curon;
int curvis;
int saved_x;
int saved_y;
int wrap;
int style;
int col;
int row;
int colmask = 0x7f;
int rowmask = 0x7f;
int sbufcnt = 0;
int sbufx;
int sbufy;
char lineBuffer[LINEBUF + 1];
char *sbuf = lineBuffer;

int term_init(void);
/****************************************************************************
 * Private Functions
 ****************************************************************************/

void sflush(void);
void lineRedraw(void);
void sadd(char c);
void show_cursor(void);
void draw_cursor(void);
void hide_cursor(void);
void vscrollup(int lines);
void vscrolldown(int lines);
void esc5(unsigned char c);
void esc4(unsigned char c);
void esc3(unsigned char c);
void esc2(unsigned char c);
void esc1(unsigned char c);
void esc0(unsigned char c);
void esc100(unsigned char c);
void printc(unsigned char c);
void init(void);
void term(void);
void usage(void);
int do_special_key(unsigned char *buffer, int key, int modifiers);
int do_special_key_ansi(unsigned char *buffer, int key, int modifiers);
void pos_xaxis(int c);
void pos_yaxis(int c);
void rendition(int escvalue);

/* **************************************************************************/

void sflush(void)
{
  if (sbufcnt)
    {
      GrText(w1, gc1, sbufx * fonw, sbufy * fonh, sbuf, sbufcnt, GR_TFTOP);
      sbufcnt = 0;
    }
}

void lineRedraw(void)
{
  GrSetGCForeground(gc1, gi.background);
  GrFillRect(w1, gc1, curx * fonw, cury * fonh, (col - curx) * fonw, fonh);
  GrSetGCForeground(gc1, gi.foreground);

  if (sbufcnt)
    {
      sbuf[sbufcnt] = 0;
      GrText(w1, gc1, sbufx * fonw, sbufy * fonh, sbuf, sbufcnt, GR_TFTOP);
    }
}

void sadd(char c)
{
  if (sbufcnt == LINEBUF)
    {
      sflush();
    }

  if (!sbufcnt)
    {
      sbufx = curx;
      sbufy = cury;
    }

  sbuf[sbufcnt++] = c;
}

void show_cursor(void)
{
  GrSetGCMode(gc1, GR_MODE_XOR);
  GrSetGCForeground(gc1, WHITE);
  GrFillRect(w1, gc1, curx * fonw, cury * fonh + 1, fonw, fonh - 1);
  GrSetGCForeground(gc1, gi.foreground);
  GrSetGCMode(gc1, GR_MODE_COPY);
}

void draw_cursor(void)
{
  if (curon)
    {
      if (!curvis)
        {
          curvis = 1;
          show_cursor();
        }
    }
}

void hide_cursor(void)
{
  if (curvis)
    {
      curvis = 0;
      show_cursor();
    }
}

/* VVV */

void vscrollup(int lines)
{
  hide_cursor();
  GrCopyArea(w1, gc1, 0, scrolltop * fonh, winw,
    (scrollbottom - (scrolltop - 1) - lines - 1) * fonh, w1, 0,
    (scrolltop + lines) * fonh, MWROP_COPY);
  GrSetGCForeground(gc1, gi.background);
  GrFillRect(w1, gc1, 0, (scrollbottom - lines) * fonh, winw, lines * fonh);
  GrSetGCForeground(gc1, gi.foreground);
}

void vscrolldown(int lines)
{
  hide_cursor();

  /* FIXME add for loop */

  GrCopyArea(w1, gc1, 0, (scrolltop + lines) * fonh, winw,
    (scrollbottom - scrolltop - lines) * fonh, w1, 0, scrolltop * fonh,
    MWROP_COPY);
  GrSetGCForeground(gc1, gi.background);
  GrFillRect(w1, gc1, 0, scrolltop * fonh, winw, lines * fonh);
  GrSetGCForeground(gc1, gi.foreground);
}

void esc5(unsigned char c)
{
  GrSetGCBackground(gc1, c);
  GrGetGCInfo(gc1, &gi);
  escstate = 0;
}

void esc4(unsigned char c)
{
  GrSetGCForeground(gc1, c);
  GrGetGCInfo(gc1, &gi);
  escstate = 0;
}

void esc3(unsigned char c)
{
  curx = (c - 32) & colmask;
  if (curx >= col)
    {
      curx = col - 1;
    }
  else if (curx < 0)
    {
      curx = 0;
    }

  escstate = 0;
}

void esc2(unsigned char c)
{
  cury = (c - 32) & rowmask;
  if (cury >= row)
    {
      cury = row - 1;
    }
  else if (cury < 0)
    {
      cury = 0;
    }

  escstate = 3;
}

void esc1(unsigned char c)
{
  escstate = 0;

  /* detect ANSI / VT100 codes */

  if (c == '[')
    {
      escstate = 10;
      return;
    }

  if (c == '(')
    {
      escstate = 10;
      roundbracket = 1;
      return;
    }

  if (termtype == ANSI)
    {
      /* no bracket code - just ESC+letter
       * so read no further char just do esc100 and terminate
       * ESC state to read new sequence or unescaped chars.
       */

      nobracket = 1;
      esc100(c);
      escstate = 0;
      return;
    }

  /* now vt52 codes */

  switch (c)
    {
      case 'A':
        {
          hide_cursor();
          if ((cury -= 1) < 0)
            {
              cury = 0;
            }
          break;
        }

      case 'B':
        {
          hide_cursor();
          if ((cury += 1) >= row)
            {
              cury = row - 1;
            }
          break;
        }

      case 'C':
        {
          hide_cursor();
          if ((curx += 1) >= col)
            {
              curx = col - 1;
            }
          break;
        }

      case 'D':
        {
          hide_cursor();
          if ((curx -= 1) < 0)
            {
              curx = 0;
            }
          break;
        }

      case 'E':
        {
          GrClearWindow(w1, 0);
          curx = 0;
          cury = 0;
          break;
        }

      case 'H':
        {
          curx = 0;
          cury = 0;
          break;
        }

      case 'I':
        {
          scrolledFlag = 1;
          if ((cury -= 1) < 0)
            {
              cury = 0;
              vscrollup(1);
            }
          break;
        }

      case 'J':
        {
          if (cury < row - 1)
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, (cury + 1) * fonh, winw,
                (row - 1 - cury) * fonh);
              GrSetGCForeground(gc1, gi.foreground);
            }

          GrSetGCForeground(gc1, gi.background);
          GrFillRect(w1, gc1, curx * fonw, cury * fonh, (col - curx) * fonw,
            fonh);
          GrSetGCForeground(gc1, gi.foreground);
          break;
        }

      case 'K':
        {
          GrSetGCForeground(gc1, gi.background);
          GrFillRect(w1, gc1, curx * fonw, cury * fonh, (col - curx) * fonw,
            fonh);
          GrSetGCForeground(gc1, gi.foreground);
          break;
        }

      case 'L':
        {
          if (cury < row - 1)
            {
              vscrollup(1);
            }

          curx = 0;
          break;
        }

      case 'M':
        {
          if (cury < row - 1)
            {
              vscrollup(1);
            }

          curx = 0;
          break;
        }

      case 'Y':
        {
          escstate = 2;
          break;
        }

      case 'b':
        {
          escstate = 4;
          break;
        }

      case 'c':
        {
          escstate = 5;
          break;
        }

      case 'd':

        {
          if (cury > 0)
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, 0, winw, cury * fonh);
              GrSetGCForeground(gc1, gi.foreground);
            }

          if (curx > 0)
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, cury * fonh, curx * fonw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
            }
          break;
        }

      case 'e':
        {
          curon = 1;
          break;
        }

      case 'f':
        {
          curon = 0;
          break;
        }

      case 'j':
        {
          saved_x = curx;
          saved_y = cury;
          break;
        }

      case 'k':
        {
          curx = saved_x;
          cury = saved_y;
          break;
        }

      case 'l':
        {
          GrSetGCForeground(gc1, gi.background);
          GrFillRect(w1, gc1, 0, cury * fonh, winw, fonh);
          GrSetGCForeground(gc1, gi.foreground);
          curx = 0;
          break;
        }

      case 'o':
        {
          if (curx > 0)
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, cury * fonh, curx * fonw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
            }
          break;
        }

      case 'p':
        {
          if (!ReverseMode)
            {
              GrSetGCForeground(gc1, gi.background);
              GrSetGCBackground(gc1, gi.foreground);
              ReverseMode = 1;
            }
          break;
        }

      case 'q':
        {
          if (ReverseMode)
            {
              GrSetGCForeground(gc1, gi.foreground);
              GrSetGCBackground(gc1, gi.background);
              ReverseMode = 0;
            }
          break;
        }

      case 'v':
        {
          wrap = 1;
          break;
        }

      case 'w':
        {
          wrap = 0;
          break;
        }

  /* and these are the extensions not in VT52 */

      case 'G':
        {
          break;
        }

      case 'g':

        /* GrSetGCFont(gc1, boldFont); */

        {
          break;
        }

      case 'h':

        {
          break;
        }

      case 'i':
        {
          break;
        }

      /* j, k and l are already used */

      case 'm':
        {
          break;
        }

  /* these ones aren't yet on the termcap entries */

      case 'n':
        {
          break;
        }

      /* o, p and q are already used */

      case 'r':
        {
          break;
        }

      case 's':
        {
          break;
        }

      case 't':
        {
          break;
        }

      default:
        {
          break;
        }
    }
}

void pos_xaxis(int c)
{
  curx = c & colmask;
  if (curx >= col)
    {
      curx = col - 1;
    }
  else if (curx < 0)
    {
      curx = 0;
    }
}

void pos_yaxis(int c)
{
  cury = c & rowmask;

  if (cury >= scrollbottom)
    {
      cury = scrollbottom - 1;
    }
  else if (cury < scrolltop)
    {
      cury = scrolltop - 1;
    }
}

void rendition(int escvalue)
{
  if (escvalue == 0)
    {
      GrSetGCForeground(gc1, stdforeground);
      GrSetGCBackground(gc1, stdbackground);
      ReverseMode = 0;
    }
  else if (escvalue == 7)
    {
      if (!ReverseMode)
        {
          GrSetGCForeground(gc1, gi.background);
          GrSetGCBackground(gc1, gi.foreground);
          ReverseMode = 1;
        }
    }
  else if (escvalue == 27)
    {
      if (ReverseMode)
        {
          GrSetGCForeground(gc1, gi.foreground);
          GrSetGCBackground(gc1, gi.background);
          ReverseMode = 0;
        }
    }
  else if ((escvalue > 29) && (escvalue < 38))
    {
      switch (escvalue)
        {
          case 30:
            {
              GrSetGCForeground(gc1, BLACK);
              break;
            }

          case 31:
            {
              GrSetGCForeground(gc1, RED);
              break;
            }

          case 32:
            {
              GrSetGCForeground(gc1, GREEN);
              break;
            }

          case 33:
            {
              GrSetGCForeground(gc1, BROWN);
              break;
            }

          case 34:
            {
              GrSetGCForeground(gc1, BLUE);
              break;
            }

          case 35:
            {
              GrSetGCForeground(gc1, MAGENTA);
              break;
            }

          case 36:
            {
              GrSetGCForeground(gc1, CYAN);
              break;
            }

          case 37:
            {
              GrSetGCForeground(gc1, WHITE);
              break;
            }

          case 39:
            {
              GrSetGCForeground(gc1, stdforeground);
              break;
            }
        }
    }
  else if ((escvalue > 39) && (escvalue < 49))
    {
      switch (escvalue)
        {
          case 40:
            {
              GrSetGCBackground(gc1, BLACK);
              break;
            }

          case 41:
            {
              GrSetGCBackground(gc1, RED);
              break;
            }

          case 42:
            {
              GrSetGCBackground(gc1, GREEN);
              break;
            }

          case 43:
            {
              GrSetGCBackground(gc1, BROWN);
              break;
            }

          case 44:
            {
              GrSetGCBackground(gc1, BLUE);
              break;
            }

          case 45:
            {
              GrSetGCBackground(gc1, MAGENTA);
              break;
            }

          case 46:
            {
              GrSetGCBackground(gc1, CYAN);
              break;
            }

          case 47:
            {
              GrSetGCBackground(gc1, WHITE);
              break;
            }

          case 49:
            {
              GrSetGCBackground(gc1, stdbackground);
              break;
            }
        }
    }
}

void esc100(unsigned char c)
{
  int y;
  int yy;
  char buf[32];

  /* leave escstate=10 till done.  This states gets this function called. */

  static int escvalue1;
  static int escvalue2;
  static int escvalue3;
  static char valuebuffer[3];

  valuebuffer[2] = '\0';

  if (c == '?')
    {
      return;
    }

  if (nobracket == 1)
    {
      /* fall through */
    }
  else if (roundbracket == 1)

  /* fall through */

    {
    }
  else if ((c > 0x2F) && (c < ':'))
    {
      if (valuebuffer[0] != '\0')
        {
          valuebuffer[1] = c;
        }
      else
        {
          valuebuffer[0] = c;
        }

      return;
    }
  else if (c == ';')
    {
      if (semicolonflag == 0)
        {
          escvalue1 = atoi(valuebuffer);
          semicolonflag++;
          valuebuffer[0] = '\0';
          valuebuffer[1] = '\0';
          return;
        }
      else if (semicolonflag == 1)
        {
          escvalue2 = atoi(valuebuffer);
          semicolonflag++;
          valuebuffer[0] = '\0';
          valuebuffer[1] = '\0';
          return;
        }
    }
  else if (((c > '@') && (c < '[')) || ((c > 0x60) && (c < '{'))) /* is it a
                                                                   * letter? */
    {
      if (semicolonflag == 0)
        {
          escvalue1 = atoi(valuebuffer);
          escvalue2 = 0;
          escvalue3 = 0;
        }
      else if (semicolonflag == 1)
        {
          escvalue2 = atoi(valuebuffer);
          escvalue3 = 0;
        }
      else if (semicolonflag == 2)
        {
          escvalue3 = atoi(valuebuffer);
        }

      escstate = 0;
      valuebuffer[0] = '\0';
      valuebuffer[1] = '\0';
    }
  else
    {
      return;
    }

  /* fall through now if letter received */

  /* now interpret the ESC sequence */

/* the cursor positions: cury,curx are zero based, so 0,0 is home position
 *  also if command asks to position to 5 this has to be 4
 *  y is the row/line position, x is the column position
 *  fonh = fi.height = height of character or line in pixel
 *  fonw = fi.maxwidth = width of character in pixel
 *  winw = width of line in pixel
 *  winh = height of screen in pixel
 *  col = number of columns for current screen width
 *  row = number of lines for current screen height
 *  scrolltop, scrollbottom = upper and lower scroll region limit in
 * lines/rows
 */

  if (nobracket == 1)
    {
      if (c == '8')

      /* HOME will reduce by one below, so add here! */

        {
          escvalue1 = savey + 1;
          escvalue2 = savex + 1;
          c = 'H';
        }
      else if (c == '7')
        {
          savex = curx;
          savey = cury;
          c = '!';
        }
      else if (c == 'M')    /* reverse index, same as cursor up but scroll
                             * display at top */
        {
          if (cury <= scrolltop)
            {
              escvalue1 = 1;
              c = 'L';
            }
          else
            {
              cury--;
              pos_yaxis(cury);
              c = '!';
            }
        }
      else if (c == 'D')    /* index, same as cursor down but scroll display at
                             * bottom */
        {
          if ((cury + 1) > scrollbottom)
            {
              escvalue1 = 1;
              c = 'M';
            }
          else
            {
              cury++;
              pos_yaxis(cury);
              c = '!';
            }
        }
    }

  nobracket = 0;

  if (roundbracket == 1)
    {
      c = 0;
      roundbracket = 0;
    }

  switch (c)
    {
      case 'A':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          hide_cursor();
          if ((cury -= escvalue1) < 0)
            {
              /* cury = 0; */

              cury = scrolltop - 1;
            }
          break;
        }

      case 'B':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          hide_cursor();
          if ((cury += escvalue1) >= row)
            {
              /* cury = row - 1; */

              cury = scrollbottom - 1;
            }
          break;
        }

      case 'C':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          hide_cursor();
          if ((curx += escvalue1) >= col)
            {
              curx = col - 1;
            }
          break;
        }

      case 'D':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          hide_cursor();
          if ((curx -= escvalue1) < 0)
            {
              curx = 0;
            }
          break;
        }

      case 'J':
        {
          if (escvalue1 == 0)               /* erase from current cursor to end
                                             * of page/scrollbottom */
            {
              if (cury < scrollbottom - 1)
                {
                  GrSetGCForeground(gc1, gi.background);
                  GrFillRect(w1, gc1, 0, (cury + 1) * fonh, winw,
                    (scrollbottom - 1 - cury) * fonh);
                  GrSetGCForeground(gc1, gi.foreground);
                }

              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, curx * fonw, cury * fonh,
                (col - curx) * fonw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
              break;
            }
          else if (escvalue1 == 1)          /* erase from home/scrolltop to
                                             * cursor */
            {
              if (cury < scrollbottom - 1)  /* erase area from top to line
                                             * above current line */
                {
                  GrSetGCForeground(gc1, gi.background);
                  GrFillRect(w1, gc1, 0, scrolltop, winw, (cury + 1) * fonh);
                  GrSetGCForeground(gc1, gi.foreground);
                }

              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, cury * fonh, curx * fonw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
              break;
            }
          else if (escvalue1 == 2) /* erase entire page - leave cursor
                                    * untouched */

          /* GrClearWindow(w1, 0);
           * erase just the scrolling area
           */

            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, scrolltop, winw, (scrollbottom) * fonh);
              GrSetGCForeground(gc1, gi.foreground);
              break;
            }
        }

      case 'K':
        {
          if (escvalue1 == 0)
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, curx * fonw, cury * fonh,
                (col - curx) * fonw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
              break;
            }
          else if (escvalue1 == 1)
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, cury * fonh, curx * fonw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
              break;
            }
          else if (escvalue1 == 2) /* erase entire line - leave cursor
                                    * untouched */
            {
              GrSetGCForeground(gc1, gi.background);
              GrFillRect(w1, gc1, 0, cury * fonh, winw, fonh);
              GrSetGCForeground(gc1, gi.foreground);
              break;
            }
        }

      case 'P':/* erase number of characters after and including the cursor
                * and move remaining to this position */
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          GrSetGCForeground(gc1, gi.background);

          /* copy remaining chars on line to cursor position */

          GrCopyArea(w1, gc1, curx * fonw, cury * fonh,
            (col - curx - escvalue1) * fonw, fonh, w1,
            (curx + escvalue1) * fonw, cury * fonh, MWROP_COPY);

          /* clear space at end of line */

          GrFillRect(w1, gc1, (col - escvalue1) * fonw, cury * fonh,
            (escvalue1) * fonw, fonh);
          GrSetGCForeground(gc1, gi.foreground);
          break;
        }

      case 'L':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          y = cury;
          yy = stdrow - 1;

          /* XXX */

          hide_cursor();

          /* copy from cursor the number of lines down */

          while (--yy >= y)
            {
              GrCopyArea(w1, gc1, 0, (yy + escvalue1) * fonh, winw - 0, fonh,
                w1, 0, yy * fonh, MWROP_COPY);
            }

          /* clear number of lines starting at cursor position */

          GrSetGCForeground(gc1, gi.background);
          GrFillRect(w1, gc1, 0, cury * fonh, winw, escvalue1 * fonh);
          GrSetGCForeground(gc1, gi.foreground);
          break;
        }

      case 'M':

        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          GrCopyArea(w1, gc1, 0, cury * fonh, winw,
            (scrollbottom - cury - escvalue1) * fonh, w1, 0,
            (cury + escvalue1) * fonh, MWROP_COPY);

          /* clear number of lines starting from scrollbottom up */

          GrSetGCForeground(gc1, gi.background);
          GrFillRect(w1, gc1, 0, (scrollbottom - escvalue1) * fonh, winw,
            escvalue1 * fonh);
          GrSetGCForeground(gc1, gi.foreground);
          break;
        }

      case 'S':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          vscrollup(escvalue1);
          break;
        }

      case 'T':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          vscrolldown(escvalue1);
          break;
        }

      case 'H':
      case 'f':
        {
          if (escvalue1 > 0)
            {
              escvalue1--;
            }

          if (escvalue2 > 0)
            {
              escvalue2--;
            }

          pos_yaxis(escvalue1);
          pos_xaxis(escvalue2);
          break;
        }

      case 'E':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          hide_cursor();
          if ((cury += escvalue1) >= scrollbottom)
            {
              cury = scrollbottom - 1;
            }

          curx = 0;
          break;
        }

      case 'F':
        {
          if (escvalue1 == 0)
            {
              escvalue1 = 1;
            }

          hide_cursor();
          if ((cury -= escvalue1) < 0)
            {
              cury = 0;
            }

          curx = 0;
          break;
        }

      case 'd':
        {
          if (escvalue1 > 0)
            {
              escvalue1--;
            }

          pos_yaxis(escvalue1);
          break;
        }

      case '`':
      case 'G':
        {
          if (escvalue1 > 0)
            {
              escvalue1--;
            }

          pos_xaxis(escvalue1);
          break;
        }

      case 'e':
        {
          if (escvalue1 > 0)
            {
              escvalue1--;
            }

          pos_yaxis(escvalue1 + cury);
          break;
        }

      case 'a':
        {
          if (escvalue1 > 0)
            {
              escvalue1--;
            }

          pos_xaxis(escvalue1 + curx);
          break;
        }

      case 'm':

        /* may be more values, do just up to three here
         * foreground colors run from 30 to 37, background from 40 to 47
         */

        {
          rendition(escvalue1);
          if (escvalue2 != 0)
            {
              rendition(escvalue2);
            }

          if (escvalue3 != 0)
            {
              rendition(escvalue3);
            }

          escvalue1 = 0;
          escvalue2 = 0;
          escvalue3 = 0;
          GrGetGCInfo(gc1, &gi);
          break;
        }

      case 's':
        {
          saved_x = curx;
          saved_y = cury;
          break;
        }

      case 'u':
        {
          curx = saved_x;
          cury = saved_y;
          break;
        }

      case 'r':
        {
          if ((escvalue1 > -1) && (escvalue2 <= (winh - 1)))
            {
              scrolltop = escvalue1;
              scrollbottom = escvalue2;
            }
          break;
        }

      case 'h':
        {
          if (escvalue1 == 7)
            {
              wrap = 1;
            }

          if (escvalue1 == 25)
            {
              show_cursor();
            }
          break;
        }

      case 'l':
        {
          if (escvalue1 == 7)
            {
              wrap = 0;
            }

          if (escvalue1 == 25)
            {
              hide_cursor();
            }
          break;
        }

      case 'n':
        {
          sprintf(buf, "\033[%d;%dR", cury + 1, curx + 1);
          write(termfd, buf, strlen(buf));
          break;
        }

      default:
        {
          break;
        }
    }
}

/* un-escaped character print routine
 */

void esc0(unsigned char c)
{
  switch (c)
    {
      case 0:
        {
          /* printing \000 on a terminal means "do nothing".
           * But since we use \000 as string terminator none
           * of the characters that follow were printed.
           *
           * perl -e 'printf("a%ca", 0);'
           *
           * said 'a' in a wterm, but should say 'aa'. This
           * bug screwed up most ncurses programs.
           */

          break;
        }

      case 7:
        {
          if (visualbell)
            {
              /* w_setmode(win, M_INVERS);
               * w_pbox(win, 0, 0, winw, winh);
               * w_test(win, 0, 0);
               * w_pbox(win, 0, 0, winw, winh);
               */
            }
          else
            {
              GrBell();
            }
          break;
        }

      case 8:
        {
          hide_cursor();
          if ((curx -= 1) < 0)
            {
              curx = 0;
            }

          pos_xaxis(curx);
          break;
        }

      case 9:
        {
          int borg;
          int i;

          borg = (((curx >> 3) + 1) << 3);
          if (borg >= col)
            {
              borg = col - 1;
            }

          borg = borg - curx;
          for (i = 0; i < borg; ++i)
            {
              sadd(' ');
            }

          if ((curx = ((curx >> 3) + 1) << 3) >= col)
            {
              curx = col - 1;
            }

          pos_xaxis(curx);
        }
        break;

      case 10:
        {
          sflush();
          if (++cury >= scrollbottom)
            {
              /* scroll before moving cursor, then reduce and add again */

              cury--;
              vscrollup(1);
              cury++;

              /* set cursor into lowest line (cury zero based so -1) */

              cury = scrollbottom - 1;

              /* cury = row-1; */
            }

          pos_yaxis(cury);
          break;
        }

      case 13:
        {
          sflush();
          curx = 0;
          pos_xaxis(curx);
          break;
        }

      case 27:
        {
          sflush();
          semicolonflag = 0;
          escstate = 1;
          break;
        }

      case 127:
        {
          break;
        }

      default:
        {
          sadd(c);
          if (++curx >= col)
            {
              sflush();
              if (!wrap)
                {
                  curx = col - 1;
                }
              else
                {
                  curx = 0;
                  if (++cury >= scrollbottom)
                    {
                      vscrollup(1);
                    }
                }
            }
          break;
        }
    }
}

void printc(unsigned char c)
{
  switch (escstate)
    {
      case 0:
        {
          esc0(c);
          break;
        }

      case 1:
        {
          sflush();
          esc1(c);
          break;
        }

      case 2:
        {
          sflush();
          esc2(c);
          break;
        }

      case 3:
        {
          sflush();
          esc3(c);
          break;
        }

      case 4:
        {
          sflush();
          esc4(c);
          break;
        }

      case 5:
        {
          sflush();
          esc5(c);
          break;
        }

      case 10:
        {
          sflush();
          esc100(c);
          break;
        }

      default:
        {
          escstate = 0;
          break;
        }
    }
}

void init(void)
{
  curx = saved_x = 0;
  cury = saved_y = 0;
  wrap = 1;
  curon = 1;
  curvis = 0;
  escstate = 0;
}

/* general code...
 */

void term(void)
{
  long in;
  long l;
  GR_EVENT_KEYSTROKE *kp;
  int bufflen;
  int gotexpose = 0;
  GR_EVENT wevent;
  unsigned char buf[KBDBUF];

  if (startprogram)
    {
      usleep(1000);
      write(termfd, startprogram, strlen(startprogram));
      write(termfd, "\r", 1);
    }

  while (42)
    {
      if (havefocus)
        {
          draw_cursor();
        }

      GrGetNextEvent(&wevent);

      switch (wevent.type)
        {
          case GR_EVENT_TYPE_CLOSE_REQ:
            {
              GrClose();
              exit(0);
              break;
            }

          case GR_EVENT_TYPE_KEY_DOWN:
            {
              /* deal with special keys */

              kp = (GR_EVENT_KEYSTROKE *)&wevent;
              if (kp->ch & MWKEY_NONASCII_MASK)
                {
                  if (termtype == ANSI)
                    {
                      bufflen = do_special_key_ansi(buf, kp->ch,
                        kp->modifiers);
                    }
                  else
                    {
                      bufflen = do_special_key(buf, kp->ch, kp->modifiers);
                    }
                }
              else
                {
                  *buf = kp->ch & 0xff;
                  bufflen = 1;
                }

              if (bufflen > 0)
                {
                  write(termfd, buf, bufflen);
                }
              break;
            }

          case GR_EVENT_TYPE_FOCUS_IN:
            {
              havefocus = GR_TRUE;
              break;
            }

          case GR_EVENT_TYPE_FOCUS_OUT:
            {
              havefocus = GR_FALSE;
              hide_cursor();
              break;
            }

          case GR_EVENT_TYPE_UPDATE:
            {
              /* if we get temporarily unmapped (moved),
               * set cursor state off.
               */

              if (wevent.update.utype == GR_UPDATE_UNMAPTEMP)
                {
                  hide_cursor();
                }
              break;
            }

          case GR_EVENT_TYPE_EXPOSURE:
            {
              if (!gotexpose)
                {
                  GrRegisterInput(termfd);
                  gotexpose = GR_TRUE;
                }
              break;
            }

          case GR_EVENT_TYPE_FDINPUT:
            {
              if (!gotexpose)
                {
                  break;
                }

              hide_cursor();
              if ((in = read(termfd, buf, sizeof(buf))) > 0)
                {
                  for (l = 0; l < in; l++)
                    {
                      printc(buf[l]);
                    }

                  sflush();
                }
              break;
            }

          case GR_EVENT_TYPE_NONE:
          case GR_EVENT_TYPE_TIMEOUT:
            {
              break;
            }

          default:
            {
              hide_cursor();
              break;
            }
        }
    }
}

int do_special_key(unsigned char *buffer, int key, int modifier)
{
  /* handle vt52 keys here */

  int len;
  char *str;
  char locbuff[32];

  switch (key)
    {
      case  MWKEY_LEFT:
        {
          str = "\033D";
          len = 2;
          break;
        }

      case MWKEY_RIGHT:
        {
          str = "\033C";
          len = 2;
          break;
        }

      case MWKEY_UP:
        {
          if (scrolledFlag)
            {
              str = "";
              len = 0;
              scrolledFlag = 0;
            }
          else
            {
              str = "\033A";
              len = 2;
            }
          break;
        }

      case MWKEY_DOWN:
        {
          str = "\033B";
          len = 2;
          break;
        }

      case MWKEY_KP0:
        {
          str = "\033\077\160";
          len = 3;
          break;
        }

      case MWKEY_KP1:
        {
          str = "\033\077\161";
          len = 3;
          break;
        }

      case MWKEY_KP2:
        {
          str = "\033\077\162";
          len = 3;
          break;
        }

      case MWKEY_KP3:
        {
          str = "\033\077\163";
          len = 3;
          break;
        }

      case MWKEY_KP4:
        {
          str = "\033\077\164";
          len = 3;
          break;
        }

      case MWKEY_KP5:
        {
          str = "\033\077\165";
          len = 3;
          break;
        }

      case MWKEY_KP6:
        {
          str = "\033\077\166";
          len = 3;
          break;
        }

      case MWKEY_KP7:
        {
          str = "\033\077\167";
          len = 3;
          break;
        }

      case MWKEY_KP8:
        {
          str = "\033\077\170";
          len = 3;
          break;
        }

      case MWKEY_KP9:
        {
          str = "\033\077\161";
          len = 3;
          break;
        }

      case MWKEY_KP_PERIOD:
        {
          str = "\033\077\156";
          len = 3;
          break;
        }

      case MWKEY_KP_ENTER:
        {
          str = "\033\077\115";
          len = 3;
          break;
        }

      case MWKEY_DELETE:
        {
          str = "\033C\177";
          len = 3;
          break;
        }

      case MWKEY_F1 ... MWKEY_F12:
        {
          if (modifier & MWKMOD_LMETA)
            {
                locbuff[0] = 033;
              locbuff[1] = 'c';
              locbuff[2] = (char)bgcolor[key - MWKEY_F1];
              locbuff[3] = '\0';
              str = locbuff;
              len = 3;
            }
          else if (modifier & MWKMOD_RMETA)
            {
                locbuff[0] = 033;
              locbuff[1] = 'b';
              locbuff[2] = (char)fgcolor[key - MWKEY_F1];
              locbuff[3] = '\0';
              str = locbuff;
              len = 3;
            }
          else
            {
              switch (key)
                {
                  case MWKEY_F1:
                    {
                      str = "\033Y";
                      len = 2;
                      break;
                    }

                  case MWKEY_F2:
                    {
                      str = "\033P";
                      len = 2;
                      break;
                    }

                  case MWKEY_F3:
                    {
                      str = "\033Q";
                      len = 2;
                      break;
                    }

                  case MWKEY_F4:
                    {
                      str = "\033R";
                      len = 2;
                      break;
                    }

                  case MWKEY_F5:
                    {
                      str = "\033S";
                      len = 2;
                      break;
                    }

                  case MWKEY_F6:
                    {
                      str = "\033T";
                      len = 2;
                      break;
                    }

                  case MWKEY_F7:
                    {
                      str = "\033U";
                      len = 2;
                      break;
                    }

                  case MWKEY_F8:
                    {
                      str = "\033V";
                      len = 2;
                      break;
                    }

                  case MWKEY_F9:
                    {
                      str = "\033W";
                      len = 2;
                      break;
                    }

                  case MWKEY_F10:
                    {
                      str = "\033X";
                      len = 2;
                      break;
                    }
                }
            }
        }

      default:
        {
          len = 0;
        }
    }

  buffer[0] = '\0';
  if (len > 0)
    {
      strcpy((char *)buffer, str);
    }

  return len;
}

int do_special_key_ansi(unsigned char *buffer, int key, int modifier)
{
  int len;
  char *str;
  char locbuff[32];

  switch (key)
    {
      case  MWKEY_LEFT:
        {
          str = "\033[D";
          len = 3;
          break;
        }

      case MWKEY_RIGHT:
        {
          str = "\033[C";
          len = 3;
          break;
        }

      case MWKEY_UP:
        {
          if (scrolledFlag)
            {
              str = "";
              len = 0;
              scrolledFlag = 0;
            }
          else
            {
              str = "\033[A";
              len = 3;
            }
          break;
        }

      case MWKEY_DOWN:
        {
          str = "\033[B";
          len = 3;
          break;
        }

      case MWKEY_HOME:
        {
          str = "\033[H";
          len = 3;
          break;
        }

      case MWKEY_INSERT:
        {
          str = "\033[2~";
          len = 4;
          break;
        }

      case MWKEY_KP0:
        {
          str = "\033Op";
          len = 3;
          break;
        }

      case MWKEY_END:
        {
          str = "\033[F";
          len = 3;
          break;
        }

      case MWKEY_KP1:
        {
          str = "\033Oq";
          len = 3;
          break;
        }

      case MWKEY_KP2:
        {
          str = "\033Or";
          len = 3;
          break;
        }

      case MWKEY_PAGEDOWN:
        {
          str = "\033[6~";
          len = 4;
          break;
        }

      case MWKEY_KP3:
        {
          str = "\033Os";
          len = 3;
          break;
        }

      case MWKEY_KP4:
        {
          str = "\033Ot";
          len = 3;
          break;
        }

      case MWKEY_KP5:
        {
          str = "\033Ou";
          len = 3;
          break;
        }

      case MWKEY_KP6:
        {
          str = "\033Ov";
          len = 3;
          break;
        }

      case MWKEY_KP7:
        {
          str = "\033Ow";
          len = 3;
          break;
        }

      case MWKEY_KP8:
        {
          str = "\033Ox";
          len = 3;
          break;
        }

      case MWKEY_PAGEUP:
        {
          str = "\033[5~";
          len = 4;
          break;
        }

      case MWKEY_KP9:
        {
          str = "\033Oy";
          len = 3;
          break;
        }

/* case MWKEY_KP_PERIOD:
 *       str="\033On";
 *       len=3;
 *       break;
 */

      case MWKEY_KP_ENTER:
        {
          str = "\033OM";
          len = 3;
          break;
        }

      case MWKEY_KP_PERIOD:
      case MWKEY_DELETE:
        {
          str = "\033[3~";
          len = 4;
          break;
        }

      case MWKEY_F1 ... MWKEY_F12:
        {
          if (modifier & MWKMOD_LMETA)
            {
              /* we set background color */

              locbuff[0] = 033;
              locbuff[1] = 'c';
              locbuff[2] = (char)bgcolor[key - MWKEY_F1];
              locbuff[3] = '\0';
              str = locbuff;
              len = 3;
            }
          else if (modifier & MWKMOD_RMETA)
            {
              /* we set foreground color */

              locbuff[0] = 033;
              locbuff[1] = 'b';
              locbuff[2] = (char)fgcolor[key - MWKEY_F1];
              locbuff[3] = '\0';
              str = locbuff;
              len = 3;
            }
          else
            {
              switch (key)
                {
                  case MWKEY_F1:
                    {
                      str = "\033OP";
                      len = 3;
                      break;
                    }

                  case MWKEY_F2:
                    {
                      str = "\033OQ";
                      len = 3;
                      break;
                    }

                  case MWKEY_F3:
                    {
                      str = "\033OR";
                      len = 3;
                      break;
                    }

                  case MWKEY_F4:
                    {
                      str = "\033OS";
                      len = 3;
                      break;
                    }

                  case MWKEY_F5:
                    {
                      str = "\033[15~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F6:
                    {
                      str = "\033[17~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F7:
                    {
                      str = "\033[18~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F8:
                    {
                      str = "\033[19~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F9:
                    {
                      str = "\033[20~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F10:
                    {
                      str = "\033[21~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F11:
                    {
                      str = "\033[22~";
                      len = 5;
                      break;
                    }

                  case MWKEY_F12:
                    {
                      str = "\033[23~";
                      len = 5;
                      break;
                    }
                }
            }
        }

      /* fall thru */

      default:
        {
          len = 0;
        }
    }

  buffer[0] = '\0';
  if (len > 0)
    {
      strcpy((char *)buffer, str);
    }

  return len;
}

void usage(void)
{
  GrError("usage: nxterm [-v5] [command]]\n");
  exit(1);
}

static void *mysignal(int signum, void *handler)
{
  struct sigaction sa;
  struct sigaction so;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(signum, &sa, &so);

  return so.sa_handler;
}

static void sigpipe(int sig)
{
  GrClose();
  kill(-pid, SIGHUP);
  _exit(sig);
}

static void sigchild(int sig)
{
  GrClose();
  _exit(sig);
}

static void sigquit(int sig)
{
  signal(sig, SIG_IGN);
  GrClose();
  kill(-pid, SIGHUP);
}

#ifndef NONETWORK
extern int nanox_server_main(int argc, char *argv[]);
#endif

int main(int argc, char **argv)
{
  GR_CURSOR_ID c1;
  GR_BITMAP bitmap1fg[7];
  GR_BITMAP bitmap1bg[7];

#ifndef NONETWORK
  /* Start the Nano-X server as a separate task; GrOpen() below retries
   * the connection while the server comes up.
   */

  task_create("nanoxserver", 100, 65536, nanox_server_main, NULL);
#endif

  argv++;
  while (*argv && **argv == '-')
    {
      switch (*(*argv + 1))
        {
          case '5':
            {
              termtype = VT52;
              argv++;
              break;
            }

          case 'v':
            {
              visualbell = 1;
              argv++;
              break;
            }

          default:
            {
              usage();
            }
        }
    }

  /* now *argv either points to a program to start or is zero
   */

  if (*argv)
    {
      startprogram = *argv++;
    }

  if (GrOpen() < 0)
    {
      GrError("cannot open graphics\n");
      exit(1);
    }

  GrGetScreenInfo(&si);

  col = stdcol;
  row = stdrow;
  scrolltop = 0;
  scrollbottom = row;

  regFont = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
  /* regFont = GrCreateFontEx(GR_FONT_OEM_FIXED, 0, 0, NULL);
   *boldFont = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
   */

  GrGetFontInfo(regFont, &fi);
  winw = col * fi.maxwidth;
  winh = row * fi.height;
  w1 = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, TITLE, GR_ROOT_WINDOW_ID, -1, -1,
    winw, winh, stdbackground);

  GrSelectEvents(w1,
    GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_EXPOSURE |
    GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_FOCUS_IN |
    GR_EVENT_MASK_FOCUS_OUT | GR_EVENT_MASK_UPDATE |
    GR_EVENT_MASK_CLOSE_REQ);
  GrMapWindow(w1);

  gc1 = GrNewGC();
  GrSetGCFont(gc1, regFont);

  bitmap1fg[0] = MASK7(_, _, X, _, X, _, _);
  bitmap1fg[1] = MASK7(_, _, _, X, _, _, _);
  bitmap1fg[2] = MASK7(_, _, _, X, _, _, _);
  bitmap1fg[3] = MASK7(_, _, _, X, _, _, _);
  bitmap1fg[4] = MASK7(_, _, _, X, _, _, _);
  bitmap1fg[5] = MASK7(_, _, _, X, _, _, _);
  bitmap1fg[6] = MASK7(_, _, X, _, X, _, _);

  bitmap1bg[0] = MASK7(_, X, X, X, X, X, _);
  bitmap1bg[1] = MASK7(_, _, X, X, X, _, _);
  bitmap1bg[2] = MASK7(_, _, X, X, X, _, _);
  bitmap1bg[3] = MASK7(_, _, X, X, X, _, _);
  bitmap1bg[4] = MASK7(_, _, X, X, X, _, _);
  bitmap1bg[5] = MASK7(_, _, X, X, X, _, _);
  bitmap1bg[6] = MASK7(_, X, X, X, X, X, _);

  c1 = GrNewCursor(7, 7, 3, 3, stdforeground, stdbackground, bitmap1fg,
    bitmap1bg);
  GrSetWindowCursor(w1, c1);
  GrSetGCForeground(gc1, stdforeground);
  GrSetGCBackground(gc1, stdbackground);
  GrGetWindowInfo(w1, &wi);
  GrGetGCInfo(gc1, &gi);

  /* create pty, SIGCHLD handler set afterwards in case of grantpt() called */

  termfd = term_init();
  if (termfd < 0)
    {
      GrClose();
      exit(1);
    }

  mysignal(SIGTERM, sigquit);
  mysignal(SIGHUP,  sigquit);
  mysignal(SIGQUIT, sigquit);
  mysignal(SIGPIPE, sigpipe);
  mysignal(SIGCHLD, sigchild);
  mysignal(SIGINT,  SIG_IGN);

  /* just in case we're started in the background */

  signal(SIGTTOU, SIG_IGN);

  init();
  term();
  return 0;
}

int term_init(void)
{
  int tfd;
  int ptfd;
  pid_t child;
  int status;
  posix_spawnattr_t attr;
  posix_spawn_file_actions_t file_actions;
  char *const argv[] =
  {
    CONFIG_SYSTEM_NSH_PROGNAME, NULL
  };

  tfd = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (tfd < 0)
    {
      GrError("Can't create pty /dev/ptmx\n");
      return -1;
    }

  strcpy(ptyname, ptsname(tfd));

  signal(SIGCHLD, SIG_DFL);
  grantpt(tfd);
  unlockpt(tfd);

  if ((ptfd = open(ptyname, O_RDWR)) < 0)
    {
      GrError("Can't open %s\n", ptyname);
      return -1;
    }

  posix_spawnattr_init(&attr);
  posix_spawnattr_setstacksize(&attr, CONFIG_SYSTEM_NSH_STACKSIZE);

  posix_spawn_file_actions_init(&file_actions);
  posix_spawn_file_actions_addclose(&file_actions, STDIN_FILENO);
  posix_spawn_file_actions_addclose(&file_actions, STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&file_actions, STDERR_FILENO);
  posix_spawn_file_actions_adddup2(&file_actions, ptfd, STDIN_FILENO);
  posix_spawn_file_actions_adddup2(&file_actions, ptfd, STDOUT_FILENO);
  posix_spawn_file_actions_adddup2(&file_actions, ptfd, STDERR_FILENO);
  posix_spawn_file_actions_addclose(&file_actions, ptfd);

  status = posix_spawn(&child, CONFIG_SYSTEM_NSH_PROGNAME, &file_actions,
    &attr, argv, environ);

  posix_spawn_file_actions_destroy(&file_actions);
  close(ptfd);

  if (status != 0)
    {
      GrError("Error: posix_spawn returned %d\n", status);
      return -1;
    }

  return tfd;
}
