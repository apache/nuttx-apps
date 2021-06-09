/****************************************************************************
 * apps/interpreters/bas/bas_fs.h
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
 ****************************************************************************/

#ifndef __APPS_EXAMPLES_BAS_BAS_FS_H
#define __APPS_EXAMPLES_BAS_BAS_FS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include "bas_str.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FS_COLOUR_BLACK         0
#define FS_COLOUR_BLUE          1
#define FS_COLOUR_GREEN         2
#define FS_COLOUR_CYAN          3
#define FS_COLOUR_RED           4
#define FS_COLOUR_MAGENTA       5
#define FS_COLOUR_BROWN         6
#define FS_COLOUR_WHITE         7
#define FS_COLOUR_GREY          8
#define FS_COLOUR_LIGHTBLUE     9
#define FS_COLOUR_LIGHTGREEN    10
#define FS_COLOUR_LIGHTCYAN     11
#define FS_COLOUR_LIGHTRED      12
#define FS_COLOUR_LIGHTMAGENTA  13
#define FS_COLOUR_YELLOW        14
#define FS_COLOUR_BRIGHTWHITE   15

#define FS_ACCESS_NONE          0
#define FS_ACCESS_READ          1
#define FS_ACCESS_WRITE         2
#define FS_ACCESS_READWRITE     3

#define FS_LOCK_NONE            0
#define FS_LOCK_SHARED          1
#define FS_LOCK_EXCLUSIVE       2

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct FileStream
{
  int dev,tty;
  int recLength;

  int infd;
  char inBuf[1024];
  size_t inSize,inCapacity;

  int outfd;
  int outPos;
  int outLineWidth;
  int outColWidth;
  char outBuf[1024];
  size_t outSize,outCapacity;
  int outforeground,outbackground;

  int randomfd;
  int recPos;
  char *recBuf;
  struct StringField field;

  int binaryfd;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char *FS_errmsg;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int FS_opendev(int dev, int infd, int outfd);
int FS_openin(const char *name);
int FS_openinChn(int chn, const char *name, int mode);
int FS_openout(const char *name);
int FS_openoutChn(int chn, const char *name, int mode, int append);
int FS_openrandomChn(int chn, const char *name, int mode, int recLength);
int FS_openbinaryChn(int chn, const char *name, int mode);
int FS_freechn(void);
int FS_flush(int dev);
int FS_close(int dev);

#ifdef CONFIG_SERIAL_TERMIOS
int FS_istty(int chn);
#else
#  define FS_istty(chn) (1)
#endif

int FS_lock(int chn, off_t offset, off_t length, int mode, int w);
int FS_truncate(int chn);
void FS_shellmode(int chn);
void FS_fsmode(int chn);
void FS_xonxoff(int chn, int on);
int FS_put(int chn);
int FS_putChar(int dev, char ch);
int FS_putChars(int dev, const char *chars);
int FS_putString(int dev, const struct String *s);
int FS_putItem(int dev, const struct String *s);
int FS_putbinaryString(int chn, const struct String *s);
int FS_putbinaryInteger(int chn, long int x);
int FS_putbinaryReal(int chn, double x);
int FS_getbinaryString(int chn, struct String *s);
int FS_getbinaryInteger(int chn, long int *x);
int FS_getbinaryReal(int chn, double *x);
int FS_nextcol(int dev);
int FS_nextline(int dev);
int FS_tab(int dev, int position);
int FS_cls(int chn);
int FS_locate(int chn, int line, int column);
int FS_colour(int chn, int foreground, int background);
int FS_get(int chn);
int FS_getChar(int dev);
int FS_eof(int chn);
long int FS_loc(int chn);
long int FS_lof(int chn);
int FS_width(int dev, int width);
int FS_zone(int dev, int zone);
long int FS_recLength(int chn);
void FS_field(int chn, struct String *s, long int position, long int length);
int FS_appendToString(int dev, struct String *s, int nl);
int FS_inkeyChar(int dev, int ms);
void FS_sleep(double s);
int FS_seek(int chn, long int record);
void FS_closefiles(void);
int FS_charpos(int chn);
int FS_copy(const char *from, const char *to);
int FS_portInput(int address);
int FS_memInput(int address);
int FS_portOutput(int address, int value);
int FS_memOutput(int address, int value);

#endif /* __APPS_EXAMPLES_BAS_BAS_FS_H */
