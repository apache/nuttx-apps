/****************************************************************************
 * apps/system/cle/cle.c
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

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/ascii.h>
#include <nuttx/vt100.h>

#include "system/cle.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Some environments may return CR as end-of-line, others LF, and others
 * both.  If not specified, the logic here assumes either (but not both) as
 * the default.
 */

#if defined(CONFIG_EOL_IS_CR)
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#  undef  CONFIG_EOL_IS_EITHER_CRLF
#elif defined(CONFIG_EOL_IS_LF)
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#  undef  CONFIG_EOL_IS_EITHER_CRLF
#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_EITHER_CRLF
#elif defined(CONFIG_EOL_IS_EITHER_CRLF)
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#else
#  undef  CONFIG_EOL_IS_CR
#  undef  CONFIG_EOL_IS_LF
#  undef  CONFIG_EOL_IS_BOTH_CRLF
#  define CONFIG_EOL_IS_EITHER_CRLF 1
#endif

/* Control characters */

#undef  CTRL
#define CTRL(a)         ((a) & 0x1f)

#define CLE_BEL(priv)   cle_putch(priv,CTRL('G'))

/* Sizes of things */

#define TABSIZE         8    /* A TAB is eight characters */
#define TABMASK         7    /* Mask for TAB alignment */
#define NEXT_TAB(p)     (((p) + TABSIZE) & ~TABMASK)

/* Debug */

#ifndef CONFIG_SYSTEM_CLE_DEBUGLEVEL
#  define CONFIG_SYSTEM_CLE_DEBUGLEVEL 0
#endif

#if CONFIG_SYSTEM_CLE_DEBUGLEVEL > 0
#  define cledbg  cle_debug
#else
#  define cledbg  _none
#endif

#if CONFIG_SYSTEM_CLE_DEBUGLEVEL > 1
#  define cleinfo cle_debug
#else
#  define cleinfo _none
#endif

#ifdef CONFIG_SYSTEM_COLOR_CLE
#  define COLOR_PROMPT  VT100_YELLOW
#  define COLOR_COMMAND VT100_CYAN
#  define COLOR_OUTPUT  VT100_GREEN
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* VI Key Bindings */

enum cle_key_e
{
  KEY_BEGINLINE   = CTRL('A'),  /* Move cursor to start of current line */
  KEY_LEFT        = CTRL('B'),  /* Move left one character */
  KEY_DEL         = CTRL('D'),  /* Delete a single character at the cursor position */
  KEY_ENDLINE     = CTRL('E'),  /* Move cursor to end of current line */
  KEY_RIGHT       = CTRL('F'),  /* Move right one character */
  KEY_DELLEFT     = CTRL('H'),  /* Delete character, left (backspace)  */
  KEY_DELEOL      = CTRL('K'),  /* Delete to the end of the line */
  KEY_CLRSCR      = CTRL('L'),  /* Clear the screen */
  KEY_DN          = CTRL('N'),  /* Cursor down */
  KEY_UP          = CTRL('P'),  /* Cursor up   */
  KEY_DELLINE     = CTRL('U'),  /* Delete the entire line */
  KEY_QUOTE       = '\\'        /* The next character is quote (use literal value) */
};

/* This structure describes the overall state of the editor */

