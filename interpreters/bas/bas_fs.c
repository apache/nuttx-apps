/****************************************************************************
 * apps/interpreters/bas/bas_fs.c
 * BASIC file system interface.
 *
 *   Copyright (c) 1999-2014 Michael Haardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Adapted to NuttX and re-released under a 3-clause BSD license:
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Authors: Alan Carvalho de Assis <Alan Carvalho de Assis>
 *            Gregory Nutt <gnutt@nuttx.org>
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

#include <sys/time.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/ascii.h>
#include <nuttx/vt100.h>

#include "bas_vt100.h"
#include "bas_fs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINEWIDTH 80
#define COLWIDTH  14

#define _(String) String

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct FileStream **g_file;
static int g_capacity;
static int g_used;
static const int g_open_mode[4] = { 0, O_RDONLY, O_WRONLY, O_RDWR };
static char g_errmsgbuf[80];

#ifdef CONFIG_INTERPRETER_BAS_VT100
static const uint8_t g_vt100_colormap[8] =
{
  VT100_BLACK, VT100_BLUE,   VT100_GREEN,  VT100_CYAN,
  VT100_RED,  VT100_MAGENTA, VT100_YELLOW, VT100_WHITE
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

const char *FS_errmsg;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int size(int dev)
{
  if (dev >= g_capacity)
    {
      int i;
      struct FileStream **n;

      n = (struct FileStream **)
        realloc(g_file, (dev + 1) * sizeof(struct FileStream *));
      if (n == (struct FileStream **)0)
        {
          FS_errmsg = strerror(errno);
          return -1;
        }

      g_file = n;
      for (i = g_capacity; i <= dev; ++i)
        {
          g_file[i] = (struct FileStream *)0;
        }

      g_capacity = dev + 1;
    }

  return 0;
}

static int opened(int dev, int mode)
{
  int fd = -1;

  if (dev < 0 || dev >= g_capacity || g_file[dev] == (struct FileStream *)0)
    {
      snprintf(g_errmsgbuf, sizeof(g_errmsgbuf), _("channel #%d not open"),
               dev);
      FS_errmsg = g_errmsgbuf;
      return -1;
    }

  if (mode == -1)
    {
      return 0;
    }

  switch (mode)
    {
    case 0:
      {
        fd = g_file[dev]->outfd;
        if (fd == -1)
          {
            snprintf(g_errmsgbuf, sizeof(g_errmsgbuf),
                     _("channel #%d not opened for writing"), dev);
          }
        break;
      }

    case 1:
      {
        fd = g_file[dev]->infd;
        if (fd == -1)
          {
            snprintf(g_errmsgbuf, sizeof(g_errmsgbuf),
                     _("channel #%d not opened for reading"), dev);
          }
        break;
      }

    case 2:
      {
        fd = g_file[dev]->randomfd;
        if (fd == -1)
          {
            snprintf(g_errmsgbuf, sizeof(g_errmsgbuf),
                     _("channel #%d not opened for random access"), dev);
          }
        break;
      }

    case 3:
      {
        fd = g_file[dev]->binaryfd;
        if (fd == -1)
          {
            snprintf(g_errmsgbuf, sizeof(g_errmsgbuf),
                     _("channel #%d not opened for binary access"), dev);
          }
        break;
      }

    case 4:
      {
        fd = (g_file[dev]->randomfd != -1 ? g_file[dev]->randomfd : g_file[dev]->binaryfd);
        if (fd == -1)
          {
            snprintf(g_errmsgbuf, sizeof(g_errmsgbuf),
                     _("channel #%d not opened for random or binary access"),
                     dev);
          }
        break;
      }

    default:
      assert(0);
    }

  if (fd == -1)
    {
      FS_errmsg = g_errmsgbuf;
      return -1;
    }
  else
    {
      return 0;
    }
}

static int refill(int dev)
{
  struct FileStream *f;
  ssize_t len;

  f = g_file[dev];
  f->inSize = 0;
  len = read(f->infd, f->inBuf, sizeof(f->inBuf));
  if (len <= 0)
    {
      f->inCapacity = 0;
      FS_errmsg = (len == -1 ? strerror(errno) : (const char *)0);
      return -1;
    }
  else
    {
      f->inCapacity = len;
      return 0;
    }
}

static int edit(int chn, int onl)
{
  struct FileStream *f = g_file[chn];
  char *buf = f->inBuf;
  char ch;
  int r;

  for (buf = f->inBuf; buf < (f->inBuf + f->inCapacity); ++buf)
    {
      if (*buf >= '\0' && *buf < ' ')
        {
          FS_putChar(chn, '^');
          FS_putChar(chn, *buf ? (*buf + 'a' - 1) : '@');
        }
      else
        {
          FS_putChar(chn, *buf);
        }
    }
  do
    {
      FS_flush(chn);
      if ((r = read(f->infd, &ch, 1)) == -1)
        {
          f->inCapacity = 0;
          FS_errmsg = strerror(errno);
          return -1;
        }
      else if (r == 0 || (f->inCapacity == 0 && ch == 4))
        {
          FS_errmsg = (char *)0;
          return -1;
        }

      /* Check for backspace
       *
       * There are several notions of backspace, for an elaborate summary see
       * http://www.ibb.net/~anne/keyboard.html. There is no clean solution.
       * Here both DEL and backspace are treated like backspace here.  The
       * Unix/Linux screen terminal by default outputs  DEL (0x7f) when the
       * backspace key is pressed.
       */

      if (ch == ASCII_BS || ch == ASCII_DEL)
        {
          if (f->inCapacity)
            {
#ifdef CONFIG_INTERPRETER_BAS_VT100
              /* Could use vt100_clrtoeol */
#endif
              /* Is the previous character in the buffer 2 character escape sequence? */

              if (f->inBuf[f->inCapacity - 1] >= '\0' &&
                  f->inBuf[f->inCapacity - 1] < ' ')
                {
                  /* Yes.. erase two characters */

                  FS_putChars(chn, "\b\b  \b\b");
                }
              else
                 {
                  /* Yes.. erase one characters */

                  FS_putChars(chn, "\b \b");
                }

              --f->inCapacity;
            }
        }
      else if ((f->inCapacity + 1) < sizeof(f->inBuf))
        {
#ifdef CONFIG_EOL_IS_BOTH_CRLF
          /* Ignore carriage returns that may accompany a CRLF sequence. */

          if (ch != '\r')
#endif
            {
              /* Is this a new line character */

#ifdef CONFIG_EOL_IS_CR
              if (ch != '\r')
#elif defined(CONFIG_EOL_IS_LF)
              if (ch != '\n')
#elif defined(CONFIG_EOL_IS_EITHER_CRLF)
              if (ch != '\n' && ch != '\r' )
#endif
                {
                  /* No.. escape control characters other than newline and
                   * carriage return
                   */

                  if (ch >= '\0' && ch < ' ')
                    {
                      FS_putChar(chn, '^');
                      FS_putChar(chn, ch ? (ch + 'a' - 1) : '@');
                    }

                  /* Output normal, printable characters */

                  else
                    {
                      FS_putChar(chn, ch);
                    }
                }

              /* It is a newline */

              else
                {
                  /* Echo the newline (or not).  We always use newline
                   * termination when talking to the host.
                   */

                  if (onl)
                    {
                      FS_putChar(chn, '\n');
                    }

#if defined(CONFIG_EOL_IS_CR) || defined(CONFIG_EOL_IS_EITHER_CRLF)
                  /* If the host is talking to us with CR line terminations,
                   * switch to use LF internally.
                   */

                  ch = '\n';
#endif
                }

              f->inBuf[f->inCapacity++] = ch;
            }
        }
    }
  while (ch != '\n');

  return 0;
}

