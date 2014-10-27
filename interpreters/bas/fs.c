/* BASIC file system interface. */
/* #includes */ /*{{{C}}}*//*{{{*/
#include "config.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_GETTEXT
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) String
#endif
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#endif
#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#include <unistd.h>

#include "fs.h"

#ifdef USE_DMALLOC
#include "dmalloc.h"
#endif
/*}}}*/
/* #defines */ /*{{{*/
#define LINEWIDTH 80
#define COLWIDTH  14
/*}}}*/

static struct FileStream **file;
static int capacity;
static int used;
static struct termios origMode,rawMode;
static const int open_mode[4]={ 0, O_RDONLY, O_WRONLY, O_RDWR };
static struct sigaction old_sigint, old_sigquit;
static int termchannel;

const char *FS_errmsg;
static char FS_errmsgbuf[80];
volatile int FS_intr;

static int size(int dev) /*{{{*/
{
  if (dev>=capacity)
  {
    int i;
    struct FileStream **n;

    if ((n=(struct FileStream**)realloc(file,(dev+1)*sizeof(struct FileStream*)))==(struct FileStream**)0)
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
    file=n;
    for (i=capacity; i<=dev; ++i) file[i]=(struct FileStream*)0;
    capacity=dev+1;
  }
  return 0;
}
/*}}}*/
static int opened(int dev, int mode) /*{{{*/
{
  int fd=-1;

  if (dev<0 || dev>=capacity || file[dev]==(struct FileStream*)0)
  {
    snprintf(FS_errmsgbuf,sizeof(FS_errmsgbuf),_("channel #%d not open"),dev);
    FS_errmsg=FS_errmsgbuf;
    return -1;
  }
  if (mode==-1) return 0;
  switch (mode)
  {
    case 0:
    {
      fd=file[dev]->outfd;
      if (fd==-1) snprintf(FS_errmsgbuf,sizeof(FS_errmsgbuf),_("channel #%d not opened for writing"),dev);
      break;
    }
    case 1:
    {
      fd=file[dev]->infd;
      if (fd==-1) snprintf(FS_errmsgbuf,sizeof(FS_errmsgbuf),_("channel #%d not opened for reading"),dev);
      break;
    }
    case 2:
    {
      fd=file[dev]->randomfd;
      if (fd==-1) snprintf(FS_errmsgbuf,sizeof(FS_errmsgbuf),_("channel #%d not opened for random access"),dev);
      break;
    }
    case 3:
    {
      fd=file[dev]->binaryfd;
      if (fd==-1) snprintf(FS_errmsgbuf,sizeof(FS_errmsgbuf),_("channel #%d not opened for binary access"),dev);
      break;
    }
    case 4:
    {
      fd=(file[dev]->randomfd!=-1?file[dev]->randomfd:file[dev]->binaryfd);
      if (fd==-1) snprintf(FS_errmsgbuf,sizeof(FS_errmsgbuf),_("channel #%d not opened for random or binary access"),dev);
      break;
    }
    default: assert(0);
  }
  if (fd==-1)
  {
    FS_errmsg=FS_errmsgbuf;
    return -1;
  }
  else return 0;
}
/*}}}*/
static int refill(int dev) /*{{{*/
{
  struct FileStream *f;
  ssize_t len;

  f=file[dev];
  f->inSize=0;
  len=read(f->infd,f->inBuf,sizeof(f->inBuf));
  if (len<=0)
  {
    f->inCapacity=0;
    FS_errmsg=(len==-1?strerror(errno):(const char*)0);
    return -1;
  }
  else
  {
    f->inCapacity=len;
    return 0;
  }
}
/*}}}*/
static int edit(int chn, int onl) /*{{{*/
{
  struct FileStream *f=file[chn];
  char *buf=f->inBuf;
  char ch;
  int r;

  for (buf=f->inBuf; buf<(f->inBuf+f->inCapacity); ++buf)
  {
    if (*buf>='\0' && *buf<' ')
    {
      FS_putChar(chn,'^');
      FS_putChar(chn,*buf?(*buf+'a'-1):'@');
    }
    else FS_putChar(chn,*buf);
  }
  do
  {
    FS_flush(chn);
    if ((r=read(f->infd,&ch,1))==-1)
    {
      f->inCapacity=0;
      FS_errmsg=strerror(errno);
      return -1;
    }
    else if (r==0 || (f->inCapacity==0 && ch==4))
    {
      FS_errmsg=(char*)0;
      return -1;
    }
    if (ch==rawMode.c_cc[VERASE])
    {
      if (f->inCapacity)
      {
        if (f->inBuf[f->inCapacity-1]>='\0' && f->inBuf[f->inCapacity-1]<' ') FS_putChars(chn,"\b\b  \b\b");
        else FS_putChars(chn,"\b \b");
        --f->inCapacity;
      }
    }
    else if ((f->inCapacity+1)<sizeof(f->inBuf))
    {
      if (ch!='\n')
      {
        if (ch>='\0' && ch<' ')
        {
          FS_putChar(chn,'^');
          FS_putChar(chn,ch?(ch+'a'-1):'@');
        }
        else FS_putChar(chn,ch);
      }
      else if (onl) FS_putChar(chn,'\n');
      f->inBuf[f->inCapacity++]=ch;
    }
  } while (ch!='\n');
  return 0;
}
/*}}}*/
static int outc(int ch) /*{{{*/
{
  struct FileStream *f;

  if (opened(termchannel,0)==-1) return -1;
  f=file[termchannel];
  if (f->outSize+1>=f->outCapacity && FS_flush(termchannel)==-1) return -1;
  f->outBuf[f->outSize++]=ch;
  FS_errmsg=(const char*)0;
  return ch;
}
/*}}}*/
#ifdef HAVE_TGETENT
static char *term,entrybuf[2048],*cap;
static char *cl,*cm,*ce,*cr,*md,*me,*AF,*AB;
static int Co,NC;

