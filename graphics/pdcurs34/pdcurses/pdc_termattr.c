/****************************************************************************
 * apps/graphics/pdcurses/pdc_termattr.c
 * Public Domain Curses
 * RCSID("$Id: termattr.c,v 1.54 2008/07/13 16:08:18 wmcbrine Exp $")
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

/* Name: termattr
 *
 * Synopsis:
 *       int baudrate(void);
 *       char erasechar(void);
 *       bool has_ic(void);
 *       bool has_il(void);
 *       char killchar(void);
 *       char *longname(void);
 *       chtype termattrs(void);
 *       attr_t term_attrs(void);
 *       char *termname(void);
 *
 *       int erasewchar(wchar_t *ch);
 *       int killwchar(wchar_t *ch);
 *
 *       char wordchar(void);
 *
 * Description:
 *       baudrate() is supposed to return the output speed of the
 *       terminal. In PDCurses, it simply returns INT_MAX.
 *
 *       has_ic and has_il() return true. These functions have meaning in
 *       some other implementations of curses.
 *
 *       erasechar() and killchar() return ^H and ^U, respectively -- the
 *       ERASE and KILL characters. In other curses implementations,
 *       these may vary by terminal type. erasewchar() and killwchar()
 *       are the wide-character versions; they take a pointer to a
 *       location in which to store the character, and return OK or ERR.
 *
 *       longname() returns a pointer to a static area containing a
 *       verbose description of the current terminal. The maximum length
 *       of the string is 128 characters.  It is defined only after the
 *       call to initscr() or newterm().
 *
 *       termname() returns a pointer to a static area containing a
 *       short description of the current terminal (14 characters).
 *
 *       termattrs() returns a logical OR of all video attributes
 *       supported by the terminal.
 *
 *       wordchar() is a PDCurses extension of the concept behind the
 *       functions erasechar() and killchar(), returning the "delete
 *       word" character, ^W.
 *
 * Portability                                X/Open    BSD    SYS V
 *       baudrate                                Y       Y       Y
 *       erasechar                               Y       Y       Y
 *       has_ic                                  Y       Y       Y
 *       has_il                                  Y       Y       Y
 *       killchar                                Y       Y       Y
 *       longname                                Y       Y       Y
 *       termattrs                               Y       Y       Y
 *       termname                                Y       Y       Y
 *       erasewchar                              Y
 *       killwchar                               Y
 *       term_attrs                              Y
 *       wordchar                                -       -       -
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <limits.h>

#include "curspriv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int baudrate(void)
{
  PDC_LOG(("baudrate() - called\n"));

  return INT_MAX;
}

char erasechar(void)
{
  PDC_LOG(("erasechar() - called\n"));

  return _ECHAR;                /* character delete char (^H) */
}

bool has_ic(void)
{
  PDC_LOG(("has_ic() - called\n"));

  return true;
}

bool has_il(void)
{
  PDC_LOG(("has_il() - called\n"));

  return true;
}

char killchar(void)
{
  PDC_LOG(("killchar() - called\n"));

  return _DLCHAR;               /* line delete char (^U) */
}

char *longname(void)
{
  PDC_LOG(("longname() - called\n"));

  return ttytype + 9;           /* skip "pdcurses|" */
}

chtype termattrs(void)
{
  chtype temp = A_BLINK | A_BOLD | A_INVIS | A_REVERSE | A_UNDERLINE;

  /* note: blink is bold background on some platforms */

  PDC_LOG(("termattrs() - called\n"));

  if (!SP->mono)
    {
      temp |= A_COLOR;
    }

  return temp;
}

attr_t term_attrs(void)
{
  PDC_LOG(("term_attrs() - called\n"));

  return WA_BLINK | WA_BOLD | WA_INVIS | WA_LEFT | WA_REVERSE |
         WA_RIGHT | WA_UNDERLINE;
}

char *termname(void)
{
  PDC_LOG(("termname() - called\n"));

  return "pdcurses";
}

char wordchar(void)
{
  PDC_LOG(("wordchar() - called\n"));

  return _DWCHAR;               /* word delete char */
}

#ifdef CONFIG_PDCURSES_WIDE
int erasewchar(wchar_t *ch)
{
  PDC_LOG(("erasewchar() - called\n"));

  if (!ch)
    {
      return ERR;
    }

  *ch = (wchar_t) _ECHAR;
  return OK;
}

int killwchar(wchar_t *ch)
{
  PDC_LOG(("killwchar() - called\n"));

  if (!ch)
    {
      return ERR;
    }

  *ch = (wchar_t) _DLCHAR;
  return OK;
}
#endif