struct cle_s
{
  uint16_t curpos;          /* Current cursor position */
  uint16_t cursave;         /* Saved cursor position */
  uint16_t row;             /* This is the row that we are editing in */
  uint16_t coloffs;         /* Left cursor offset */
  uint16_t linelen;         /* Size of the line buffer */
  uint16_t nchars;          /* Size of data in the line buffer */
  int infd;                 /* Input file handle */
  int outfd;                /* Output file handle */
  FAR char *line;           /* Line buffer */
  FAR const char *prompt;   /* Prompt, in case we have to re-print it */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#if CONFIG_SYSTEM_CLE_DEBUGLEVEL > 0
static void     cle_debug(FAR const char *fmt, ...) printflike(1, 2);
#endif

/* Low-level display and data entry functions */

static void     cle_write(FAR struct cle_s *priv, FAR const char *buffer,
                  uint16_t buflen);
static void     cle_putch(FAR struct cle_s *priv, char ch);
static int      cle_getch(FAR struct cle_s *priv);
static void     cle_cursoron(FAR struct cle_s *priv);
static void     cle_cursoroff(FAR struct cle_s *priv);
static void     cle_setcursor(FAR struct cle_s *priv, uint16_t column);
static int      cle_getcursor(FAR struct cle_s *priv, uint16_t *prow,
                  uint16_t *pcolumn);
static void     cle_clrtoeol(FAR struct cle_s *priv);

/* Editor function */

static bool     cle_opentext(FAR struct cle_s *priv, uint16_t pos,
                  uint16_t increment);
static void     cle_closetext(FAR struct cle_s *priv, uint16_t pos,
                  uint16_t size);
static void     cle_showtext(FAR struct cle_s *priv);
static void     cle_insertch(FAR struct cle_s *priv, char ch);
static int      cle_editloop(FAR struct cle_s *priv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_CLE_CMD_HISTORY
/* Command history
 *
 *   g_cmd_history[][]             Circular buffer
 *   g_cmd_history_head            Head of the circular buffer, most recent
 *                                 command
 *   g_cmd_history_steps_from_head Offset from head
 *   g_cmd_history_len             Number of elements in the circular buffer
 *
 * REVISIT:  These globals will *not* work in an environment where there
 * are multiple copies if the NSH shell!  Use of global variables is not
 * thread safe!  These settings should, at least, be semaphore protected so
 * that the integrity of the data is assured, even though commands from
 * different sessions may be intermixed.
 */

static char g_cmd_history[CONFIG_SYSTEM_CLE_CMD_HISTORY_LEN]
                         [CONFIG_SYSTEM_CLE_CMD_HISTORY_LINELEN];
static int g_cmd_history_head            = -1;
static int g_cmd_history_steps_from_head = 1;
static int g_cmd_history_len             = 0;

#endif /* CONFIG_SYSTEM_CLE_CMD_HISTORY */

/* VT100 escape sequences */

static const char g_cursoron[]     = VT100_CURSORON;
static const char g_cursoroff[]    = VT100_CURSOROFF;
static const char g_getcursor[]    = VT100_GETCURSOR;
static const char g_erasetoeol[]   = VT100_CLEAREOL;
static const char g_fmtcursorpos[] = VT100_FMT_CURSORPOS;
static const char g_clrscr[]       = VT100_CLEARSCREEN;
static const char g_home[]         = VT100_CURSORHOME;
#ifdef CONFIG_SYSTEM_COLOR_CLE
static const char g_setcolor[]     = VT100_FMT_FORE_COLOR;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cle_debug
 *
 * Description:
 *   Print a debug message to the syslog
 *
 ****************************************************************************/

#if CONFIG_SYSTEM_CLE_DEBUGLEVEL > 0
static void cle_debug(FAR const char *fmt, ...)
{
  va_list ap;

  /* Let vsyslog do the real work */

  va_start(ap, fmt);
  vsyslog(LOG_DEBUG, fmt, ap);
  va_end(ap);
}
#endif

/****************************************************************************
 * Name: cle_write
 *
 * Description:
 *   Write a sequence of bytes to the console output device.
 *
 ****************************************************************************/

static void cle_write(FAR struct cle_s *priv, FAR const char *buffer,
                      uint16_t buflen)
{
  ssize_t nwritten;

  /* Loop until all bytes have been successfully written (or until a
   * unrecoverable error is encountered)
   */

  do
    {
      /* Put the next gulp */

      nwritten = write(priv->outfd, buffer, buflen);

      /* Handle write errors.  write() should neve return 0. */

      if (nwritten <= 0)
        {
          /* EINTR is not really an error; it simply means that a signal was
           * received while waiting for write.
           */

          int errcode = errno;
          if (nwritten == 0 || errcode != EINTR)
            {
              cledbg("ERROR: write to stdout failed: %d\n", errcode);
              return;
            }
        }

      /* Decrement the count of bytes remaining to be sent (to handle the
       * case of a partial write)
       */

      else
        {
          buffer += nwritten;
          buflen -= nwritten;
        }
    }
  while (buflen > 0);
}

/****************************************************************************
 * Name: cle_putch
 *
 * Description:
 *   Write a single character to the console output device.
 *
 ****************************************************************************/

static void cle_putch(FAR struct cle_s *priv, char ch)
{
  cle_write(priv, &ch, 1);
}

/****************************************************************************
 * Name: cle_getch
 *
 * Description:
 *   Get a single character from the console input device.
 *
 ****************************************************************************/

static int cle_getch(FAR struct cle_s *priv)
{
  char buffer;
  ssize_t nread;

  /* Loop until we successfully read a character (or until an unexpected
   * error occurs).
   */

  do
    {
      /* Read one character from the incoming stream */

      nread = read(priv->infd, &buffer, 1);

      /* Check for error or end-of-file. */

      if (nread <= 0)
        {
          /* EINTR is not really an error; it simply means that a signal we
           * received while waiting for input.
           */

          int errcode = errno;
          if (nread == 0 || errcode != EINTR)
            {
              cledbg("ERROR: read from stdin failed: %d\n", errcode);
              return -EIO;
            }
        }
    }
  while (nread < 1);

  /* On success, return the character that was read */

  cleinfo("Returning: %c[%02x]\n", isprint(buffer) ? buffer : '.', buffer);
  return buffer;
}

/****************************************************************************
 * Name: cle_cursoron
 *
 * Description:
 *   Turn on the cursor
 *
 ****************************************************************************/

static void cle_cursoron(FAR struct cle_s *priv)
{
  /* Send the VT100 CURSORON command */

  cle_write(priv, g_cursoron, sizeof(g_cursoron));
}

/****************************************************************************
 * Name: cle_cursoroff
 *
 * Description:
 *   Turn off the cursor
 *
 ****************************************************************************/

static void cle_cursoroff(FAR struct cle_s *priv)
{
  /* Send the VT100 CURSOROFF command */

  cle_write(priv, g_cursoroff, sizeof(g_cursoroff));
}

/****************************************************************************
 * Name: cle_setcursor
 *
 * Description:
 *   Move the current cursor position to position (row,col)
 *
 ****************************************************************************/

static void cle_setcursor(FAR struct cle_s *priv, uint16_t column)
{
  char buffer[16];
  int len;

  cleinfo("row=%d column=%d offset=%d\n", priv->row, column, priv->coloffs);

  /* Format the cursor position command.  The origin is (1,1). */

  len = snprintf(buffer, 16, g_fmtcursorpos,
                 priv->row, column + priv->coloffs);

  /* Send the VT100 CURSORPOS command */

  cle_write(priv, buffer, len);
}

/****************************************************************************
 * Name: cle_setcolor
 *
 * Description:
 *   Set foreground color
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_COLOR_CLE
static void cle_setcolor(FAR struct cle_s *priv, uint8_t color)
{
  char buffer[16];
  int len;

  len = snprintf(buffer, 16, g_setcolor, color);
  cle_write(priv, buffer, len);
}
#endif

/****************************************************************************
 * Name: cle_outputprompt
 *
 * Description:
 *   Send the prompt to the screen
 *
 ****************************************************************************/

static void cle_outputprompt(FAR struct cle_s *priv)
{
#ifdef CONFIG_SYSTEM_COLOR_CLE
  cle_setcolor(priv, COLOR_PROMPT);
#endif
  cle_write(priv, priv->prompt, strlen(priv->prompt));
#ifdef CONFIG_SYSTEM_COLOR_CLE
  cle_setcolor(priv, COLOR_OUTPUT);
#endif
}

/****************************************************************************
 * Name: cle_clrscr
 *
 * Description:
 *   Clear the screen, and re-establish the prompt
 *
 ****************************************************************************/

static void cle_clrscr(FAR struct cle_s *priv)
{
  /* Send the VT100 CLEARSCREEN command */

  cle_write(priv, g_clrscr, sizeof(g_clrscr));
  cle_write(priv, g_home, sizeof(g_home));
  priv->row = 1;
  cle_outputprompt(priv);
}

/****************************************************************************
 * Name: cle_getcursor
 *
 * Description:
 *   Get the current cursor position.
 *
 ****************************************************************************/

static int cle_getcursor(FAR struct cle_s *priv, FAR uint16_t *prow,
                         FAR uint16_t *pcolumn)
{
  uint32_t row;
  uint32_t column;
  int nbad;
  int ch;

  /* Send the VT100 GETCURSOR command */

  cle_write(priv, g_getcursor, sizeof(g_getcursor));

  /* We expect to get back ESC[v;hR where v is the row and h is the column.
   * once the sequence has started we don't expect any characters
   * interspersed.
   */

  for (nbad = 0; nbad < 10; nbad++)
    {
      /* Look for initial ESC */

      ch = cle_getch(priv);
      if (ch != ASCII_ESC)
        {
          continue;
        }

      /* Have ESC, now we expect '[' */

      ch = cle_getch(priv);
      if (ch != '[')
        {
          continue;
        }

      /* ...now we expect to see a numeric value terminated with ';' */

      row = 0;

      while (isdigit(ch = cle_getch(priv)))
        {
          row = row * 10 + (ch - '0');
        }

      if (ch != ';')
        {
          continue;
        }

      /* ...now we expect to see another numeric value terminated with 'R' */

      column = 0;
      while (isdigit(ch = cle_getch(priv)))
        {
          column = 10 * column + (ch - '0');
        }

      /* ...we are done */

      cleinfo("row=%" PRId32 " column=%" PRId32 "\n", row, column);

      /* Make sure that the values are within range */

      if (row <= UINT16_MAX && column <= UINT16_MAX)
        {
          *prow = row;
          *pcolumn = column;
          return OK;
        }
      else
        {
          return -ERANGE;
        }
    }

  return -EINVAL;
}

/****************************************************************************
 * Name: cle_clrtoeol
 *
 * Description:
 *   Clear the display from the current cursor position to the end of the
 *   current line.
 *
 ****************************************************************************/

static void cle_clrtoeol(FAR struct cle_s *priv)
{
  /* Send the VT100 ERASETOEOL command */

  cle_write(priv, g_erasetoeol, sizeof(g_erasetoeol));
}

/****************************************************************************
 * Name: cle_opentext
 *
 * Description:
 *   Make space for new text of size 'increment' at the specified cursor
 *   position.  This function will not allow text grow beyond ('linelen' - 1)
 *   in size.
 *
 ****************************************************************************/

static bool cle_opentext(FAR struct cle_s *priv, uint16_t pos,
                         uint16_t increment)
{
  int i;

  cleinfo("pos=%ld increment=%ld\n", (long)pos, (long)increment);

  /* Check if there is space in the line buffer to open up a region the size
   * of 'increment'
   */

  if (priv->nchars + increment >= priv->linelen)
    {
      CLE_BEL(priv);
      return false;
    }

  /* Move text to make space for new text of size 'increment' at the current
   * cursor position
   */

  for (i = priv->nchars - 1; i >= pos; i--)
    {
      priv->line[i + increment] = priv->line[i];
    }

  /* Adjust end of file position */

  priv->nchars += increment;
  return true;
}

/****************************************************************************
 * Name: cle_closetext
 *
 * Description:
 *   Delete a region in the line buffer by copying the end of the line buffer
 *   over the deleted region and adjusting the size of the region.
 *
 ****************************************************************************/

static void cle_closetext(FAR struct cle_s *priv, uint16_t pos,
                          uint16_t size)
{
  int i;

  cleinfo("pos=%ld size=%ld\n", (long)pos, (long)size);

  /* Close up the gap to remove 'size' characters at 'pos' */

  for (i = pos + size; i < priv->nchars; i++)
    {
      priv->line[i - size] = priv->line[i];
    }

  /* Adjust sizes and positions */

  priv->nchars -= size;

  if (priv->curpos > pos)
    {
      /* Check if the cursor position is beyond the deleted region */

      if (priv->curpos - pos > size)
        {
          /* Yes... just subtract the size of the deleted region */

          priv->curpos -= size;
        }
      else
        {
          /* What if the position is within the deleted region?  Set it to
           * the beginning of the deleted region.
           */

        priv->curpos = pos;
        }
    }
}

/****************************************************************************
 * Name: cle_showtext
 *
 * Description:
 *   Update the display based on the last operation.  This function is
 *   called at the beginning of the editor loop.
 *
 ****************************************************************************/

static void cle_showtext(FAR struct cle_s *priv)
{
  uint16_t column;
  uint16_t tabcol;

  /* Turn off the cursor during the update. */

  cle_cursoroff(priv);

  /* Set the cursor position to the beginning of this row */

#ifdef CONFIG_SYSTEM_COLOR_CLE
  cle_setcolor(priv, COLOR_COMMAND);
#endif
  cle_setcursor(priv, 0);
  cle_clrtoeol(priv);

  /* Loop for each column */

  for (column = 0; column < priv->nchars; )
    {
      /* Perform TAB expansion */

      if (priv->line[column] == '\t')
        {
          tabcol = NEXT_TAB(column);
          if (tabcol < priv->linelen)
            {
              for (; column < tabcol; column++)
                {
                  cle_putch(priv, ' ');
                }
            }
          else
            {
              /* Break out of the loop... there is nothing left on the
               * line but whitespace.
               */

              break;
            }
        }

      /* Add the normal character to the display */

      else
        {
          cle_putch(priv, priv->line[column]);
          column++;
        }
    }

#ifdef CONFIG_SYSTEM_COLOR_CLE
  cle_setcolor(priv, COLOR_OUTPUT);
#endif

  /* Turn the cursor back on */

  cle_cursoron(priv);
}

/****************************************************************************
 * Name: cle_insertch
 *
 * Description:
 *   Insert one character into the line buffer
 *
 ****************************************************************************/

static void cle_insertch(FAR struct cle_s *priv, char ch)
{
  cleinfo("curpos=%" PRId16 " ch=%c[%02x]\n", priv->curpos,
          isprint(ch) ? ch : '.', ch);

  /* Make space in the buffer for the new character */

  if (cle_opentext(priv, priv->curpos, 1))
    {
      /* Add the new character to the buffer */

      priv->line[priv->curpos++] = ch;
    }
}

/****************************************************************************
 * Name: cle_editloop
 *
 * Description:
 *   Command line editor loop
 *
 ****************************************************************************/

static int cle_editloop(FAR struct cle_s *priv)
{
  /* Loop while we are in command mode */

  for (; ; )
    {
#if  1 /* Perhaps here should be a config switch */
      char state = 0;
#endif
      int ch;

      /* Make sure that the display reflects the current state */

      cle_showtext(priv);
      cle_setcursor(priv, priv->curpos);

      /* Get the next character from the input */

#if  1 /* Perhaps here should be a config switch */
      /* Simple decode of some VT100/xterm codes: left/right, up/dn,
       * home/end, del
       */

      /* loop till we have a ch */

      for (; ; )
        {
          ch = cle_getch(priv);
          if (ch < 0)
            {
              return -EIO;
            }
          else if (state != 0)
            {
              if (state == (char)1)  /* Got ESC */
                {
                  if (ch == '[' || ch == 'O')
                    {
                      state = ch;
                    }
                  else
                    {
                      break;  /* break the for loop */
                    }
                }
              else if (state == '[')
                {
                  /* Got ESC[ */

                  switch (ch)
                    {
                      case '3':    /* ESC[3~  = DEL */
                        {
                          state = ch;
                          continue;
                        }

                      case 'A':
                        {
                          ch = KEY_UP;
                        }
                        break;

                      case 'B':
                        {
                          ch = KEY_DN;
                        }
                        break;

                      case 'C':
                        {
                          ch = KEY_RIGHT;
                        }
                        break;

                      case 'D':
                        {
                          ch = KEY_LEFT;
                        }
                        break;

                      case 'F':
                        {
                          ch = KEY_ENDLINE;
                        }
                        break;

                      case 'H':
                        {
                          ch = KEY_BEGINLINE;
                        }
                        break;

                      default:
                        break;
                    }
                  break;  /* Break the 'for' loop */
                }
              else if (state == 'O')
                {
                  /* got ESCO */

                  if (ch == 'F')
                    {
                      ch = KEY_ENDLINE;
                    }

                  break; /* Break the 'for' loop */
               }
              else if (state == '3')
                {
                  if (ch == '~')
                    {
                      ch = KEY_DEL;
                    }

                  break; /* Break the 'for' loop */
                }
              else
                {
                  break; /* Break the 'for' loop */
                }
            }
          else if (ch == ASCII_ESC)
            {
              ++state;
            }
          else
            {
              break; /* Break the 'for' loop, use the char */
            }
        }

#else
      ch = cle_getch(priv);
      if (ch < 0)
        {
          return -EIO;
        }

#endif

      /* Then handle the character. */

#ifdef CONFIG_SYSTEM_CLE_CMD_HISTORY
      if (g_cmd_history_len > 0)
        {
          int i = 1;

          switch (ch)
            {
              case KEY_UP:

                /* Go to the past command in history */

                g_cmd_history_steps_from_head--;

                if (-g_cmd_history_steps_from_head >= g_cmd_history_len)
                  {
                    g_cmd_history_steps_from_head = -(g_cmd_history_len - 1);
                  }

                break;

              case KEY_DN:

                /* Go to the recent command in history */

                g_cmd_history_steps_from_head++;

                if (g_cmd_history_steps_from_head > 1)
                  {
                    g_cmd_history_steps_from_head = 1;
                  }

                break;

              default:
                i = 0;
                break;
            }

          if (i != 0)
            {
              priv->nchars = 0;
              priv->curpos = 0;

              if (g_cmd_history_steps_from_head != 1)
                {
                  int idx = g_cmd_history_head +
                            g_cmd_history_steps_from_head;

                  /* Circular buffer wrap around */

                  if (idx < 0)
                    {
                      idx = idx + CONFIG_SYSTEM_CLE_CMD_HISTORY_LEN;
                    }
                  else if (idx >= CONFIG_SYSTEM_CLE_CMD_HISTORY_LEN)
                    {
                      idx = idx - CONFIG_SYSTEM_CLE_CMD_HISTORY_LEN;
                    }

                  for (i = 0; g_cmd_history[idx][i] != '\0'; i++)
                    {
                      cle_insertch(priv, g_cmd_history[idx][i]);
                    }

                  priv->curpos = priv->nchars;
                }

              continue;
            }
        }
#endif /* CONFIG_SYSTEM_CLE_CMD_HISTORY */

      switch (ch)
        {
        case KEY_BEGINLINE: /* Move cursor to start of current line */
          {
            priv->curpos = 0;
          }
          break;

        case KEY_LEFT: /* Move the cursor left 1 character */
          {
            if (priv->curpos > 0)
              {
                priv->curpos--;
              }
            else
              {
                CLE_BEL(priv);
              }
          }
          break;

        case KEY_DEL: /* Delete 1 character at the cursor */
          {
            if (priv->curpos < priv->nchars)
              {
                cle_closetext(priv, priv->curpos, 1);
              }
            else
              {
                CLE_BEL(priv);
              }
          }
          break;

        case KEY_ENDLINE: /* Move cursor to end of current line */
          {
            priv->curpos = priv->nchars;
          }
          break;

        case KEY_RIGHT: /* Move the cursor right one character */
          {
            if (priv->curpos < priv->nchars)
              {
                priv->curpos++;
              }
            else
              {
                CLE_BEL(priv);
              }
          }
          break;

        case ASCII_DEL:
        case KEY_DELLEFT:  /* Delete 1 character before the cursor */
          {
            if (priv->curpos > 0)
              {
                cle_closetext(priv, --priv->curpos, 1);
              }
            else
              {
                CLE_BEL(priv);
              }
          }
          break;

        case KEY_DELEOL:  /* Delete to the end of the line */
          {
            priv->nchars = (priv->nchars > 0 ? priv->curpos : 0);
          }
          break;

        case KEY_DELLINE:  /* Delete to the end of the line */
          {
            priv->nchars = 0;
            priv->curpos = 0;
          }
          break;

        case KEY_CLRSCR: /* Clear the screen & return cursor to top */
          cle_clrscr(priv);
          break;

        case KEY_QUOTE: /* Quoted character follows */
          {
            ch = cle_getch(priv);
            if (ch < 0)
              {
                return -EIO;
              }

            /* Insert the next character unconditionally */

            cle_insertch(priv, ch);
          }
          break;

        /* Newline terminates editing.  But what is a newline? */

#if defined(CONFIG_EOL_IS_CR) || defined(CONFIG_EOL_IS_EITHER_CRLF)
        case '\r': /* CR terminates line */

#elif defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF) || \
      defined(CONFIG_EOL_IS_EITHER_CRLF)

        case '\n': /* LF terminates line */
#endif
          {
            /* Add the newline to the buffer at the end of the line */

            priv->curpos = priv->nchars;
            cle_insertch(priv, '\n');
            cle_putch(priv, '\n');
            return OK;
          }
          break;

#if defined(CONFIG_EOL_IS_BOTH_CRLF)
        case '\r': /* Wait for the LF */
          break;
#endif

        /* Text to insert or unimplemented/invalid keypresses */

        default:
          {
            /* Ignore all control characters except for tab and newline */

            if (!iscntrl(ch) || ch == '\t')
              {
                /* Insert the filtered character into the buffer */

                cle_insertch(priv, ch);
              }
            else
              {
                CLE_BEL(priv);
              }
          }
          break;
        }
    }

  return OK;
}

/****************************************************************************
 * Command line processing
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cle/cle_fd
 *
 * Description:
 *   EMACS-like command line editor.  This is actually more like readline
 *   than is the NuttX readline!
 *
 ****************************************************************************/

int cle_fd(FAR char *line, FAR const char *prompt, uint16_t linelen,
           int infd, int outfd)
{
  FAR struct cle_s priv;
  uint16_t column;
  int ret;

  /* Initialize the CLE state structure */

  memset(&priv, 0, sizeof(struct cle_s));

  priv.linelen  = linelen;
  priv.line     = line;

  priv.infd     = infd;
  priv.outfd    = outfd;

  /* Store the prompt in case we need to re-print it */

  priv.prompt = prompt;
  cle_outputprompt(&priv);

  /* Get the current cursor position */

  ret = cle_getcursor(&priv, &priv.row, &column);

  if (ret < 0)
    {
      return ret;
    }

  /* Turn the column number into an offset */

  if (column < 1)
    {
      return -EINVAL;
    }

  priv.coloffs = column;

  cleinfo("row=%d column=%d\n", priv.row, column);

  /* The editor loop */

  ret = cle_editloop(&priv);

  /* Make sure that the line is NUL terminated */

  line[priv.nchars] = '\0';

#ifdef CONFIG_SYSTEM_CLE_CMD_HISTORY
  /* Save history of command, only if there was something typed besides
   * return character.
   */

  if (priv.nchars > 1)
    {
      int i;

      g_cmd_history_head =
        (g_cmd_history_head + 1) % CONFIG_SYSTEM_CLE_CMD_HISTORY_LEN;

      for (i = 0;
           (i < priv.nchars - 1) &&
            i < (CONFIG_SYSTEM_CLE_CMD_HISTORY_LINELEN - 1);
           i++)
        {
          g_cmd_history[g_cmd_history_head][i] = line[i];
        }

      g_cmd_history[g_cmd_history_head][i] = '\0';
      g_cmd_history_steps_from_head        = 1;

      if (g_cmd_history_len < CONFIG_SYSTEM_CLE_CMD_HISTORY_LEN)
        {
          g_cmd_history_len++;
        }
    }
#endif /* CONFIG_SYSTEM_CLE_CMD_HISTORY */

  return ret;
}

#ifdef CONFIG_FILE_STREAM
int cle(FAR char *line, FAR const char *prompt, uint16_t linelen,
        FAR FILE *instream, FAR FILE *outstream)
{
  return cle_fd(line, prompt, linelen, instream->fs_fd, outstream->fs_fd);
}
#endif
