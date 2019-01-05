/****************************************************************************
 * apps/graphics/pdcurses/pdc_getstr.c
 * Public Domain Curses
 * RCSID("$Id: getstr.c,v 1.51 2008/07/14 04:24:51 wmcbrine Exp $")
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

/* Name: getstr
 *
 * Synopsis:
 *       int getstr(char *str);
 *       int wgetstr(WINDOW *win, char *str);
 *       int mvgetstr(int y, int x, char *str);
 *       int mvwgetstr(WINDOW *win, int y, int x, char *str);
 *       int getnstr(char *str, int n);
 *       int wgetnstr(WINDOW *win, char *str, int n);
 *       int mvgetnstr(int y, int x, char *str, int n);
 *       int mvwgetnstr(WINDOW *win, int y, int x, char *str, int n);
 *
 *       int get_wstr(wint_t *wstr);
 *       int wget_wstr(WINDOW *win, wint_t *wstr);
 *       int mvget_wstr(int y, int x, wint_t *wstr);
 *       int mvwget_wstr(WINDOW *win, int, int, wint_t *wstr);
 *       int getn_wstr(wint_t *wstr, int n);
 *       int wgetn_wstr(WINDOW *win, wint_t *wstr, int n);
 *       int mvgetn_wstr(int y, int x, wint_t *wstr, int n);
 *       int mvwgetn_wstr(WINDOW *win, int y, int x, wint_t *wstr, int n);
 *
 * Description:
 *       These routines call wgetch() repeatedly to build a string,
 *       interpreting erase and kill characters along the way, until a
 *       newline or carriage return is received. When PDCurses is built
 *       with wide-character support enabled, the narrow-character
 *       functions convert the wgetch()'d values into a multibyte string
 *       in the current locale before returning it. The resulting string
 *       is placed in the area pointed to by *str. The routines with n as
 *       the last argument read at most n characters.
 *
 *       Note that there's no way to know how long the buffer passed to
 *       wgetstr() is, so use wgetnstr() to avoid buffer overflows.
 *
 * Return Value:
 *       This functions return ERR on failure or any other value on
 *       success.
 *
 * Portability                                X/Open    BSD    SYS V
 *       getstr                                  Y       Y       Y
 *       wgetstr                                 Y       Y       Y
 *       mvgetstr                                Y       Y       Y
 *       mvwgetstr                               Y       Y       Y
 *       getnstr                                 Y       -      4.0
 *       wgetnstr                                Y       -      4.0
 *       mvgetnstr                               Y       -       -
 *       mvwgetnstr                              Y       -       -
 *       get_wstr                                Y
 *       wget_wstr                               Y
 *       mvget_wstr                              Y
 *       mvwget_wstr                             Y
 *       getn_wstr                               Y
 *       wgetn_wstr                              Y
 *       mvgetn_wstr                             Y
 *       mvwgetn_wstr                            Y
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "curspriv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXLINE 255

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int wgetnstr(WINDOW *win, char *str, int n)
{
#ifdef CONFIG_PDCURSES_WIDE
  wchar_t wstr[MAXLINE + 1];

  if (n < 0 || n > MAXLINE)
    {
      n = MAXLINE;
    }

  if (wgetn_wstr(win, (wint_t *) wstr, n) == ERR)
    {
      return ERR;
    }

  return PDC_wcstombs(str, wstr, n);
#else
  int ch;
  int i;
  int num;
  int x;
  int chars;
  char *p;
  bool stop;
  bool oldecho;
  bool oldcbreak;
  bool oldnodelay;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("wgetnstr() - called\n"));

  if (!win || !str)
    {
      return ERR;
    }

  chars = 0;
  p = str;
  stop = false;

  x = win->_curx;

  oldcbreak  = SP->cbreak;      /* Remember states */
  oldecho    = SP->echo;
  oldnodelay = win->_nodelay;

  SP->echo   = false;           /* We do echo ourselves */
  cbreak();                     /* Ensure each key is returned immediately */
  win->_nodelay = false;        /* Don't return -1 */

  wrefresh(win);

  while (!stop)
    {
      ch = wgetch(win);

      switch (ch)
        {
        case '\t':
          ch = ' ';
          num = TABSIZE - (win->_curx - x) % TABSIZE;
          for (i = 0; i < num; i++)
            {
              if (chars < n)
                {
                  if (oldecho)
                    {
                      waddch(win, ch);
                    }

                  *p++ = ch;
                  ++chars;
                }
              else
                {
                  beep();
                }
            }
          break;

        case _ECHAR:           /* CTRL-H -- Delete character */
          if (p > str)
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              ch = (unsigned char)(*--p);
              if ((ch < ' ') && (oldecho))
                {
                  waddstr(win, "\b \b");
                }

              chars--;
            }
          break;

        case _DLCHAR:          /* CTRL-U -- Delete line */
          while (p > str)
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              ch = (unsigned char)(*--p);
              if ((ch < ' ') && (oldecho))
                {
                  waddstr(win, "\b \b");
                }
            }

          chars = 0;
          break;

        case _DWCHAR:          /* CTRL-W -- Delete word */

          while ((p > str) && (*(p - 1) == ' '))
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              --p;              /* Remove space */
              chars--;
            }
          while ((p > str) && (*(p - 1) != ' '))
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              ch = (unsigned char)(*--p);
              if ((ch < ' ') && (oldecho))
                {
                  waddstr(win, "\b \b");
                }

              chars--;
            }
          break;

        case '\n':
        case '\r':
          stop = true;
          if (oldecho)
            {
              waddch(win, '\n');
            }
          break;

        default:
          if (chars < n)
            {
              if (!SP->key_code && ch < 0x100)
                {
                  *p++ = ch;
                  if (oldecho)
                    {
                      waddch(win, ch);
                    }

                  chars++;
                }
            }
          else
            {
              beep();
            }

          break;

        }

      wrefresh(win);
    }

  *p = '\0';

  SP->echo = oldecho;           /* restore old settings */
  SP->cbreak = oldcbreak;
  win->_nodelay = oldnodelay;

  return OK;
