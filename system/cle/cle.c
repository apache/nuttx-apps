/****************************************************************************
 * apps/system/cle/cle.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

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

#include <apps/cle.h>

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

#define CLE_BEL(cle)    cle_putch(cle,CTRL('G'))

/* Sizes of things */

#define TABSIZE         8    /* A TAB is eight characters */
#define TABMASK         7    /* Mask for TAB alignment */
#define NEXT_TAB(p)     (((p) + TABSIZE) & ~TABMASK)

/* Debug */

#ifndef CONFIG_SYSEM_CLE_DEBUGLEVEL
#  define CONFIG_SYSEM_CLE_DEBUGLEVEL 0
#endif

#ifdef CONFIG_CPP_HAVE_VARARGS
#  if CONFIG_SYSEM_CLE_DEBUGLEVEL > 0
#    define vidbg(format, arg...) \
       syslog(EXTRA_FMT format EXTRA_ARG, ##arg)
#    define vvidbg(format, ap) \
       vsyslog(format, ap)
#  else
#    define vidbg(x...)
#    define vvidbg(x...)
#  endif

#  if CONFIG_SYSEM_CLE_DEBUGLEVEL > 1
#    define vivdbg(format, arg...) \
       syslog(EXTRA_FMT format EXTRA_ARG, ##arg)
#  else
#    define vivdbg(x...)
#  endif
#else
#  if CONFIG_SYSEM_CLE_DEBUGLEVEL > 0
#    define vidbg  syslog
#    define vvidbg vsyslog
#  else
#    define vidbg  (void)
#    define vvidbg (void)
#  endif

#  if CONFIG_SYSEM_CLE_DEBUGLEVEL > 1
#    define vivdbg syslog
#  else
#    define vivdbg (void)
#  endif
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
/* VI Key Bindings */

enum cle_key_e
{
  KEY_BEGINLINE   = CTRL('A'),  /* Move cursor to start of current line */
  KEY_LEFT        = CTRL('B'),  /* Move left one character */
  KEY_DEL         = CTRL('D'),  /* Delete a single character at the cursor position*/
  KEY_ENDLINE     = CTRL('E'),  /* Move cursor to end of current line */
  KEY_RIGHT       = CTRL('F'),  /* Move right one character */
  KEY_DELLEFT     = CTRL('H'),  /* Delete character, left (backspace)  */
  KEY_DELEOL      = CTRL('K'),  /* Delete to the end of the line */
  KEY_DELLINE     = CTRL('U'),  /* Delete the entire line */
  KEY_QUOTE       = '\\',       /* The next character is quote (use literal value) */
};

/* This structure describes the overall state of the editor */

struct cle_s
{
  uint16_t curpos;          /* Current cursor position */
  uint16_t cursave;         /* Saved cursor position */
  uint16_t row;             /* This is the row that we are editing iun */
  uint16_t coloffs;         /* Left cursor offset */
  uint16_t linelen;         /* Size of the line buffer */
  uint16_t nchars;          /* Size of data in the line buffer */
  int infd;                 /* Input file descriptor */
  int outfd;                /* Output file descriptor */
  FAR char *line;           /* Line buffer */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Low-level display and data entry functions */

static void     cle_write(FAR struct cle_s *cle, FAR const char *buffer,
                  uint16_t buflen);
static void     cle_putch(FAR struct cle_s *cle, char ch);
static int      cle_getch(FAR struct cle_s *cle);
static void     cle_cursoron(FAR struct cle_s *cle);
static void     cle_cursoroff(FAR struct cle_s *cle);
static void     cle_setcursor(FAR struct cle_s *cle, uint16_t column);
static int      cle_getcursor(FAR struct cle_s *cle, uint16_t *prow,
                  uint16_t *pcolumn);
static void     cle_clrtoeol(FAR struct cle_s *cle);

/* Editor function */

static bool     cle_opentext(FAR struct cle_s *cle, uint16_t pos,
                  uint16_t increment);
static void     cle_closetext(FAR struct cle_s *cle, uint16_t pos, uint16_t size);
static void     cle_showtext(FAR struct cle_s *cle);
static void     cle_insertch(FAR struct cle_s *cle, char ch);
static int      cle_editloop(FAR struct cle_s *cle);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* VT100 escape sequences */

static const char g_cursoron[]      = VT100_CURSORON;
static const char g_cursoroff[]     = VT100_CURSOROFF;
static const char g_getcursor[]     = VT100_GETCURSOR;
static const char g_erasetoeol[]    = VT100_CLEAREOL;
static const char g_attriboff[]     = VT100_MODESOFF;
static const char g_fmtcursorpos[]  = VT100_FMT_CURSORPOS;

/* Error format strings */

static const char g_fmtnotvalid[]   = "Command not valid";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Low-level display and data entry functions
 ****************************************************************************/

/****************************************************************************
 * Name: cle_write
 *
 * Description:
 *   Write a sequence of bytes to the console device (stdout, fd = 1).
 *
 ****************************************************************************/

static void cle_write(FAR struct cle_s *cle, FAR const char *buffer,
                     uint16_t buflen)
{
  ssize_t nwritten;
  uint16_t  nremaining = buflen;

  //vivdbg("buffer=%p buflen=%d\n", buffer, (int)buflen);

  /* Loop until all bytes have been successuflly written (or until a
   * un-recoverable error is encountered)
   */

  do
    {
      /* Take the next gulp */

      nwritten = write(cle->outfd, buffer, buflen);

      /* Handle write errors.  write() should neve return 0. */

      if (nwritten <= 0)
        {
          /* EINTR is not really an error; it simply means that a signal was
           * received while waiting for write.
           */

          int errcode = errno;
          if (nwritten == 0 || errcode != EINTR)
            {
              vidbg("ERROR: write to stdout failed: %d\n", errcode);
              return;
            }
        }

      /* Decrement the count of bytes remaining to be sent (to handle the
       * case of a partial write)
       */

      else
        {
          nremaining -= nwritten;
        }
    }
  while (nremaining > 0);
}

/****************************************************************************
 * Name: cle_putch
 *
 * Description:
 *   Write a single character to the console device.
 *
 ****************************************************************************/

static void cle_putch(FAR struct cle_s *cle, char ch)
{
  cle_write(cle, &ch, 1);
}

/****************************************************************************
 * Name: cle_getch
 *
 * Description:
 *   Get a single character from the console device (stdin, fd = 0).
 *
 ****************************************************************************/

static int cle_getch(FAR struct cle_s *cle)
{
  char buffer;
  ssize_t nread;

  /* Loop until we successfully read a character (or until an unexpected
   * error occurs).
   */

  do
    {
      /* Read one character from the incoming stream */

      nread = read(cle->infd, &buffer, 1);

      /* Check for error or end-of-file. */

      if (nread <= 0)
        {
          /* EINTR is not really an error; it simply means that a signal we
           * received while waiting for input.
           */

          int errcode = errno;
          if (nread == 0 || errcode != EINTR)
            {
              vidbg("ERROR: read from stdin failed: %d\n", errcode);
              return -EIO;
            }
        }
    }
  while (nread < 1);

  /* On success, return the character that was read */

  vivdbg("Returning: %c[%02x]\n", isprint(buffer) ? buffer : '.', buffer);
  return buffer;
}

/****************************************************************************
 * Name: cle_cursoron
 *
 * Description:
 *   Turn on the cursor
 *
 ****************************************************************************/

static void cle_cursoron(FAR struct cle_s *cle)
{
  /* Send the VT100 CURSORON command */

  cle_write(cle, g_cursoron, sizeof(g_cursoron));
}

/****************************************************************************
 * Name: cle_cursoroff
 *
 * Description:
 *   Turn off the cursor
 *
 ****************************************************************************/

static void cle_cursoroff(FAR struct cle_s *cle)
{
  /* Send the VT100 CURSOROFF command */

  cle_write(cle, g_cursoroff, sizeof(g_cursoroff));
}

/****************************************************************************
 * Name: cle_setcursor
 *
 * Description:
 *   Move the current cursor position to position (row,col)
 *
 ****************************************************************************/

static void cle_setcursor(FAR struct cle_s *cle, uint16_t column)
{
  char buffer[16];
  int len;

  vivdbg("row=%d column=%d\n", row, column);

  /* Format the cursor position command.  The origin is (1,1). */

  len = snprintf(buffer, 16, g_fmtcursorpos, cle->row, column + cle->coloffs);

  /* Send the VT100 CURSORPOS command */

  cle_write(cle, buffer, len);
}

/****************************************************************************
 * Name: cle_getcursor
 *
 * Description:
 *   Get the current cursor position.
 *
 ****************************************************************************/

static int cle_getcursor(FAR struct cle_s *cle, FAR uint16_t *prow,
                        FAR uint16_t *pcolumn)
{
  uint32_t row;
  uint32_t column;
  int nbad;
  int ch;

  /* Send the VT100 GETCURSOR command */

  cle_write(cle, g_getcursor, sizeof(g_getcursor));

  /* We expect to get ESCv;hR where v is the row and h is the column */

  nbad = 0;
  for (;;)
    {
      /* Get the next character from the input */

      ch = cle_getch(cle);
      if (ch == ASCII_ESC)
        {
          break;
        }
      else if (ch < 0)
        {
          return -EIO;
        }
      else if (++nbad > 3)
        {
          /* We are probably talking to a non-VT100 terminal! */

          return -EINVAL;
        }
    }

  /* Now we expect to see a numeric value terminated with ';' */

  row = 0;
  for (;;)
    {
      /* Get the next character from the input */

      ch = cle_getch(cle);
      if (isdigit(ch))
        {
          row = 10*row + (ch - '0');
        }
      else if (ch == ';')
        {
          break;
        }
      else if (ch < 0)
        {
          return -EIO;
        }
      else
        {
          return -EINVAL;
        }
    }

  /* Now we expect to see another numeric value terminated with 'R' */

  column = 0;
  for (;;)
    {
      /* Get the next character from the input */

      ch = cle_getch(cle);
      if (isdigit(ch))
        {
          column = 10*column + (ch - '0');
        }
      else if (ch == 'R')
        {
          break;
        }
      else if (ch < 0)
        {
          return -EIO;
        }
      else
        {
          return -EINVAL;
        }
    }

  vivdbg("row=%ld column=%ld\n", row, column);

  /* Make sure that the values are within range */

  if (row <= UINT16_MAX && column <= UINT16_MAX)
    {
      *prow    = row;
      *pcolumn = column;
      return OK;
    }

  return -ERANGE;
}

/****************************************************************************
 * Name: cle_clrtoeol
 *
 * Description:
 *   Clear the display from the current cursor position to the end of the
 *   current line.
 *
 ****************************************************************************/

static void cle_clrtoeol(FAR struct cle_s *cle)
{
  /* Send the VT100 ERASETOEOL command */

  cle_write(cle, g_erasetoeol, sizeof(g_erasetoeol));
}

/****************************************************************************
 * Name: cle_opentext
 *
 * Description:
 *   Make space for new text of size 'increment' at the specified cursor
 *   position.
 *
 ****************************************************************************/

static bool cle_opentext(FAR struct cle_s *cle, uint16_t pos, uint16_t increment)
{
  int i;

  vivdbg("pos=%ld increment=%ld\n", (long)pos, (long)increment);

  /* Check if there is space in the line buffer to open up a region the size
   * of 'increment'
   */

  if (cle->nchars + increment > cle->linelen)
    {
      return false;
    }

  /* Move text to make space for new text of size 'increment' at the current
   * cursor position
   */

  for (i = cle->nchars - 1; i >= pos; i--)
    {
      cle->line[i + increment] = cle->line[i];
    }

  /* Adjust end of file position */

  cle->nchars += increment;
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

static void cle_closetext(FAR struct cle_s *cle, uint16_t pos, uint16_t size)
{
  int i;

  vivdbg("pos=%ld size=%ld\n", (long)pos, (long)size);

  /* Close up the gap to remove 'size' characters at 'pos' */

  for (i = pos + size; i < cle->nchars; i++)
    {
      cle->line[i - size] = cle->line[i];
    }

  /* Adjust sizes and positions */

  cle->nchars -= size;

  /* Check if the cursor position is beyond the deleted region */

  if (cle->curpos > pos + size)
    {
      /* Yes... just subtract the size of the deleted region */

      cle->curpos -= size;
    }

  /* What if the position is within the deleted region?  Set it to the
   * beginning of the deleted region.
   */

  else if (cle->curpos > pos)
    {
      cle->curpos = pos;
    }
}

/****************************************************************************
 * Name: cle_showtext
 *
 * Description:
 *   Update the display based on the last operation.  This function is
 *   called at the beginning of the processing loop in Command and Insert
 *   modes (and also in the continuous replace mode).
 *
 ****************************************************************************/

static void cle_showtext(FAR struct cle_s *cle)
{
  uint16_t column;
  uint16_t tabcol;

  /* Turn off the cursor during the update. */

  cle_cursoroff(cle);

  /* Set the cursor position to the beginning of this row */

  cle_setcursor(cle, 0);
  cle_clrtoeol(cle);

  /* Loop for each column */

  for (column = 0; column < cle->nchars && column < cle->linelen; column++)
    {
      /* Perform TAB expansion */

      if (cle->line[column] == '\t')
        {
          tabcol = NEXT_TAB(column);
          if (tabcol < cle->linelen)
            {
              for (; column < tabcol; column++)
                {
                  cle_putch(cle, ' ');
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
          cle_putch(cle, cle->line[column]);
          column++;
        }
    }

  /* Turn the cursor back on */

  cle_cursoron(cle);
}

/****************************************************************************
 * Name: cle_insertch
 *
 * Description:
 *   Insert one character into the line buffer
 *
 ****************************************************************************/

static void cle_insertch(FAR struct cle_s *cle, char ch)
{
  vivdbg("curpos=%ld ch=%c[%02x]\n", cle->curpos, isprint(ch) ? ch : '.', ch);

  /* Make space in the buffer for the new character */

  if (cle_opentext(cle, cle->curpos, 1))
    {
      /* Add the new character to the buffer */

      cle->line[cle->curpos++] = ch;
    }
}

/****************************************************************************
 * Name: cle_editloop
 *
 * Description:
 *   Command line editor loop
 *
 ****************************************************************************/

static int cle_editloop(FAR struct cle_s *cle)
{
  /* Loop while we are in command mode */

  for (;;)
    {
      int ch;

      /* Make sure that the display reflects the current state */

      cle_showtext(cle);
      cle_setcursor(cle, cle->curpos);

      /* Get the next character from the input */

      ch = cle_getch(cle);
      if (ch < 0)
        {
          return -EIO;
        }

      /* Then handle the character. */

      switch (ch)
        {
        case KEY_BEGINLINE: /* Move cursor to start of current line */
          {
            cle->curpos = 0;
          }
          break;

        case KEY_LEFT: /* Move the cursor left 1 character */
          {
            if (cle->curpos > 0)
              {
                cle->curpos--;
              }
            else
              {
                CLE_BEL(cle);
              }
          }
          break;

        case KEY_DEL: /* Delete 1 character at the cursor */
        case ASCII_DEL:
          {
            if (cle->curpos < cle->nchars)
              {
                cle_closetext(cle, cle->curpos, 1);
              }
            else
              {
                CLE_BEL(cle);
              }
          }
          break;

        case KEY_ENDLINE: /* Move cursor to end of current line */
          {
            if (cle->nchars > 0)
              {
                cle->curpos = cle->nchars - 1;
              }
            else
              {
                CLE_BEL(cle);
              }
          }
          break;

        case KEY_RIGHT: /* Move the cursor right one character */
          {
            if (cle->curpos < cle->nchars)
              {
                cle->curpos++;
              }
            else
              {
                CLE_BEL(cle);
              }
          }
          break;

        case KEY_DELLEFT: /* Delete 1 character before the cursor */
        //case ASCII_BS:
          {
            if (cle->curpos > 0)
              {
                cle_closetext(cle, --cle->curpos, 1);
              }
            else
              {
                CLE_BEL(cle);
              }
          }
          break;

        case KEY_DELEOL:  /* Delete to the end of the line */
          {
            cle->nchars = (cle->nchars > 0 ? cle->curpos + 1 : 0);
          }
          break;

        case KEY_DELLINE:  /* Delete to the end of the line */
          {
            cle->nchars = 0;
            cle->curpos = 0;
          }
          break;

        case KEY_QUOTE: /* Quoted character follows */
          {
            ch = cle_getch(cle);
            if (ch < 0)
              {
                return -EIO;
              }

            /* Insert the next character unconditionally */

            cle_insertch(cle, ch);
          }
          break;

        /* Newline terminates editing */

#if defined(CONFIG_EOL_IS_CR)
        case '\r': /* CR terminates line */
          {
            return OK;
          }
          break;

#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
        case '\r': /* Wait for the LF */
          break;
#endif

#if defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
        case '\n': /* LF terminates line */
          {
            return OK;
          }
          break;
#endif

#ifdef CONFIG_EOL_IS_EITHER_CRLF
        case '\r': /* Ether CR or LF terminates line */
        case '\n':
          {
            return OK;
          }
          break;
#endif
        /* Text to insert or unimplemented/invalid keypresses */

        default:
          {
            /* Ignore all control characters except for tab and newline */

            if (!iscntrl(ch) || ch == '\t')
              {
                /* Insert the filtered character into the buffer */

                cle_insertch(cle, ch);
              }
            else
              {
                CLE_BEL(cle);
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
 * Name: cle
 *
 * Description:
 *   EMACS-like command line editor.  This is actually more like readline
 *   than is the NuttX readline!
 *
 ****************************************************************************/

int cle(FAR char *line, uint16_t linelen, FILE *instream, FILE *outstream)
{
  FAR struct cle_s cle;
  uint16_t column;
  int ret;

  /* Initialize the cle state structure */

  cle.linelen  = linelen;
  cle.line     = line;

  /* REVISIT:  Non-standard, non-portable */

  cle.infd     = instream->fs_fd;
  cle.outfd    = outstream->fs_fd;

  /* Get the current cursor position */

  ret = cle_getcursor(&cle, &cle.row, &column);
  if (ret < 0)
    {
      return ret;
    }

  /* Turn the column number into an offset */

  if (column < 1)
    {
      return -EINVAL;
    }

  cle.coloffs = column - 1;

  /* The editor loop */

  return cle_editloop(&cle);
}