static int mytputs(const char *str, int affcnt, int (*out)(int)) /*{{{*/
{
#ifdef TPUTS_RETURNS_VOID
  tputs(str,affcnt,out);
  return 0;
#else
  return tputs(str,affcnt,out);
#endif
}
/*}}}*/
static int initTerminal(int chn) /*{{{*/
{
  static int init=0;

  if (!init)
  {
    termchannel=chn;
    if ((term=getenv("TERM"))==(char*)0)
    {
      FS_errmsg=_("environment variable TERM is not set");
      return -1;
    }
    switch (tgetent(entrybuf,term))
    {
      case -1:
      {
        FS_errmsg=_("reading terminal description failed");
        return -1;
      }
      case 0:
      {
        sprintf(FS_errmsgbuf,_("unknown terminal type %s"),term);
        FS_errmsg=FS_errmsgbuf;
        return -1;
      }
      case 1:
      {
        cl=tgetstr("cl",&cap);
        cm=tgetstr("cm",&cap);
        ce=tgetstr("ce",&cap);
        cr=tgetstr("cr",&cap);
        md=tgetstr("md",&cap);
        me=tgetstr("me",&cap);
        AF=tgetstr("AF",&cap);
        AB=tgetstr("AB",&cap);
        Co=tgetnum("Co");
        if ((NC=tgetnum("NC"))==-1) NC=0;
        return 0;
      }
    }
    init=1;
  }
  return 0;
}
/*}}}*/
static int cls(int chn) /*{{{*/
{
  if (cl==(char*)0)
  {
    sprintf(FS_errmsgbuf,_("terminal type %s can not clear the screen"),term);
    FS_errmsg=FS_errmsgbuf;
    return -1;
  }
  if (mytputs(cl,0,outc)==-1) return -1;
  return 0;
}
/*}}}*/
static int locate(int chn, int line, int column) /*{{{*/
{
  termchannel=chn;
  if (cm==(char*)0)
  {
    sprintf(FS_errmsgbuf,_("terminal type %s can not position the cursor"),term);
    FS_errmsg=FS_errmsgbuf;
    return -1;
  }
  if (mytputs(tgoto(cm,column-1,line-1),0,outc)==-1) return -1;
  return 0;
}
/*}}}*/
static int colour(int chn, int foreground, int background) /*{{{*/
{
  if (AF && AB && Co>=8)
  {
    static int map[8]={ 0,4,2,6,1,5,3,7 };

    if (foreground!=-1)
    {
      if (md && me && !(NC&32))
      {
        if (foreground>7 && file[chn]->outforeground<=7)
        {
          if (mytputs(md,0,outc)==-1) return -1;
          /* all attributes are gone now, need to set background again */
          if (background==-1) background=file[chn]->outbackground;
        }
        else if (foreground<=7 && file[chn]->outforeground>7)
        {
          if (mytputs(me,0,outc)==-1) return -1;
        }
      }
      if (mytputs(tgoto(AF,0,map[foreground&7]),0,outc)==-1) return -1;
    }
    if (background!=-1)
    {
      if (mytputs(tgoto(AB,0,map[background&7]),0,outc)==-1) return -1;
    }
  }
  return 0;
}
/*}}}*/
static int resetcolour(int chn) /*{{{*/
{
  if (me) mytputs(me,0,outc);
  if (ce) mytputs(ce,0,outc);
  return 0;
}
/*}}}*/
static void carriage_return(int chn) /*{{{*/
{
  if (cr) mytputs(cr,0,outc);
  else outc('\r');
  outc('\n');
}
/*}}}*/
#else
static int initTerminal(int chn) /*{{{*/
{
  termchannel=chn;
  return 0;
}
/*}}}*/
static int cls(int chn) /*{{{*/
{
  FS_errmsg=_("This installation does not support terminal handling");
  return -1;
}
/*}}}*/
static int locate(int chn, int line, int column) /*{{{*/
{
  FS_errmsg=_("This installation does not support terminal handling");
  return -1;
}
/*}}}*/
static int colour(int chn, int foreground, int background) /*{{{*/
{
  FS_errmsg=_("This installation does not support terminal handling");
  return -1;
}
/*}}}*/
static int resetcolour(int chn) /*{{{*/
{
  return 0;
}
/*}}}*/
static void carriage_return(int chn) /*{{{*/
{
  outc('\r');
  outc('\n');
}
/*}}}*/
#endif
static void sigintr(int sig) /*{{{*/
{
  FS_intr=1;
  FS_allowIntr(0);
}
/*}}}*/