static int cls(int chn)
{
#ifdef CONFIG_INTERPRETER_BAS_VT100
  vt100_clrscreen(chn);
  vt100_cursorhome(chn);
  return 0;
#else
  FS_errmsg = _("Clear screen operation not implemented");
  return -1;
#endif
}

static int locate(int chn, int line, int column)
{
#ifdef CONFIG_INTERPRETER_BAS_VT100
  vt100_setcursor(chn, line, column);
  return 0;
#else
  FS_errmsg = _("Set cursor position operation not implement");
  return -1;
#endif
}

static int colour(int chn, int foreground, int background)
{
#ifdef CONFIG_INTERPRETER_BAS_VT100
  if (foreground >= 0)
    {
      vt100_foreground_color(chn, foreground);
    }

  if (background >= 0)
    {
      vt100_background_color(chn, background);
    }

  return 0;
#else
  FS_errmsg = _("Set color operation no implemented");
  return -1;
#endif
}

static int resetcolour(int chn)
{
#ifdef CONFIG_INTERPRETER_BAS_VT100
  vt100_foreground_color(chn, VT100_DEFAULT);
  vt100_background_color(chn, VT100_DEFAULT);
#endif
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int FS_opendev(int chn, int infd, int outfd)
{
  if (size(chn) == -1)
    {
      return -1;
    }

  if (g_file[chn] != (struct FileStream *)0)
    {
      FS_errmsg = _("channel already open");
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->dev = 1;
#ifdef CONFIG_SERIAL_TERMIOS
  g_file[chn]->tty = (infd == 0 ? isatty(infd) && isatty(outfd) : 0);
#else
  g_file[chn]->tty = 1;
#endif
  g_file[chn]->recLength = 1;
  g_file[chn]->infd = infd;
  g_file[chn]->inSize = 0;
  g_file[chn]->inCapacity = 0;
  g_file[chn]->outfd = outfd;
  g_file[chn]->outPos = 0;
  g_file[chn]->outLineWidth = LINEWIDTH;
  g_file[chn]->outColWidth = COLWIDTH;
  g_file[chn]->outCapacity = sizeof(g_file[chn]->outBuf);
  g_file[chn]->outSize = 0;
  g_file[chn]->outforeground = -1;
  g_file[chn]->outbackground = -1;
  g_file[chn]->randomfd = -1;
  g_file[chn]->binaryfd = -1;
  FS_errmsg = (const char *)0;
  ++g_used;
  return 0;
}

int FS_openin(const char *name)
{
  int chn, fd;

  if ((fd = open(name, O_RDONLY)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  for (chn = 0; chn < g_capacity; ++chn)
    {
      if (g_file[chn] == (struct FileStream *)0)
        {
          break;
        }
    }

  if (size(chn) == -1)
    {
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->recLength = 1;
  g_file[chn]->dev = 0;
  g_file[chn]->tty = 0;
  g_file[chn]->infd = fd;
  g_file[chn]->inSize = 0;
  g_file[chn]->inCapacity = 0;
  g_file[chn]->outfd = -1;
  g_file[chn]->randomfd = -1;
  g_file[chn]->binaryfd = -1;
  FS_errmsg = (const char *)0;
  ++g_used;
  return chn;
}

int FS_openinChn(int chn, const char *name, int mode)
{
  int fd;
  mode_t fl;

  if (size(chn) == -1)
    {
      return -1;
    }

  if (g_file[chn] != (struct FileStream *)0)
    {
      FS_errmsg = _("channel already open");
      return -1;
    }

  fl = g_open_mode[mode];

  /* Serial devices on Linux should be opened non-blocking, otherwise the
   * open() may block already.  Named pipes can not be opened non-blocking in
   * write-only mode, so first try non-blocking, then blocking. */

  if ((fd = open(name, fl | O_NONBLOCK)) == -1)
    {
      if (errno != ENXIO || (fd = open(name, fl)) == -1)
        {
          FS_errmsg = strerror(errno);
          return -1;
        }
    }
  else if (fcntl(fd, F_SETFL, (long)fl) == -1)
    {
      FS_errmsg = strerror(errno);
      close(fd);
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->recLength = 1;
  g_file[chn]->dev = 0;
  g_file[chn]->tty = 0;
  g_file[chn]->infd = fd;
  g_file[chn]->inSize = 0;
  g_file[chn]->inCapacity = 0;
  g_file[chn]->outfd = -1;
  g_file[chn]->randomfd = -1;
  g_file[chn]->binaryfd = -1;
  FS_errmsg = (const char *)0;
  ++g_used;
  return chn;
}

int FS_openout(const char *name)
{
  int chn, fd;

  if ((fd = open(name, O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  for (chn = 0; chn < g_capacity; ++chn)
    {
      if (g_file[chn] == (struct FileStream *)0)
        {
          break;
        }
    }

  if (size(chn) == -1)
    {
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->recLength = 1;
  g_file[chn]->dev = 0;
  g_file[chn]->tty = 0;
  g_file[chn]->infd = -1;
  g_file[chn]->outfd = fd;
  g_file[chn]->outPos = 0;
  g_file[chn]->outLineWidth = LINEWIDTH;
  g_file[chn]->outColWidth = COLWIDTH;
  g_file[chn]->outSize = 0;
  g_file[chn]->outCapacity = sizeof(g_file[chn]->outBuf);
  g_file[chn]->randomfd = -1;
  g_file[chn]->binaryfd = -1;
  FS_errmsg = (const char *)0;
  ++g_used;
  return chn;
}

int FS_openoutChn(int chn, const char *name, int mode, int append)
{
  int fd;
  mode_t fl;

  if (size(chn) == -1)
    {
      return -1;
    }

  if (g_file[chn] != (struct FileStream *)0)
    {
      FS_errmsg = _("channel already open");
      return -1;
    }

  fl = g_open_mode[mode] | (append ? O_APPEND : 0);

  /* Serial devices on Linux should be opened non-blocking, otherwise the */
  /* open() may block already.  Named pipes can not be opened non-blocking */
  /* in write-only mode, so first try non-blocking, then blocking.  */

  fd = open(name, fl | O_CREAT | (append ? 0 : O_TRUNC) | O_NONBLOCK, 0666);
  if (fd == -1)
    {
      if (errno != ENXIO ||
          (fd = open(name, fl | O_CREAT | (append ? 0 : O_TRUNC), 0666)) == -1)
        {
          FS_errmsg = strerror(errno);
          return -1;
        }
    }
  else if (fcntl(fd, F_SETFL, (long)fl) == -1)
    {
      FS_errmsg = strerror(errno);
      close(fd);
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->recLength = 1;
  g_file[chn]->dev = 0;
  g_file[chn]->tty = 0;
  g_file[chn]->infd = -1;
  g_file[chn]->outfd = fd;
  g_file[chn]->outPos = 0;
  g_file[chn]->outLineWidth = LINEWIDTH;
  g_file[chn]->outColWidth = COLWIDTH;
  g_file[chn]->outSize = 0;
  g_file[chn]->outCapacity = sizeof(g_file[chn]->outBuf);
  g_file[chn]->randomfd = -1;
  g_file[chn]->binaryfd = -1;
  FS_errmsg = (const char *)0;
  ++g_used;
  return chn;
}

int FS_openrandomChn(int chn, const char *name, int mode, int recLength)
{
  int fd;

  assert(chn >= 0);
  assert(name != (const char *)0);
  assert(recLength > 0);
  if (size(chn) == -1)
    {
      return -1;
    }

  if (g_file[chn] != (struct FileStream *)0)
    {
      FS_errmsg = _("channel already open");
      return -1;
    }

  if ((fd = open(name, g_open_mode[mode] | O_CREAT, 0666)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->recLength = recLength;
  g_file[chn]->dev = 0;
  g_file[chn]->tty = 0;
  g_file[chn]->infd = -1;
  g_file[chn]->outfd = -1;
  g_file[chn]->randomfd = fd;
  g_file[chn]->recBuf = malloc(recLength);
  memset(g_file[chn]->recBuf, 0, recLength);
  StringField_new(&g_file[chn]->field);
  g_file[chn]->binaryfd = -1;
  FS_errmsg = (const char *)0;
  ++g_used;
  return chn;
}

int FS_openbinaryChn(int chn, const char *name, int mode)
{
  int fd;

  assert(chn >= 0);
  assert(name != (const char *)0);
  if (size(chn) == -1)
    {
      return -1;
    }

  if (g_file[chn] != (struct FileStream *)0)
    {
      FS_errmsg = _("channel already open");
      return -1;
    }

  if ((fd = open(name, g_open_mode[mode] | O_CREAT, 0666)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  g_file[chn] = malloc(sizeof(struct FileStream));
  g_file[chn]->recLength = 1;
  g_file[chn]->dev = 0;
  g_file[chn]->tty = 0;
  g_file[chn]->infd = -1;
  g_file[chn]->outfd = -1;
  g_file[chn]->randomfd = -1;
  g_file[chn]->binaryfd = fd;
  FS_errmsg = (const char *)0;
  ++g_used;
  return chn;
}

int FS_freechn(void)
{
  int i;

  for (i = 0; i < g_capacity && g_file[i]; ++i);
  if (size(i) == -1)
    {
      return -1;
    }

  return i;
}

int FS_flush(int dev)
{
  ssize_t written;
  size_t offset;

  if (g_file[dev] == (struct FileStream *)0)
    {
      FS_errmsg = _("channel not open");
      return -1;
    }

  offset = 0;
  while (offset < g_file[dev]->outSize)
    {
      written =
        write(g_file[dev]->outfd, g_file[dev]->outBuf + offset,
              g_file[dev]->outSize - offset);
      if (written == -1)
        {
          FS_errmsg = strerror(errno);
          return -1;
        }
      else
        {
          offset += written;
        }
    }

  g_file[dev]->outSize = 0;
  FS_errmsg = (const char *)0;
  return 0;
}

int FS_close(int dev)
{
  if (g_file[dev] == (struct FileStream *)0)
    {
      FS_errmsg = _("channel not open");
      return -1;
    }

  if (g_file[dev]->outfd >= 0)
    {
      if (g_file[dev]->tty &&
          (g_file[dev]->outforeground != -1 || g_file[dev]->outbackground != -1))
        {
          resetcolour(dev);
        }

      FS_flush(dev);
      close(g_file[dev]->outfd);
    }

  if (g_file[dev]->randomfd >= 0)
    {
      StringField_destroy(&g_file[dev]->field);
      free(g_file[dev]->recBuf);
      close(g_file[dev]->randomfd);
    }

  if (g_file[dev]->binaryfd >= 0)
    {
      close(g_file[dev]->binaryfd);
    }

  if (g_file[dev]->infd >= 0)
    {
      close(g_file[dev]->infd);
    }

  free(g_file[dev]);
  g_file[dev] = (struct FileStream *)0;
  FS_errmsg = (const char *)0;
  if (--g_used == 0)
    {
      free(g_file);
      g_file     = (struct FileStream **)0;
      g_capacity = 0;
    }

  return 0;
}

#ifdef CONFIG_SERIAL_TERMIOS
int FS_istty(int chn)
{
  return (g_file[chn] && g_file[chn]->tty);
}
#endif

int FS_lock(int chn, off_t offset, off_t length, int mode, int w)
{
  int fd;
  struct flock recordLock;

  if (g_file[chn] == (struct FileStream *)0)
    {
      FS_errmsg = _("channel not open");
      return -1;
    }

  if ((fd = g_file[chn]->infd) == -1)
    {
      if ((fd = g_file[chn]->outfd) == -1)
        {
          if ((fd = g_file[chn]->randomfd) == -1)
            {
              if ((fd = g_file[chn]->binaryfd) == -1)
                assert(0);
            }
        }
    }

  recordLock.l_whence = SEEK_SET;
  recordLock.l_start = offset;
  recordLock.l_len = length;
  switch (mode)
    {
    case FS_LOCK_SHARED:
      recordLock.l_type = F_RDLCK;
      break;

    case FS_LOCK_EXCLUSIVE:
      recordLock.l_type = F_WRLCK;
      break;

    case FS_LOCK_NONE:
      recordLock.l_type = F_UNLCK;
      break;

    default:
      assert(0);
    }

  if (fcntl(fd, w ? F_SETLKW : F_SETLK, &recordLock) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return 0;
}

int FS_truncate(int chn)
{
#ifdef CONFIG_INTERPRETER_BAS_HAVE_FTRUNCATE
  int fd;
  off_t o;

  if (g_file[chn] == (struct FileStream *)0)
    {
      FS_errmsg = _("channel not open");
      return -1;
    }

  if ((fd = g_file[chn]->infd) == -1)
    {
      if ((fd = g_file[chn]->outfd) == -1)
        {
          if ((fd = g_file[chn]->randomfd) == -1)
            {
              if ((fd = g_file[chn]->binaryfd) == -1)
                {
                  assert(0);
                }
            }
        }
    }

  if ((o = lseek(fd, 0, SEEK_CUR)) == (off_t) - 1 || ftruncate(fd, o + 1) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return 0;
#else
  FS_errmsg = strerror(ENOSYS);
  return -1;
#endif
}

void FS_shellmode(int dev)
{
}

void FS_fsmode(int chn)
{
}

void FS_xonxoff(int chn, int on)
{
  /* Not implemented */
}

int FS_put(int chn)
{
  ssize_t offset, written;

  if (opened(chn, 2) == -1)
    {
      return -1;
    }

  offset = 0;
  while (offset < g_file[chn]->recLength)
    {
      written =
        write(g_file[chn]->randomfd, g_file[chn]->recBuf + offset,
              g_file[chn]->recLength - offset);
      if (written == -1)
        {
          FS_errmsg = strerror(errno);
          return -1;
        }
      else
        {
          offset += written;
        }
    }

  FS_errmsg = (const char *)0;
  return 0;
}

int FS_putChar(int dev, char ch)
{
  struct FileStream *f;

  if (opened(dev, 0) == -1)
    {
      return -1;
    }

  f = g_file[dev];
  if (ch == '\n')
    {
      f->outPos = 0;
    }

  if (ch == '\b' && f->outPos)
    {
      --f->outPos;
    }

  if (f->outSize + 2 >= f->outCapacity && FS_flush(dev) == -1)
    {
      return -1;
    }

  if (f->outLineWidth && f->outPos == f->outLineWidth)
    {
      f->outBuf[f->outSize++] = '\n';
      f->outPos = 0;
    }

  f->outBuf[f->outSize++] = ch;

  if (ch != '\n' && ch != '\b')
    {
      ++f->outPos;
    }

  FS_errmsg = (const char *)0;
  return 0;
}

int FS_putChars(int dev, const char *chars)
{
  while (*chars)
    {
      if (FS_putChar(dev, *chars++) == -1)
        {
          return -1;
        }
    }

  return 0;
}

int FS_putString(int dev, const struct String *s)
{
  size_t len = s->length;
  const char *c = s->character;

  while (len)
    {
      if (FS_putChar(dev, *c++) == -1)
        {
          return -1;
        }
      else
        {
          --len;
        }
    }

  return 0;
}

int FS_putItem(int dev, const struct String *s)
{
  struct FileStream *f;

  if (opened(dev, 0) == -1)
    {
      return -1;
    }

  f = g_file[dev];
  if (f->outPos && f->outPos + s->length > f->outLineWidth)
    {
      FS_nextline(dev);
    }

  return FS_putString(dev, s);
}

int FS_putbinaryString(int chn, const struct String *s)
{
  if (opened(chn, 3) == -1)
    {
      return -1;
    }

  if (s->length &&
      write(g_file[chn]->binaryfd, s->character, s->length) != s->length)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return 0;
}

int FS_putbinaryInteger(int chn, long int x)
{
  char s[sizeof(long int)];
  int i;

  if (opened(chn, 3) == -1)
    {
      return -1;
    }

  for (i = 0; i < sizeof(x); ++i, x >>= 8)
    {
      s[i] = (x & 0xff);
    }

  if (write(g_file[chn]->binaryfd, s, sizeof(s)) != sizeof(s))
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return 0;
}

int FS_putbinaryReal(int chn, double x)
{
  if (opened(chn, 3) == -1)
    {
      return -1;
    }

  if (write(g_file[chn]->binaryfd, &x, sizeof(x)) != sizeof(x))
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return 0;
}

int FS_getbinaryString(int chn, struct String *s)
{
  ssize_t len;

  if (opened(chn, 3) == -1)
    {
      return -1;
    }

  if (s->length &&
      (len = read(g_file[chn]->binaryfd, s->character, s->length)) != s->length)
    {
      if (len == -1)
        {
          FS_errmsg = strerror(errno);
        }
      else
        {
          FS_errmsg = _("End of g_file");
        }

      return -1;
    }

  return 0;
}

int FS_getbinaryInteger(int chn, long int *x)
{
  char s[sizeof(long int)];
  int i;
  ssize_t len;

  if (opened(chn, 3) == -1)
    {
      return -1;
    }

  if ((len = read(g_file[chn]->binaryfd, s, sizeof(s))) != sizeof(s))
    {
      if (len == -1)
        {
          FS_errmsg = strerror(errno);
        }
      else
        {
          FS_errmsg = _("End of file");
        }

      return -1;
    }

  *x = (s[sizeof(x) - 1] < 0) ? -1 : 0;
  for (i = sizeof(s) - 1; i >= 0; --i)
    {
      *x = (*x << 8) | (s[i] & 0xff);
    }

  return 0;
}

int FS_getbinaryReal(int chn, double *x)
{
  ssize_t len;

  if (opened(chn, 3) == -1)
    {
      return -1;
    }

  if ((len = read(g_file[chn]->binaryfd, x, sizeof(*x))) != sizeof(*x))
    {
      if (len == -1)
        {
          FS_errmsg = strerror(errno);
        }
      else
        {
          FS_errmsg = _("End of file");
        }

      return -1;
    }

  return 0;
}

int FS_nextcol(int dev)
{
  struct FileStream *f;

  if (opened(dev, 0) == -1)
    {
      return -1;
    }

  f = g_file[dev];
  if (f->outPos % f->outColWidth
      && f->outLineWidth
      && ((f->outPos / f->outColWidth + 2) * f->outColWidth) > f->outLineWidth)
    {
      return FS_putChar(dev, '\n');
    }

  if (!(f->outPos % f->outColWidth) && FS_putChar(dev, ' ') == -1)
    {
      return -1;
    }

  while (f->outPos % f->outColWidth)
    {
      if (FS_putChar(dev, ' ') == -1)
        {
          return -1;
        }
    }

  return 0;
}

int FS_nextline(int dev)
{
  struct FileStream *f;

  if (opened(dev, 0) == -1)
    {
      return -1;
    }

  f = g_file[dev];
  if (f->outPos && FS_putChar(dev, '\n') == -1)
    {
      return -1;
    }

  return 0;
}

int FS_tab(int dev, int position)
{
  struct FileStream *f = g_file[dev];

  if (f->outLineWidth && position >= f->outLineWidth)
    {
      position = f->outLineWidth - 1;
    }

  while (f->outPos < (position - 1))
    {
      if (FS_putChar(dev, ' ') == -1)
        {
          return -1;
        }
    }

  return 0;
}

int FS_width(int dev, int width)
{
  if (opened(dev, 0) == -1)
    {
      return -1;
    }

  if (width < 0)
    {
      FS_errmsg = _("negative width");
      return -1;
    }

  g_file[dev]->outLineWidth = width;
  return 0;
}

int FS_zone(int dev, int zone)
{
  if (opened(dev, 0) == -1)
    {
      return -1;
    }

  if (zone <= 0)
    {
      FS_errmsg = _("non-positive zone width");
      return -1;
    }

  g_file[dev]->outColWidth = zone;
  return 0;
}

int FS_cls(int chn)
{
  struct FileStream *f;

  if (opened(chn, 0) == -1)
    {
      return -1;
    }

  f = g_file[chn];
  if (!f->tty)
    {
      FS_errmsg = _("not a terminal");
      return -1;
    }

  if (cls(chn) == -1)
    {
      return -1;
    }

  if (FS_flush(chn) == -1)
    {
      return -1;
    }

  f->outPos = 0;
  return 0;
}

int FS_locate(int chn, int line, int column)
{
  struct FileStream *f;

  if (opened(chn, 0) == -1)
    {
      return -1;
    }

  f = g_file[chn];
  if (!f->tty)
    {
      FS_errmsg = _("not a terminal");
      return -1;
    }

  if (locate(chn, line, column) == -1)
    {
      return -1;
    }

  if (FS_flush(chn) == -1)
    {
      return -1;
    }

  f->outPos = column - 1;
  return 0;
}

int FS_colour(int chn, int foreground, int background)
{
  struct FileStream *f;

  if (opened(chn, 0) == -1)
    {
      return -1;
    }

  f = g_file[chn];
  if (!f->tty)
    {
      FS_errmsg = _("not a terminal");
      return -1;
    }

  if (colour(chn, foreground, background) == -1)
    {
      return -1;
    }

  f->outforeground = foreground;
  f->outbackground = background;
  return 0;
}

int FS_getChar(int dev)
{
  struct FileStream *f;

  if (opened(dev, 1) == -1)
    {
      return -1;
    }

  f = g_file[dev];
  if (f->inSize == f->inCapacity && refill(dev) == -1)
    {
      return -1;
    }

  FS_errmsg = (const char *)0;
  if (f->inSize + 1 == f->inCapacity)
    {
      char ch = f->inBuf[f->inSize];

      f->inSize = f->inCapacity = 0;
      return ch;
    }
  else
    {
      return f->inBuf[f->inSize++];
    }
}

int FS_get(int chn)
{
  ssize_t offset, rd;

  if (opened(chn, 2) == -1)
    {
      return -1;
    }

  offset = 0;
  while (offset < g_file[chn]->recLength)
    {
      rd =
        read(g_file[chn]->randomfd, g_file[chn]->recBuf + offset,
             g_file[chn]->recLength - offset);
      if (rd == -1)
        {
          FS_errmsg = strerror(errno);
          return -1;
        }
      else
        {
          offset += rd;
        }
    }

  FS_errmsg = (const char *)0;
  return 0;
}

int FS_inkeyChar(int dev, int ms)
{
  struct FileStream *f;
  char c;
  ssize_t len;
#ifdef CONFIG_INTERPRETER_BAS_USE_SELECT
  fd_set just_infd;
  struct timeval timeout;
#endif

  if (opened(dev, 1) == -1)
    {
      return -1;
    }

  f = g_file[dev];
  if (f->inSize < f->inCapacity)
    {
      return f->inBuf[f->inSize++];
    }

#ifdef CONFIG_INTERPRETER_BAS_USE_SELECT
  FD_ZERO(&just_infd);
  FD_SET(f->infd, &just_infd);
  timeout.tv_sec = ms / 1000;
  timeout.tv_usec = (ms % 1000) * 1000;
  switch (select(f->infd + 1, &just_infd, (fd_set *) 0, (fd_set *) 0, &timeout))
    {
    case 1:
      {
        FS_errmsg = (const char *)0;
        len = read(f->infd, &c, 1);
        return (len == 1 ? c : -1);
      }

    case 0:
      {
        FS_errmsg = (const char *)0;
        return -1;
      }

    case -1:
      {
        FS_errmsg = strerror(errno);
        return -1;
      }

    default:
      assert(0);
    }

  return 0;

#else
  FS_errmsg = (const char *)0;
  len = read(f->infd, &c, 1);

  if (len == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return (len == 1 ? c : -1);
#endif
}

void FS_sleep(double s)
{
  struct timespec p;

  p.tv_sec = floor(s);
  p.tv_nsec = 1000000000 * (s - floor(s));

  nanosleep(&p, (struct timespec *)0);
}

int FS_eof(int chn)
{
  struct FileStream *f;

  if (opened(chn, 1) == -1)
    {
      return -1;
    }

  f = g_file[chn];
  if (f->inSize == f->inCapacity && refill(chn) == -1)
    {
      return 1;
    }

  return 0;
}

long int FS_loc(int chn)
{
  int fd;
  off_t cur, offset = 0;

  if (opened(chn, -1) == -1)
    {
      return -1;
    }

  if (g_file[chn]->infd != -1)
    {
      fd = g_file[chn]->infd;
      offset = -g_file[chn]->inCapacity + g_file[chn]->inSize;
    }
  else if (g_file[chn]->outfd != -1)
    {
      fd = g_file[chn]->outfd;
      offset = g_file[chn]->outSize;
    }
  else if (g_file[chn]->randomfd != -1)
    {
      fd = g_file[chn]->randomfd;
    }
  else
    {
      fd = g_file[chn]->binaryfd;
    }

  assert(fd != -1);
  if ((cur = lseek(fd, 0, SEEK_CUR)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return (cur + offset) / g_file[chn]->recLength;
}

long int FS_lof(int chn)
{
  off_t curpos;
  off_t endpos;
  int fd;

  if (opened(chn, -1) == -1)
    {
      return -1;
    }

  if (g_file[chn]->infd != -1)
    {
      fd = g_file[chn]->infd;
    }
  else if (g_file[chn]->outfd != -1)
    {
      fd = g_file[chn]->outfd;
    }
  else if (g_file[chn]->randomfd != -1)
    {
      fd = g_file[chn]->randomfd;
    }
  else
    {
      fd = g_file[chn]->binaryfd;
    }

  assert(fd != -1);

  /* Get the size of the file */
  /* Save the current file position */

  curpos = lseek(fd, 0, SEEK_CUR);
  if (curpos == (off_t)-1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  /* Get the position at the end of the file */

  endpos = lseek(fd, 0, SEEK_END);
  if (endpos == (off_t)-1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  /* Restore the file position */

  curpos = lseek(fd, curpos, SEEK_SET);
  if (curpos == (off_t)-1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return (long int)(endpos / g_file[chn]->recLength);
}

long int FS_recLength(int chn)
{
  if (opened(chn, 2) == -1)
    {
      return -1;
    }

  return g_file[chn]->recLength;
}

void FS_field(int chn, struct String *s, long int position, long int length)
{
  assert(g_file[chn]);
  String_joinField(s, &g_file[chn]->field, g_file[chn]->recBuf + position, length);
}

int FS_seek(int chn, long int record)
{
  if (opened(chn, 2) != -1)
    {
      if (lseek
          (g_file[chn]->randomfd, (off_t) record * g_file[chn]->recLength,
           SEEK_SET) != -1)
        {
          return 0;
        }

      FS_errmsg = strerror(errno);
    }
  else if (opened(chn, 4) != -1)
    {
      if (lseek(g_file[chn]->binaryfd, (off_t) record, SEEK_SET) != -1)
        {
          return 0;
        }

      FS_errmsg = strerror(errno);
    }

  return -1;
}

int FS_appendToString(int chn, struct String *s, int onl)
{
  size_t new;
  char *n;
  struct FileStream *f = g_file[chn];
  int c;

  if (f->tty && f->inSize == f->inCapacity)
    {
      if (edit(chn, onl) == -1)
        {
          return (FS_errmsg ? -1 : 0);
        }
    }

  do
    {
      n = f->inBuf + f->inSize;
      while (1)
        {
          if (n == f->inBuf + f->inCapacity)
            {
              break;
            }

          c = *n++;
          if (c == '\n')
            {
              break;
            }
        }

      new = n - (f->inBuf + f->inSize);
      if (new)
        {
          size_t offset = s->length;

          if (String_size(s, offset + new) == -1)
            {
              FS_errmsg = strerror(errno);
              return -1;
            }

          memcpy(s->character + offset, f->inBuf + f->inSize, new);
          f->inSize += new;
          if (*(n - 1) == '\n')
            {
              if (f->inSize == f->inCapacity)
                {
                  f->inSize = f->inCapacity = 0;
                }

              return 0;
            }
        }

      if ((c = FS_getChar(chn)) >= 0)
        {
          String_appendChar(s, c);
        }

      if (c == '\n')
        {
          if (s->length >= 2 && s->character[s->length - 2] == '\r')
            {
              s->character[s->length - 2] = '\n';
              --s->length;
            }

          return 0;
        }
    }
  while (c != -1);

  return (FS_errmsg ? -1 : 0);
}

void FS_closefiles(void)
{
  int i;

  /* Example each entry in the g_files[] arrary */

  for (i = 0; i < g_capacity; ++i)
    {
      /* Has this entry been allocated?  Is it a file?  Or a device? */

      if (g_file[i] && !g_file[i]->dev)
        {
          /* It is an open file, close it */

          FS_close(i);
        }
    }
}

int FS_charpos(int chn)
{
  if (g_file[chn] == (struct FileStream *)0)
    {
      FS_errmsg = _("channel not open");
      return -1;
    }

  return (g_file[chn]->outPos);
}

int FS_copy(const char *from, const char *to)
{
  int infd, outfd;
  char buf[4096];
  ssize_t inlen, outlen = -1;

  if ((infd = open(from, O_RDONLY)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  if ((outfd = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  while ((inlen = read(infd, &buf, sizeof(buf))) > 0)
    {
      ssize_t off = 0;

      while (inlen && (outlen = write(outfd, &buf + off, inlen)) > 0)
        {
          off += outlen;
          inlen -= outlen;
        }

      if (outlen == -1)
        {
          FS_errmsg = strerror(errno);
          close(infd);
          close(outfd);
          return -1;
        }
    }

  if (inlen == -1)
    {
      FS_errmsg = strerror(errno);
      close(infd);
      close(outfd);
      return -1;
    }

  if (close(infd) == -1)
    {
      FS_errmsg = strerror(errno);
      close(outfd);
      return -1;
    }

  if (close(outfd) == -1)
    {
      FS_errmsg = strerror(errno);
      return -1;
    }

  return 0;
}

int FS_portInput(int address)
{
  FS_errmsg = _("Direct port access not available");
  return -1;
}

int FS_memInput(int address)
{
  FS_errmsg = _("Direct memory access not available");
  return -1;
}

int FS_portOutput(int address, int value)
{
  FS_errmsg = _("Direct port access not available");
  return -1;
}

int FS_memOutput(int address, int value)
{
  FS_errmsg = _("Direct memory access not available");
  return -1;
}