#endif
}

int getstr(char *str)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("getstr() - called\n"));

  return wgetnstr(stdscr, str, MAXLINE);
}

int wgetstr(WINDOW *win, char *str)
{
  PDC_LOG(("wgetstr() - called\n"));

  return wgetnstr(win, str, MAXLINE);
}

int mvgetstr(int y, int x, char *str)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvgetstr() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wgetnstr(stdscr, str, MAXLINE);
}

int mvwgetstr(WINDOW *win, int y, int x, char *str)
{
  PDC_LOG(("mvwgetstr() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wgetnstr(win, str, MAXLINE);
}

int getnstr(char *str, int n)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("getnstr() - called\n"));

  return wgetnstr(stdscr, str, n);
}

int mvgetnstr(int y, int x, char *str, int n)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvgetnstr() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wgetnstr(stdscr, str, n);
}

int mvwgetnstr(WINDOW *win, int y, int x, char *str, int n)
{
  PDC_LOG(("mvwgetnstr() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wgetnstr(win, str, n);
}

#ifdef CONFIG_PDCURSES_WIDE
int wgetn_wstr(WINDOW *win, wint_t *wstr, int n)
{
  int ch;
  int i;
  int num;
  int x;
  int chars;
  char *p;
  bool stop;
  bool oldecho;
  bool oldcbreak;
  bool oldnodelay;
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif

  PDC_LOG(("wgetn_wstr() - called\n"));

  if (!win || !wstr)
    {
      return ERR;
    }

  chars = 0;
  p     = wstr;
  stop  = false;

  x     = win->_curx;

  oldcbreak = SP->cbreak;       /* remember states */
  oldecho = SP->echo;
  oldnodelay = win->_nodelay;

  SP->echo = false;             /* we do echo ourselves */
  cbreak();                     /* ensure each key is returned immediately */
  win->_nodelay = false;        /* don't return -1 */

  wrefresh(win);

  while (!stop)
    {
      ch = wgetch(win);
      switch (ch)
        {
        case '\t':
          ch = ' ';
          num = TABSIZE - (win->_curx - x) % TABSIZE;
          for (i = 0; i < num; i++)
            {
              if (chars < n)
                {
                  if (oldecho)
                    {
                      waddch(win, ch);
                    }

                  *p++ = ch;
                  ++chars;
                }
              else
                {
                  beep();
                }
            }
          break;

        case _ECHAR:           /* CTRL-H -- Delete character */
          if (p > wstr)
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              ch = *--p;
              if ((ch < ' ') && (oldecho))
                {
                  waddstr(win, "\b \b");
                }

              chars--;
            }
          break;

        case _DLCHAR:          /* CTRL-U -- Delete line */
          while (p > wstr)
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              ch = *--p;
              if ((ch < ' ') && (oldecho))
                {
                  waddstr(win, "\b \b");
                }
            }

          chars = 0;
          break;

        case _DWCHAR:          /* CTRL-W -- Delete word */
          while ((p > wstr) && (*(p - 1) == ' '))
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              --p;              /* remove space */
              chars--;
            }

          while ((p > wstr) && (*(p - 1) != ' '))
            {
              if (oldecho)
                {
                  waddstr(win, "\b \b");
                }

              ch = *--p;
              if ((ch < ' ') && (oldecho))
                {
                  waddstr(win, "\b \b");
                }

              chars--;
            }
          break;

        case '\n':
        case '\r':
          stop = true;
          if (oldecho)
            {
              waddch(win, '\n');
            }
          break;

        default:
          if (chars < n)
            {
              if (!SP->key_code)
                {
                  *p++ = ch;
                  if (oldecho)
                    {
                      waddch(win, ch);
                    }

                  chars++;
                }
            }
          else
            {
              beep();
            }

          break;
        }

      wrefresh(win);
    }

  *p = '\0';

  SP->echo = oldecho;           /* restore old settings */
  SP->cbreak = oldcbreak;
  win->_nodelay = oldnodelay;

  return OK;
}

