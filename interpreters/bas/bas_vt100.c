/****************************************************************************
 * apps/interpreters/bas/bas_vt100.c
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
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>

#include <nuttx/vt100.h>

#include "bas_fs.h"
#include "bas_vt100.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* VT100 escape sequences */

#if 0 /* Not used */
static const char g_cursoron[]      = VT100_CURSORON;
static const char g_cursoroff[]     = VT100_CURSOROFF;
#endif
static const char g_cursorhome[]    = VT100_CURSORHOME;
#if 0 /* Not used */
static const char g_erasetoeol[]    = VT100_CLEAREOL;
#endif
static const char g_clrscreen[]     = VT100_CLEARSCREEN;
#if 0 /* Not used */
static const char g_index[]         = VT100_INDEX;
static const char g_revindex[]      = VT100_REVINDEX;
static const char g_attriboff[]     = VT100_MODESOFF;
static const char g_boldon[]        = VT100_BOLD;
static const char g_reverseon[]     = VT100_REVERSE;
static const char g_blinkon[]       = VT100_BLINK;
static const char g_boldoff[]       = VT100_BOLDOFF;
static const char g_reverseoff[]    = VT100_REVERSEOFF;
static const char g_blinkoff[]      = VT100_BLINKOFF;
#endif

static const char g_fmt_cursorpos[] = VT100_FMT_CURSORPOS;
static const char g_fmt_forecolor[] = VT100_FMT_FORE_COLOR;
static const char g_fmt_backcolor[] = VT100_FMT_BACK_COLOR;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vt100_write
 *
 * Description:
 *   Write a sequence of bytes to the channel device
 *
 ****************************************************************************/

static void vt100_write(int chn, FAR const char *buffer, size_t buflen)
{
  for (; buflen > 0; buflen--)
    {
      FS_putChar(chn, *buffer++);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vt100_blinkon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_blinkon(int chn)
{
  /* Send the VT100 BLINKON command */

  vt100_write(chn, g_blinkon, sizeof(g_blinkon));
}
#endif

/****************************************************************************
 * Name: vt100_boldon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_boldon(int chn)
{
  /* Send the VT100 BOLDON command */

  vt100_write(chn, g_boldon, sizeof(g_boldon));
}
#endif

/****************************************************************************
 * Name: vt100_reverseon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_reverseon(int chn)
{
  /* Send the VT100 REVERSON command */

  vt100_write(chn, g_reverseon, sizeof(g_reverseon));
}
#endif

/****************************************************************************
 * Name: vt100_attriboff
 *
 * Description:
 *   Disable all previously selected attributes.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_attriboff(int chn)
{
  /* Send the VT100 ATTRIBOFF command */

  vt100_write(chn, g_attriboff, sizeof(g_attriboff));
}
#endif

/****************************************************************************
 * Name: vt100_cursoron
 *
 * Description:
 *   Turn on the cursor
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_cursoron(int chn)
{
  /* Send the VT100 CURSORON command */

  vt100_write(chn, g_cursoron, sizeof(g_cursoron));
}
#endif

/****************************************************************************
 * Name: vt100_cursoroff
 *
 * Description:
 *   Turn off the cursor
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_cursoroff(int chn)
{
  /* Send the VT100 CURSOROFF command */

  vt100_write(chn, g_cursoroff, sizeof(g_cursoroff));
}
#endif

/****************************************************************************
 * Name: vt100_cursorhome
 *
 * Description:
 *   Move the current cursor to the upper left hand corner of the display
 *
 ****************************************************************************/

void vt100_cursorhome(int chn)
{
  /* Send the VT100 CURSORHOME command */

  vt100_write(chn, g_cursorhome, sizeof(g_cursorhome));
}

/****************************************************************************
 * Name: vt100_setcursor
 *
 * Description:
 *   Move the current cursor position to position (row,col)
 *
 ****************************************************************************/

void vt100_setcursor(int chn, uint16_t row, uint16_t column)
{
  char buffer[16];
  int len;

  /* Format the cursor position command.  The origin is (1,1). */

  len = snprintf(buffer, 16, g_fmt_cursorpos, row + 1, column + 1);

  /* Send the VT100 CURSORPOS command */

  vt100_write(chn, buffer, len);
}

/****************************************************************************
 * Name: vt100_clrtoeol
 *
 * Description:
 *   Clear the display from the current cursor position to the end of the
 *   current line.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_clrtoeol(int chn)
{
  /* Send the VT100 ERASETOEOL command */

  vt100_write(chn, g_erasetoeol, sizeof(g_erasetoeol));
}
#endif

/****************************************************************************
 * Name: vt100_clrscreen
 *
 * Description:
 *   Clear the entire display
 *
 ****************************************************************************/

void vt100_clrscreen(int chn)
{
  /* Send the VT100 CLRSCREEN command */

  vt100_write(chn, g_clrscreen, sizeof(g_clrscreen));
}

/****************************************************************************
 * Name: vt100_scrollup
 *
 * Description:
 *   Scroll the display up 'nlines' by sending the VT100 INDEX command.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_scrollup(int chn, uint16_t nlines)
{
  /* Scroll for the specified number of lines */

  for (; nlines; nlines--)
    {
      /* Send the VT100 INDEX command */

      vt100_write(chn, g_index, sizeof(g_index));
    }
}
#endif

/****************************************************************************
 * Name: vt100_scrolldown
 *
 * Description:
 *   Scroll the display down 'nlines' by sending the VT100 REVINDEX command.
 *
 ****************************************************************************/

#if 0 /* Not used */
void vt100_scrolldown(int chn, uint16_t nlines)
{
  /* Scroll for the specified number of lines */

  for (; nlines; nlines--)
    {
      /* Send the VT100 REVINDEX command */

      vt100_write(chn, g_revindex, sizeof(g_revindex));
    }
}
#endif

/****************************************************************************
 * Name: vt100_foreground_color
 *
 * Description:
 *   Set the foreground color
 *
 ****************************************************************************/

void vt100_foreground_color(int chn, uint8_t color)
{
  char buffer[16];
  int len;

  /* Format the foreground color command. */

  DEBUGASSERT(color < 10);
  len = snprintf(buffer, 16, g_fmt_forecolor, color);

  /* Send the VT100 foreground color command */

  vt100_write(chn, buffer, len);
}

/****************************************************************************
 * Name: vt100_background_color
 *
 * Description:
 *   Set the background color
 *
 ****************************************************************************/

void vt100_background_color(int chn, uint8_t color)
{
  char buffer[16];
  int len;

  /* Format the background color command. */

  DEBUGASSERT(color < 10);
  len = snprintf(buffer, 16, g_fmt_backcolor, color);

  /* Send the VT100 background color command */

  vt100_write(chn, buffer, len);
}
