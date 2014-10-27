#ifndef FILE_H
#define FILE_H

#include "str.h"

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

extern const char *FS_errmsg;
extern volatile int FS_intr;

extern int FS_opendev(int dev, int infd, int outfd);
extern int FS_openin(const char *name);
extern int FS_openinChn(int chn, const char *name, int mode);
extern int FS_openout(const char *name);
extern int FS_openoutChn(int chn, const char *name, int mode, int append);
extern int FS_openrandomChn(int chn, const char *name, int mode, int recLength);
extern int FS_openbinaryChn(int chn, const char *name, int mode);
extern int FS_freechn(void);
extern int FS_flush(int dev);
extern int FS_close(int dev);
extern int FS_istty(int chn);
extern int FS_lock(int chn, off_t offset, off_t length, int mode, int w);
extern int FS_truncate(int chn);
extern void FS_shellmode(int chn);
extern void FS_fsmode(int chn);
extern void FS_xonxoff(int chn, int on);
extern int FS_put(int chn);
extern int FS_putChar(int dev, char ch);
extern int FS_putChars(int dev, const char *chars);
extern int FS_putString(int dev, const struct String *s);
extern int FS_putItem(int dev, const struct String *s);
extern int FS_putbinaryString(int chn, const struct String *s);
extern int FS_putbinaryInteger(int chn, long int x);
extern int FS_putbinaryReal(int chn, double x);
extern int FS_getbinaryString(int chn, struct String *s);
extern int FS_getbinaryInteger(int chn, long int *x);
extern int FS_getbinaryReal(int chn, double *x);
extern int FS_nextcol(int dev);
extern int FS_nextline(int dev);
extern int FS_tab(int dev, int position);
extern int FS_cls(int chn);
extern int FS_locate(int chn, int line, int column);
extern int FS_colour(int chn, int foreground, int background);
extern int FS_get(int chn);
extern int FS_getChar(int dev);
extern int FS_eof(int chn);
extern long int FS_loc(int chn);
extern long int FS_lof(int chn);
extern int FS_width(int dev, int width);
extern int FS_zone(int dev, int zone);
extern long int FS_recLength(int chn);
extern void FS_field(int chn, struct String *s, long int position, long int length);
extern int FS_appendToString(int dev, struct String *s, int onl);
extern int FS_inkeyChar(int dev, int ms);
extern void FS_sleep(double s);
extern int FS_seek(int chn, long int record);
extern void FS_closefiles(void);
extern int FS_charpos(int chn);
extern int FS_copy(const char *from, const char *to);
extern int FS_portInput(int address);
extern int FS_memInput(int address);
extern int FS_portOutput(int address, int value);
extern int FS_memOutput(int address, int value);
extern void FS_allowIntr(int on);

#endif