int get_wstr(wint_t *wstr)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("get_wstr() - called\n"));

  return wgetn_wstr(stdscr, wstr, MAXLINE);
}

int wget_wstr(WINDOW *win, wint_t *wstr)
{
  PDC_LOG(("wget_wstr() - called\n"));

  return wgetn_wstr(win, wstr, MAXLINE);
}

int mvget_wstr(int y, int x, wint_t *wstr)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvget_wstr() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wgetn_wstr(stdscr, wstr, MAXLINE);
}

int mvwget_wstr(WINDOW *win, int y, int x, wint_t *wstr)
{
  PDC_LOG(("mvwget_wstr() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wgetn_wstr(win, wstr, MAXLINE);
}

int getn_wstr(wint_t *wstr, int n)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("getn_wstr() - called\n"));

  return wgetn_wstr(stdscr, wstr, n);
}

int mvgetn_wstr(int y, int x, wint_t *wstr, int n)
{
#ifdef CONFIG_PDCURSES_MULTITHREAD
  FAR struct pdc_context_s *ctx = PDC_ctx();
#endif
  PDC_LOG(("mvgetn_wstr() - called\n"));

  if (move(y, x) == ERR)
    {
      return ERR;
    }

  return wgetn_wstr(stdscr, wstr, n);
}

int mvwgetn_wstr(WINDOW *win, int y, int x, wint_t *wstr, int n)
{
  PDC_LOG(("mvwgetn_wstr() - called\n"));

  if (wmove(win, y, x) == ERR)
    {
      return ERR;
    }

  return wgetn_wstr(win, wstr, n);
}
#endif