int FS_opendev(int chn, int infd, int outfd) /*{{{*/
{
  if (size(chn)==-1) return -1;
  if (file[chn]!=(struct FileStream*)0)
  {
    FS_errmsg=_("channel already open");
    return -1;
  }
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->dev=1;
  if ((file[chn]->tty=(infd==0 ? isatty(infd) && isatty(outfd) : 0)))
  {
    if (tcgetattr(infd,&origMode)==-1)
    {
      FS_errmsg=strerror(errno);
      free(file[chn]);
      file[chn]=(struct FileStream*)0;
      return -1;
    }
    rawMode=origMode;
    rawMode.c_lflag&=~(ICANON|ECHO); /* IEXTEN would disable IUCLC, breaking UC only terminals */
    rawMode.c_cc[VMIN]=1;
    rawMode.c_cc[VTIME]=0;
    rawMode.c_oflag&=~ONLCR;
    if (tcsetattr(infd,TCSADRAIN,&rawMode)==-1)
    {
      FS_errmsg=strerror(errno);
      free(file[chn]);
      file[chn]=(struct FileStream*)0;
      return -1;
    }
    initTerminal(chn);
  }
  file[chn]->recLength=1;
  file[chn]->infd=infd;
  file[chn]->inSize=0;
  file[chn]->inCapacity=0;
  file[chn]->outfd=outfd;
  file[chn]->outPos=0;
  file[chn]->outLineWidth=LINEWIDTH;
  file[chn]->outColWidth=COLWIDTH;
  file[chn]->outCapacity=sizeof(file[chn]->outBuf);
  file[chn]->outSize=0;
  file[chn]->outforeground=-1;
  file[chn]->outbackground=-1;
  file[chn]->randomfd=-1;
  file[chn]->binaryfd=-1;
  FS_errmsg=(const char*)0;
  ++used;
  return 0;
}
/*}}}*/
int FS_openin(const char *name) /*{{{*/
{
  int chn,fd;

  if ((fd=open(name,O_RDONLY))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  for (chn=0; chn<capacity; ++chn) if (file[chn]==(struct FileStream*)0) break;
  if (size(chn)==-1) return -1;
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->recLength=1;
  file[chn]->dev=0;
  file[chn]->tty=0;
  file[chn]->infd=fd;
  file[chn]->inSize=0;
  file[chn]->inCapacity=0;
  file[chn]->outfd=-1;
  file[chn]->randomfd=-1;
  file[chn]->binaryfd=-1;
  FS_errmsg=(const char*)0;
  ++used;
  return chn;
}
/*}}}*/
int FS_openinChn(int chn, const char *name, int mode) /*{{{*/
{
  int fd;
  mode_t fl;

  if (size(chn)==-1) return -1;
  if (file[chn]!=(struct FileStream*)0)
  {
    FS_errmsg=_("channel already open");
    return -1;
  }
  fl=open_mode[mode];
  /* Serial devices on Linux should be opened non-blocking, otherwise the  */
  /* open() may block already.  Named pipes can not be opened non-blocking */
  /* in write-only mode, so first try non-blocking, then blocking.         */
  if ((fd=open(name,fl|O_NONBLOCK))==-1)
  {
    if (errno!=ENXIO || (fd=open(name,fl))==-1)
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
  }
  else if (fcntl(fd,F_SETFL,(long)fl)==-1)
  {
    FS_errmsg=strerror(errno);
    close(fd);
    return -1;
  }
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->recLength=1;
  file[chn]->dev=0;
  file[chn]->tty=0;
  file[chn]->infd=fd;
  file[chn]->inSize=0;
  file[chn]->inCapacity=0;
  file[chn]->outfd=-1;
  file[chn]->randomfd=-1;
  file[chn]->binaryfd=-1;
  FS_errmsg=(const char*)0;
  ++used;
  return chn;
}
/*}}}*/
int FS_openout(const char *name) /*{{{*/
{
  int chn,fd;

  if ((fd=open(name,O_WRONLY|O_TRUNC|O_CREAT,0666))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  for (chn=0; chn<capacity; ++chn) if (file[chn]==(struct FileStream*)0) break;
  if (size(chn)==-1) return -1;
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->recLength=1;
  file[chn]->dev=0;
  file[chn]->tty=0;
  file[chn]->infd=-1;
  file[chn]->outfd=fd;
  file[chn]->outPos=0;
  file[chn]->outLineWidth=LINEWIDTH;
  file[chn]->outColWidth=COLWIDTH;
  file[chn]->outSize=0;
  file[chn]->outCapacity=sizeof(file[chn]->outBuf);
  file[chn]->randomfd=-1;
  file[chn]->binaryfd=-1;
  FS_errmsg=(const char*)0;
  ++used;
  return chn;
}
/*}}}*/
int FS_openoutChn(int chn, const char *name, int mode, int append) /*{{{*/
{
  int fd;
  mode_t fl;

  if (size(chn)==-1) return -1;
  if (file[chn]!=(struct FileStream*)0)
  {
    FS_errmsg=_("channel already open");
    return -1;
  }
  fl=open_mode[mode]|(append?O_APPEND:0);
  /* Serial devices on Linux should be opened non-blocking, otherwise the  */
  /* open() may block already.  Named pipes can not be opened non-blocking */
  /* in write-only mode, so first try non-blocking, then blocking.         */
  if ((fd=open(name,fl|O_CREAT|(append?0:O_TRUNC)|O_NONBLOCK,0666))==-1)
  {
    if (errno!=ENXIO || (fd=open(name,fl|O_CREAT|(append?0:O_TRUNC),0666))==-1)
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
  }
  else if (fcntl(fd,F_SETFL,(long)fl)==-1)
  {
    FS_errmsg=strerror(errno);
    close(fd);
    return -1;
  }
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->recLength=1;
  file[chn]->dev=0;
  file[chn]->tty=0;
  file[chn]->infd=-1;
  file[chn]->outfd=fd;
  file[chn]->outPos=0;
  file[chn]->outLineWidth=LINEWIDTH;
  file[chn]->outColWidth=COLWIDTH;
  file[chn]->outSize=0;
  file[chn]->outCapacity=sizeof(file[chn]->outBuf);
  file[chn]->randomfd=-1;
  file[chn]->binaryfd=-1;
  FS_errmsg=(const char*)0;
  ++used;
  return chn;
}
/*}}}*/
int FS_openrandomChn(int chn, const char *name, int mode, int recLength) /*{{{*/
{
  int fd;

  assert(chn>=0);
  assert(name!=(const char*)0);
  assert(recLength>0);
  if (size(chn)==-1) return -1;
  if (file[chn]!=(struct FileStream*)0)
  {
    FS_errmsg=_("channel already open");
    return -1;
  }
  if ((fd=open(name,open_mode[mode]|O_CREAT,0666))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->recLength=recLength;
  file[chn]->dev=0;
  file[chn]->tty=0;
  file[chn]->infd=-1;
  file[chn]->outfd=-1;
  file[chn]->randomfd=fd;
  file[chn]->recBuf=malloc(recLength);
  memset(file[chn]->recBuf,0,recLength);
  StringField_new(&file[chn]->field);
  file[chn]->binaryfd=-1;
  FS_errmsg=(const char*)0;
  ++used;
  return chn;
}
/*}}}*/
int FS_openbinaryChn(int chn, const char *name, int mode) /*{{{*/
{
  int fd;

  assert(chn>=0);
  assert(name!=(const char*)0);
  if (size(chn)==-1) return -1;
  if (file[chn]!=(struct FileStream*)0)
  {
    FS_errmsg=_("channel already open");
    return -1;
  }
  if ((fd=open(name,open_mode[mode]|O_CREAT,0666))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  file[chn]=malloc(sizeof(struct FileStream));
  file[chn]->recLength=1;
  file[chn]->dev=0;
  file[chn]->tty=0;
  file[chn]->infd=-1;
  file[chn]->outfd=-1;
  file[chn]->randomfd=-1;
  file[chn]->binaryfd=fd;
  FS_errmsg=(const char*)0;
  ++used;
  return chn;
}
/*}}}*/
int FS_freechn(void) /*{{{*/
{
  int i;

  for (i=0; i<capacity && file[i]; ++i);
  if (size(i)==-1) return -1;
  return i;
}
/*}}}*/
int FS_flush(int dev) /*{{{*/
{
  ssize_t written;
  size_t offset;

  if (file[dev]==(struct FileStream*)0)
  {
    FS_errmsg=_("channel not open");
    return -1;
  }
  offset=0;
  while (offset<file[dev]->outSize)
  {
    written=write(file[dev]->outfd,file[dev]->outBuf+offset,file[dev]->outSize-offset);
    if (written==-1)
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
    else offset+=written;
  }
  file[dev]->outSize=0;
  FS_errmsg=(const char*)0;
  return 0;
}
/*}}}*/
int FS_close(int dev) /*{{{*/
{
  if (file[dev]==(struct FileStream*)0)
  {
    FS_errmsg=_("channel not open");
    return -1;
  }
  if (file[dev]->outfd>=0)
  {
    if (file[dev]->tty && (file[dev]->outforeground!=-1 || file[dev]->outbackground!=-1)) resetcolour(dev);
    FS_flush(dev);
    close(file[dev]->outfd);
  }
  if (file[dev]->randomfd>=0)
  {
    StringField_destroy(&file[dev]->field);
    free(file[dev]->recBuf);
    close(file[dev]->randomfd);
  }
  if (file[dev]->binaryfd>=0)
  {
    close(file[dev]->binaryfd);
  }
  if (file[dev]->tty) tcsetattr(file[dev]->infd,TCSADRAIN,&origMode);
  if (file[dev]->infd>=0) close(file[dev]->infd);
  free(file[dev]);
  file[dev]=(struct FileStream*)0;
  FS_errmsg=(const char*)0;
  if (--used==0)
  {
    free(file);
    capacity=0;
  }
  return 0;
}
/*}}}*/
int FS_istty(int chn) /*{{{*/
{
  return (file[chn] && file[chn]->tty);
}
/*}}}*/
int FS_lock(int chn, off_t offset, off_t length, int mode, int w) /*{{{*/
{
  int fd;
  struct flock recordLock;

  if (file[chn]==(struct FileStream*)0)
  {
    FS_errmsg=_("channel not open");
    return -1;
  }
  if ((fd=file[chn]->infd)==-1)
  if ((fd=file[chn]->outfd)==-1)
  if ((fd=file[chn]->randomfd)==-1)
  if ((fd=file[chn]->binaryfd)==-1) assert(0);
  recordLock.l_whence=SEEK_SET;
  recordLock.l_start=offset;
  recordLock.l_len=length;
  switch (mode)
  {
    case FS_LOCK_SHARED:    recordLock.l_type=F_RDLCK; break;
    case FS_LOCK_EXCLUSIVE: recordLock.l_type=F_WRLCK; break;
    case FS_LOCK_NONE:      recordLock.l_type=F_UNLCK; break;
    default: assert(0);
  }
  if (fcntl(fd,w ? F_SETLKW : F_SETLK,&recordLock)==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_truncate(int chn) /*{{{*/
{
  int fd;
  off_t o;

  if (file[chn]==(struct FileStream*)0)
  {
    FS_errmsg=_("channel not open");
    return -1;
  }
  if ((fd=file[chn]->infd)==-1)
  if ((fd=file[chn]->outfd)==-1)
  if ((fd=file[chn]->randomfd)==-1)
  if ((fd=file[chn]->binaryfd)==-1) assert(0);
  if ((o=lseek(fd,SEEK_CUR,0))==(off_t)-1 || ftruncate(fd,o+1)==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return 0;
}
/*}}}*/
void FS_shellmode(int dev) /*{{{*/
{
  struct sigaction interrupt;

  if (file[dev]->tty) tcsetattr(file[dev]->infd,TCSADRAIN,&origMode);
  interrupt.sa_flags=0;
  sigemptyset(&interrupt.sa_mask);
  interrupt.sa_handler=SIG_IGN;
  sigaction(SIGINT,&interrupt,&old_sigint);
  sigaction(SIGQUIT,&interrupt,&old_sigquit);
}
/*}}}*/
void FS_fsmode(int chn) /*{{{*/
{
  if (file[chn]->tty) tcsetattr(file[chn]->infd,TCSADRAIN,&rawMode);
  sigaction(SIGINT,&old_sigint,(struct sigaction *)0);
  sigaction(SIGQUIT,&old_sigquit,(struct sigaction *)0);
}
/*}}}*/
void FS_xonxoff(int chn, int on) /*{{{*/
{
  if (file[chn]->tty)
  {
    if (on) rawMode.c_iflag|=(IXON|IXOFF);
    else rawMode.c_iflag&=~(IXON|IXOFF);
    tcsetattr(file[chn]->infd,TCSADRAIN,&rawMode);
  }
}
/*}}}*/
int FS_put(int chn) /*{{{*/
{
  ssize_t offset,written;

  if (opened(chn,2)==-1) return -1;
  offset=0;
  while (offset<file[chn]->recLength)
  {
    written=write(file[chn]->randomfd,file[chn]->recBuf+offset,file[chn]->recLength-offset);
    if (written==-1)
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
    else offset+=written;
  }
  FS_errmsg=(const char*)0;
  return 0;
}
/*}}}*/
int FS_putChar(int dev, char ch) /*{{{*/
{
  struct FileStream *f;

  if (opened(dev,0)==-1) return -1;
  f=file[dev];
  if (ch=='\n') f->outPos=0;
  if (ch=='\b' && f->outPos) --f->outPos;
  if (f->outSize+2>=f->outCapacity && FS_flush(dev)==-1) return -1;
  if (f->outLineWidth && f->outPos==f->outLineWidth)
  {
    if (FS_istty(dev)) carriage_return(dev);
    else f->outBuf[f->outSize++]='\n';
    f->outPos=0;
  }
  if (FS_istty(dev) && ch=='\n') carriage_return(dev);
  else f->outBuf[f->outSize++]=ch;
  if (ch!='\n' && ch!='\b') ++f->outPos;
  FS_errmsg=(const char*)0;
  return 0;
}
/*}}}*/
int FS_putChars(int dev, const char *chars) /*{{{*/
{
  while (*chars) if (FS_putChar(dev,*chars++)==-1) return -1;
  return 0;
}
/*}}}*/
int FS_putString(int dev, const struct String *s) /*{{{*/
{
  size_t len=s->length;
  const char *c=s->character;

  while (len) if (FS_putChar(dev,*c++)==-1) return -1; else --len;
  return 0;
}
/*}}}*/
int FS_putItem(int dev, const struct String *s) /*{{{*/
{
  struct FileStream *f;

  if (opened(dev,0)==-1) return -1;
  f=file[dev];
  if (f->outPos && f->outPos+s->length>f->outLineWidth) FS_nextline(dev);
  return FS_putString(dev, s);
}
/*}}}*/
int FS_putbinaryString(int chn, const struct String *s) /*{{{*/
{
  if (opened(chn,3)==-1) return -1;
  if (s->length && write(file[chn]->binaryfd,s->character,s->length)!=s->length)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_putbinaryInteger(int chn, long int x) /*{{{*/
{
  char s[sizeof(long int)];
  int i;

  if (opened(chn,3)==-1) return -1;
  for (i=0; i<sizeof(x); ++i,x>>=8) s[i]=(x&0xff);
  if (write(file[chn]->binaryfd,s,sizeof(s))!=sizeof(s))
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_putbinaryReal(int chn, double x) /*{{{*/
{
  if (opened(chn,3)==-1) return -1;
  if (write(file[chn]->binaryfd,&x,sizeof(x))!=sizeof(x))
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_getbinaryString(int chn, struct String *s) /*{{{*/
{
  ssize_t len;

  if (opened(chn,3)==-1) return -1;
  if (s->length && (len=read(file[chn]->binaryfd,s->character,s->length))!=s->length)
  {
    if (len==-1) FS_errmsg=strerror(errno);
    else FS_errmsg=_("End of file");
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_getbinaryInteger(int chn, long int *x) /*{{{*/
{
  char s[sizeof(long int)];
  int i;
  ssize_t len;

  if (opened(chn,3)==-1) return -1;
  if ((len=read(file[chn]->binaryfd,s,sizeof(s)))!=sizeof(s))
  {
    if (len==-1) FS_errmsg=strerror(errno);
    else FS_errmsg=_("End of file");
    return -1;
  }
  *x=(s[sizeof(x)-1]<0) ? -1 : 0;
  for (i=sizeof(s)-1; i>=0; --i) *x=(*x<<8)|(s[i]&0xff);
  return 0;
}
/*}}}*/
int FS_getbinaryReal(int chn, double *x) /*{{{*/
{
  ssize_t len;

  if (opened(chn,3)==-1) return -1;
  if ((len=read(file[chn]->binaryfd,x,sizeof(*x)))!=sizeof(*x))
  {
    if (len==-1) FS_errmsg=strerror(errno);
    else FS_errmsg=_("End of file");
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_nextcol(int dev) /*{{{*/
{
  struct FileStream *f;

  if (opened(dev,0)==-1) return -1;
  f=file[dev];
  if
  (
    f->outPos%f->outColWidth
    && f->outLineWidth
    && ((f->outPos/f->outColWidth+2)*f->outColWidth)>f->outLineWidth
  )
  {
    return FS_putChar(dev,'\n');
  }
  if (!(f->outPos%f->outColWidth) && FS_putChar(dev,' ')==-1) return -1;
  while (f->outPos%f->outColWidth) if (FS_putChar(dev,' ')==-1) return -1;
  return 0;
}
/*}}}*/
int FS_nextline(int dev) /*{{{*/
{
  struct FileStream *f;

  if (opened(dev,0)==-1) return -1;
  f=file[dev];
  if
  (
    f->outPos
    && FS_putChar(dev,'\n')==-1
  ) return -1;
  return 0;
}
/*}}}*/
int FS_tab(int dev, int position) /*{{{*/
{
  struct FileStream *f=file[dev];

  if (f->outLineWidth && position>=f->outLineWidth) position=f->outLineWidth-1;
  while (f->outPos<(position-1)) if (FS_putChar(dev,' ')==-1) return -1;
  return 0;
}
/*}}}*/
int FS_width(int dev, int width) /*{{{*/
{
  if (opened(dev,0)==-1) return -1;
  if (width<0)
  {
    FS_errmsg=_("negative width");
    return -1;
  }
  file[dev]->outLineWidth=width;
  return 0;
}
/*}}}*/
int FS_zone(int dev, int zone) /*{{{*/
{
  if (opened(dev,0)==-1) return -1;
  if (zone<=0)
  {
    FS_errmsg=_("non-positive zone width");
    return -1;
  }
  file[dev]->outColWidth=zone;
  return 0;
}
/*}}}*/
int FS_cls(int chn) /*{{{*/
{
  struct FileStream *f;

  if (opened(chn,0)==-1) return -1;
  f=file[chn];
  if (!f->tty)
  {
    FS_errmsg=_("not a terminal");
    return -1;
  }
  if (cls(chn)==-1) return -1;
  if (FS_flush(chn)==-1) return -1;
  f->outPos=0;
  return 0;
}
/*}}}*/
int FS_locate(int chn, int line, int column) /*{{{*/
{
  struct FileStream *f;

  if (opened(chn,0)==-1) return -1;
  f=file[chn];
  if (!f->tty)
  {
    FS_errmsg=_("not a terminal");
    return -1;
  }
  if (locate(chn,line,column)==-1) return -1;
  if (FS_flush(chn)==-1) return -1;
  f->outPos=column-1;
  return 0;
}
/*}}}*/
int FS_colour(int chn, int foreground, int background) /*{{{*/
{
  struct FileStream *f;

  if (opened(chn,0)==-1) return -1;
  f=file[chn];
  if (!f->tty)
  {
    FS_errmsg=_("not a terminal");
    return -1;
  }
  if (colour(chn,foreground,background)==-1) return -1;
  f->outforeground=foreground;
  f->outbackground=background;
  return 0;
}
/*}}}*/
int FS_getChar(int dev) /*{{{*/
{
  struct FileStream *f;

  if (opened(dev,1)==-1) return -1;
  f=file[dev];
  if (f->inSize==f->inCapacity && refill(dev)==-1) return -1;
  FS_errmsg=(const char*)0;
  if (f->inSize+1==f->inCapacity)
  {
    char ch=f->inBuf[f->inSize];

    f->inSize=f->inCapacity=0;
    return ch;
  }
  else return f->inBuf[f->inSize++];
}
/*}}}*/
int FS_get(int chn) /*{{{*/
{
  ssize_t offset,rd;

  if (opened(chn,2)==-1) return -1;
  offset=0;
  while (offset<file[chn]->recLength)
  {
    rd=read(file[chn]->randomfd,file[chn]->recBuf+offset,file[chn]->recLength-offset);
    if (rd==-1)
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
    else offset+=rd;
  }
  FS_errmsg=(const char*)0;
  return 0;
}
/*}}}*/
int FS_inkeyChar(int dev, int ms) /*{{{*/
{
  struct FileStream *f;
  char c;
  ssize_t len;
#ifdef USE_SELECT
  fd_set just_infd;
  struct timeval timeout;
#else
  struct termios timedread;
#endif

  if (opened(dev,1)==-1) return -1;
  f=file[dev];
  if (f->inSize<f->inCapacity) return f->inBuf[f->inSize++];

#ifdef USE_SELECT
  FD_ZERO(&just_infd);
  FD_SET(f->infd,&just_infd);
  timeout.tv_sec=ms/1000;
  timeout.tv_usec=(ms%1000)*1000;
  switch (select(f->infd+1,&just_infd,(fd_set*)0,(fd_set*)0,&timeout))
  {
    case 1:
    {
      FS_errmsg=(const char*)0;
      len=read(f->infd,&c,1);
      return (len==1?c:-1);
    }
    case 0:
    {
      FS_errmsg=(const char*)0;
      return -1;
    }
    case -1:
    {
      FS_errmsg=strerror(errno);
      return -1;
    }
    default: assert(0);
  }
  return 0;
#else
  timedread=rawMode;
  timedread.c_cc[VMIN]=0;
  timedread.c_cc[VTIME]=(ms?ms:100)/100;
  if (tcsetattr(f->infd,TCSADRAIN,&timedread)==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  FS_errmsg=(const char*)0;
  len=read(f->infd,&c,1);
  tcsetattr(f->infd,TCSADRAIN,&rawMode);
  if (len==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return (len==1?c:-1);
#endif
}
/*}}}*/
void FS_sleep(double s) /*{{{*/
{
#ifdef HAVE_NANOSLEEP
  struct timespec p;

  p.tv_sec=floor(s);
  p.tv_nsec=1000000000*(s-floor(s));

  nanosleep(&p,(struct timespec*)0);
#else
  sleep((int)s);
#endif
}
/*}}}*/
int FS_eof(int chn) /*{{{*/
{
  struct FileStream *f;

  if (opened(chn,1)==-1) return -1;
  f=file[chn];
  if (f->inSize==f->inCapacity && refill(chn)==-1) return 1;
  return 0;
}
/*}}}*/
long int FS_loc(int chn) /*{{{*/
{
  int fd;
  off_t cur,offset=0;

  if (opened(chn,-1)==-1) return -1;
  if (file[chn]->infd!=-1) { fd=file[chn]->infd; offset=-file[chn]->inCapacity+file[chn]->inSize; }
  else if (file[chn]->outfd!=-1) { fd=file[chn]->outfd; offset=file[chn]->outSize; }
  else if (file[chn]->randomfd!=-1) fd=file[chn]->randomfd;
  else fd=file[chn]->binaryfd;
  assert(fd!=-1);
  if ((cur=lseek(fd,0,SEEK_CUR))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return (cur+offset)/file[chn]->recLength;
}
/*}}}*/
long int FS_lof(int chn) /*{{{*/
{
  struct stat buf;
  int fd;

  if (opened(chn,-1)==-1) return -1;
  if (file[chn]->infd!=-1) fd=file[chn]->infd;
  else if (file[chn]->outfd!=-1) fd=file[chn]->outfd;
  else if (file[chn]->randomfd!=-1) fd=file[chn]->randomfd;
  else fd=file[chn]->binaryfd;
  assert(fd!=-1);
  if (fstat(fd,&buf)==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return buf.st_size/file[chn]->recLength;
}
/*}}}*/
long int FS_recLength(int chn) /*{{{*/
{
  if (opened(chn,2)==-1) return -1;
  return file[chn]->recLength;
}
/*}}}*/
void FS_field(int chn, struct String *s, long int position, long int length) /*{{{*/
{
  assert(file[chn]);
  String_joinField(s,&file[chn]->field,file[chn]->recBuf+position,length);
}
/*}}}*/
int FS_seek(int chn, long int record) /*{{{*/
{
  if (opened(chn,2)!=-1)
  {
    if (lseek(file[chn]->randomfd,(off_t)record*file[chn]->recLength,SEEK_SET)!=-1) return 0;
    FS_errmsg=strerror(errno);
  }
  else if (opened(chn,4)!=-1)
  {
    if (lseek(file[chn]->binaryfd,(off_t)record,SEEK_SET)!=-1) return 0;
    FS_errmsg=strerror(errno);
  }
  return -1;
}
/*}}}*/
int FS_appendToString(int chn, struct String *s, int onl) /*{{{*/
{
  size_t new;
  char *n;
  struct FileStream *f=file[chn];
  int c;

  if (f->tty && f->inSize==f->inCapacity)
  {
    if (edit(chn,onl)==-1) return (FS_errmsg ? -1 : 0);
  }
  do
  {
    n=f->inBuf+f->inSize;
    while (1)
    {
      if (n==f->inBuf+f->inCapacity) break;
      c=*n++;
      if (c=='\n') break;
    }
    new=n-(f->inBuf+f->inSize);
    if (new)
    {
      size_t offset=s->length;

      if (String_size(s,offset+new)==-1)
      {
        FS_errmsg=strerror(errno);
        return -1;
      }
      memcpy(s->character+offset,f->inBuf+f->inSize,new);
      f->inSize+=new;
      if (*(n-1)=='\n')
      {
        if (f->inSize==f->inCapacity) f->inSize=f->inCapacity=0;
        if (s->length>=2 && s->character[s->length-2]=='\r')
        {
          s->character[s->length-2]='\n';
          --s->length;
        }
        return 0;
      }
    }
    if ((c=FS_getChar(chn))>=0) String_appendChar(s,c);
    if (c=='\n')
    {
      if (s->length>=2 && s->character[s->length-2]=='\r')
      {
        s->character[s->length-2]='\n';
        --s->length;
      }
      return 0;
    }
  } while (c!=-1);
  return (FS_errmsg ? -1 : 0);
}
/*}}}*/
void FS_closefiles(void) /*{{{*/
{
  int i;

  for (i=0; i<capacity; ++i) if (file[i] && !file[i]->dev) FS_close(i);
}
/*}}}*/
int FS_charpos(int chn) /*{{{*/
{
  if (file[chn]==(struct FileStream*)0)
  {
    FS_errmsg=_("channel not open");
    return -1;
  }
  return (file[chn]->outPos);
}
/*}}}*/
int FS_copy(const char *from, const char *to) /*{{{*/
{
  int infd,outfd;
  char buf[4096];
  ssize_t inlen,outlen=-1;

  if ((infd=open(from,O_RDONLY))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  if ((outfd=open(to,O_WRONLY|O_CREAT|O_TRUNC,0666))==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  while ((inlen=read(infd,&buf,sizeof(buf)))>0)
  {
    ssize_t off=0;

    while (inlen && (outlen=write(outfd,&buf+off,inlen))>0)
    {
      off+=outlen;
      inlen-=outlen;
    }
    if (outlen==-1)
    {
      FS_errmsg=strerror(errno);
      close(infd);
      close(outfd);
      return -1;
    }
  }
  if (inlen==-1)
  {
    FS_errmsg=strerror(errno);
    close(infd);
    close(outfd);
    return -1;
  }
  if (close(infd)==-1)
  {
    FS_errmsg=strerror(errno);
    close(outfd);
    return -1;
  }
  if (close(outfd)==-1)
  {
    FS_errmsg=strerror(errno);
    return -1;
  }
  return 0;
}
/*}}}*/
int FS_portInput(int address) /*{{{*/
{
  FS_errmsg=_("Direct port access not available");
  return -1;
}
/*}}}*/
int FS_memInput(int address) /*{{{*/
{
  FS_errmsg=_("Direct memory access not available");
  return -1;
}
/*}}}*/
int FS_portOutput(int address, int value) /*{{{*/
{
  FS_errmsg=_("Direct port access not available");
  return -1;
}
/*}}}*/
int FS_memOutput(int address, int value) /*{{{*/
{
  FS_errmsg=_("Direct memory access not available");
  return -1;
}
/*}}}*/
void FS_allowIntr(int on) /*{{{*/
{
  struct sigaction breakact;

  breakact.sa_handler=on ? sigintr : SIG_IGN;
  sigemptyset(&breakact.sa_mask);
  sigaddset(&breakact.sa_mask,SIGINT);
  breakact.sa_flags=0;
  sigaction(SIGINT,&breakact,(struct sigaction *)0);
}
/*}}}*/
