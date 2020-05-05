/****************************************************************************
 * apps/system/vi/vi.c
 *
 *   Copyright (C) 2014, 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Major Edits 2019, Ken Pettit <pettitkd@gmail.com>
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

#include <sys/stat.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <debug.h>

#include <system/termcurses.h>
#include <graphics/curses.h>

#include <nuttx/ascii.h>
#include <nuttx/vt100.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_VI_ROWS
#  define CONFIG_SYSTEM_VI_ROWS 16
#endif

#ifndef CONFIG_SYSTEM_VI_COLS
#  define CONFIG_SYSTEM_VI_COLS 64
#endif

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

#ifndef CONFIG_SYSTEM_VI_YANK_THRESHOLD
#define CONFIG_SYSTEM_VI_YANK_THRESHOLD 128
#endif

/* Control characters */

#undef  CTRL
#define CTRL(a)         ((a) & 0x1f)

#define VI_BEL(vi)      vi_putch(vi,CTRL('G'))

/* Sizes of things */

#define MAX_STRING      64   /* The maximum size of a filename or search string */
#define MAX_FILENAME    128  /* The maximum size of a filename or search string */
#define SCRATCH_BUFSIZE 128  /* The maximum size of the scratch buffer */
#define CMD_BUFSIZE     128  /* The maximum size of the scratch buffer */

#define TEXT_GULP_SIZE  512  /* Text buffer allocations are managed with this unit */
#define TEXT_GULP_MASK  511  /* Mask for aligning buffer allocation sizes */
#define ALIGN_GULP(x)   (((x) + TEXT_GULP_MASK) & ~TEXT_GULP_MASK)

#define VI_TABSIZE      8    /* A TAB is eight characters */
#define TABMASK         7    /* Mask for TAB alignment */
#define NEXT_TAB(p)     (((p) + VI_TABSIZE) & ~TABMASK)

/* Parsed command action bits */

#define CMD_READ        (1 << 0) /* Bit 0: Read */
#define CMD_WRITE_MASK  (3 << 1) /* Bits 1-2: x1=Write operation */
#  define CMD_WRITE     (1 << 1) /*           01=Write (without overwriting) */
#  define CMD_OWRITE    (3 << 1) /*           11=Overwrite */
#define CMD_QUIT_MASK   (3 << 3) /* Bits 3-4: x1=Quit operation */
#  define CMD_QUIT      (1 << 3) /*           01=Quit if saved */
#  define CMD_DISCARD   (3 << 3) /*           11=Quit without saving */

#define CMD_NONE        (0)                     /* No command */
#define CMD_WRITE_QUIT  (CMD_WRITE | CMD_QUIT)  /* Write file and quit command */
#define CMD_OWRITE_QUIT (CMD_OWRITE | CMD_QUIT) /* Overwrite file and quit command */

#define IS_READ(a)      (((uint8_t)(a) & CMD_READ) != 0)
#define IS_WRITE(a)     (((uint8_t)(a) & CMD_WRITE) != 0)
#define IS_OWRITE(a)    (((uint8_t)(a) & CMD_WRITE_MASK) == CMD_OWRITE)
#define IS_NOWRITE(a)   (((uint8_t)(a) & CMD_WRITE_MASK) == CMD_WRITE)
#define IS_QUIT(a)      (((uint8_t)(a) & CMD_QUIT) != 0)
#define IS_DISCARD(a)   (((uint8_t)(a) & CMD_QUIT_MASK) == CMD_DISCARD)
#define IS_NDISCARD(a)  (((uint8_t)(a) & CMD_QUIT_MASK) == CMD_QUIT && !IS_WRITE(a))

#define CMD_FILE_MASK   (CMD_READ | CMD_WRITE)
#define USES_FILE(a)    (((uint8_t)(a) & CMD_FILE_MASK) != 0)

/* Search control */

#define VI_CHAR_SPACE   0
#define VI_CHAR_ALPHA   1
#define VI_CHAR_PUNCT   2
#define VI_CHAR_CRLF    3

/* Output */

#define vi_error(vi, fmt, ...)   vi_printf(vi, "ERROR: ", fmt, ##__VA_ARGS__)
#define vi_message(vi, fmt, ...) vi_printf(vi, NULL, fmt, ##__VA_ARGS__)

/* Debug */

#ifndef CONFIG_SYSTEM_VI_DEBUGLEVEL
#  define CONFIG_SYSTEM_VI_DEBUGLEVEL 0
#endif

#if CONFIG_SYSTEM_VI_DEBUGLEVEL > 0
#  define vidbg(format, ...) \
     syslog(LOG_DEBUG, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#  define vvidbg(format, ap) \
     vsyslog(LOG_DEBUG, format, ap)
#else
#  define vidbg(x...)
#  define vvidbg(x...)
#endif

#if CONFIG_SYSTEM_VI_DEBUGLEVEL > 1
#  define viinfo(format, ...) \
     syslog(LOG_DEBUG, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define viinfo(x...)
#endif

/* Uncomment to enable bottom line debug printing.  Useful during yank /
 * paste debugging, etc.
 */

/* #define ENABLE_BOTTOM_LINE_DEBUG */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* VI Key Bindings */

enum vi_cmdmode_key_e
{
  KEY_CMDMODE_BEGINLINE   = '0',  /* Move cursor to start of current line */
  KEY_CMDMODE_APPEND      = 'a',  /* Enter insertion mode after current character */
  KEY_CMDMODE_WORDBACK    = 'b',  /* Scan to previous word */
  KEY_CMDMODE_CHANGE      = 'c',  /* Delete text and enter insert mode */
  KEY_CMDMODE_DEL_LINE    = 'd',  /* "dd" deletes a lines */
  KEY_CMDMODE_FINDINLINE  = 'f',  /* Find within current line */
  KEY_CMDMODE_GOTOTOP     = 'g',  /* Two of these sends cursor to the top */
  KEY_CMDMODE_LEFT        = 'h',  /* Move left one character */
  KEY_CMDMODE_INSERT      = 'i',  /* Enter insertion mode before current character */
  KEY_CMDMODE_DOWN        = 'j',  /* Move down one line */
  KEY_CMDMODE_UP          = 'k',  /* Move up one line */
  KEY_CMDMODE_RIGHT       = 'l',  /* Move right one character */
  KEY_CMDMODE_MARK        = 'm',  /* Place a mark beginning at the current cursor position */
  KEY_CMDMODE_FINDNEXT    = 'n',  /* Find next */
  KEY_CMDMODE_OPENBELOW   = 'o',  /* Enter insertion mode in new line below current */
  KEY_CMDMODE_PASTE       = 'p',  /* Paste line(s) from into text after current line */
  KEY_CMDMODE_REPLACECH   = 'r',  /* Replace character(s) under cursor */
  KEY_CMDMODE_SUBSTITUTE  = 's',  /* Substitute character with new text */
  KEY_CMDMODE_TFINDINLINE = 't',  /* Find within current line, cursor at previous char */
  KEY_CMDMODE_WORDFWD     = 'w',  /* Scan to next word */
  KEY_CMDMODE_DEL         = 'x',  /* Delete a single character */
  KEY_CMDMODE_YANK        = 'y',  /* "yy" yanks the current line(s) into the buffer */

  KEY_CMDMODE_CR          = '\r', /* CR moves to first non-space on next line */
  KEY_CMDMODE_NL          = '\n', /* NL moves to first non-space on next line */

  KEY_CMDMODE_APPENDEND   = 'A',  /* Enter insertion mode at the end of the current line */
  KEY_CMDMODE_CHANGETOEOL = 'C',  /* Change (del to EOL and go to insert mode */
  KEY_CMDMODE_DELTOEOL    = 'D',  /* Delete to End Of Line */
  KEY_CMDMODE_GOTO        = 'G',  /* Got to line */
  KEY_CMDMODE_TOP         = 'H',  /* Got to top of screen */
  KEY_CMDMODE_JOIN        = 'J',  /* Join line below with current line */
  KEY_CMDMODE_BOTTOM      = 'L',  /* Got to bottom of screen */
  KEY_CMDMODE_INSBEGIN    = 'I',  /* Enter insertion mode at the beginning of the current */
  KEY_CMDMODE_MIDDLE      = 'M',  /* Got to middle of screen */
  KEY_CMDMODE_FINDPREV    = 'N',  /* Find previous */
  KEY_CMDMODE_OPENABOVE   = 'O',  /* Enter insertion mode in new line above current line */
  KEY_CMDMODE_PASTEBEFORE = 'P',  /* Paste text before cursor location */
  KEY_CMDMODE_REPLACE     = 'R',  /* Replace character(s) under cursor until ESC */
  KEY_CMDMODE_DELBACKWARD = 'X',  /* Replace character(s) under cursor until ESC */
  KEY_CMDMODE_SAVEQUIT    = 'Z',  /* Another one is the same as "wq" */

  KEY_CMDMODE_COLONMODE   = ':',  /* The next character command prefaced with a colon */
  KEY_CMDMODE_FINDMODE    = '/',  /* Enter forward search string */
  KEY_CMDMODE_ENDLINE     = '$',  /* Move cursor to end of current line */
  KEY_CMDMODE_REVFINDMODE = '?',  /* Enter forward search string */
  KEY_CMDMODE_REPEAT      = '.',  /* Repeat last command */
  KEY_CMDMODE_FIRSTCHAR   = '^',  /* Find first non-whitespace character on line */
  KEY_CMDMODE_NEXTLINE    = '+',  /* Find first non-whitespace character on line */
  KEY_CMDMODE_PREVLINE    = '-',  /* Find first non-whitespace character on line */

  KEY_CMDMODE_PAGEUP      = CTRL('b'), /* Move backward one screen */
  KEY_CMDMODE_HALFDOWN    = CTRL('d'), /* Move down (forward) one half screen */
  KEY_CMDMODE_PAGEDOWN    = CTRL('f'), /* Move forward one screen */
  KEY_CMDMODE_REDRAW      = CTRL('l'), /* Redraws the screen */
  KEY_CMDMODE_REDRAW2     = CTRL('r'), /* Redraws the screen, removing deleted lines */
  KEY_CMDMODE_HALFUP      = CTRL('u')  /* Move up (back) one half screen */
};

enum vi_insmode_key_e
{
  KEY_INSMODE_QUOTE       = '\\', /* The next character is quote (use literal value) */
};

enum vi_colonmode_key_e
{
  KEY_COLMODE_READ        = 'r',  /* Read file */
  KEY_COLMODE_QUIT        = 'q',  /* Quit vi */
  KEY_COLMODE_WRITE       = 'w',  /* Write file */
  KEY_COLMODE_FORCE       = '!',  /* Force operation */
  KEY_COLMODE_QUOTE       = '\\'  /* The next character is quote (use literal value) */
};

enum vi_findmode_key_e
{
  KEY_FINDMODE_QUOTE      = '\\'  /* The next character is quote (use literal value) */
};

/* VI modes */

enum vi_mode_s
{
  MODE_COMMAND            = 0,    /* ESC         Command mode */
  SUBMODE_COLON,                  /* :           Command sub-mode */
  SUBMODE_FIND,                   /* /           Search sub-mode */
  SUBMODE_REVFIND,                /* ?           Search sub-mode */
  SUBMODE_REPLACECH,              /* r           Replace sub-mode 1 */
  MODE_INSERT,                    /* i,I,a,A,o,O Insert mode */
  MODE_REPLACE,                   /* R           Replace sub-mode 2 */
  MODE_FINDINLINE                 /* f           Find in line sub-mode */
};

/* This structure represents a cursor position */

struct vi_pos_s
{
  uint16_t row;
  uint16_t column;
};

#ifdef CONFIG_SYSTEM_VI_INCLUDE_UNDO
/* The undo structure.  Will be implemented soon. */

struct vi_undo_s
{
  off_t     curpos;                 /* Cursor position before operation */
  off_t     ybytes;                 /* Bytes yanked */
  off_t     ibytes;                 /* Bytes inserted */
  FAR char *yank;                   /* The yanked bytes */
  FAR char *insert;                 /* The inserted bytes */
  uint8_t   type;                   /* Type of operation */
  uint8_t   complete;               /* Indicates if this operation complete */
};
#endif

/* This structure describes the overall state of the editor */

struct vi_s
{
  struct vi_pos_s cursor;   /* Current cursor position */
  struct vi_pos_s cursave;  /* Saved cursor position */
  struct vi_pos_s display;  /* Display size */
  FAR struct termcurses_s * tcurs;
  off_t curpos;             /* The current cursor offset into the text buffer */
  off_t textsize;           /* The size of the text buffer */
  off_t winpos;             /* Offset corresponding to the start of the display */
  off_t prevpos;            /* Previous display position */
  off_t vscroll;            /* Vertical display offset in rows */
  uint16_t hscroll;         /* Horizontal display offset */
  uint16_t value;           /* Numeric value entered prior to a command */
  uint16_t reqcolumn;       /* Requested column when moving up/down */
  uint8_t mode;             /* See enum vi_mode_s */
  uint8_t cmdlen;           /* Length of the command in the scratch[] buffer */
  bool modified;            /* True: The file has modified */
  bool error;               /* True: There is an error message on the last line */
  bool delarm;              /* Delete text arm flag */
  bool chgarm;              /* Change text arm flag */
  bool yankarm;             /* Yank text arm flag */
  bool toparm;              /* One more 'g' and the cursor moves to the top */
  bool wqarm;               /* One more 'Z' is the same as :wq */
  bool fullredraw;          /* True to draw all lines on screen */
  bool drawtoeos;           /* True to draw all lines to end of screen */
  bool redrawline;          /* True to draw current line */
  bool updatereqcol;        /* True to update the requested column */
  bool tfind;               /* Find in line 't' mode */
  bool revfind;             /* In ? reverse find mode */
  bool yankcharmode;        /* Indicates yank buffer is char vs. line mode */

  /* Buffers */

  FAR char *text;           /* Dynamically allocated text buffer */
  size_t txtalloc;          /* Current allocated size of the text buffer */
  FAR char *yank;           /* Dynamically allocated yank buffer */
  size_t yankalloc;         /* Current allocated size of the yank buffer */
  size_t yanksize;          /* Current size of the text in the yank buffer */

  char filename[MAX_FILENAME];    /* Holds the currently selected filename */
  char findstr[MAX_STRING];       /* Holds the current search string */
  char scratch[SCRATCH_BUFSIZE];  /* For general, scratch usage */

#ifdef CONFIG_SYSTEM_VI_INCLUDE_UNDO
  struct vi_undo_s undo[CONFIG_SYSTEM_VI_UNDO_LEVELS];
  uint16_t undocount;       /* Number of valid undo entries */
  uint16_t undoindex;       /* Current index in undo/redo stack */
#endif

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
  char cmdbuf[CMD_BUFSIZE]; /* Last command buffer */
  uint16_t cmdindex;        /* Current index within cmdbuf */
  uint16_t cmdcount;        /* Count of entries in cmdbuf */
  uint16_t repeatvalue;     /* The command repeat value */
  bool cmdrepeat;           /* Command repeat is active */
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Low-level display and data entry functions */

static void     vi_write(FAR struct vi_s *vi, FAR const char *buffer,
                  size_t buflen);
static void     vi_putch(FAR struct vi_s *vi, char ch);
static int      vi_getch(FAR struct vi_s *vi);
#if 0 /* Not used */
static void     vi_blinkon(FAR struct vi_s *vi);
#endif
static void     vi_boldon(FAR struct vi_s *vi);
static void     vi_reverseon(FAR struct vi_s *vi);
static void     vi_attriboff(FAR struct vi_s *vi);
static void     vi_cursoron(FAR struct vi_s *vi);
static void     vi_cursoroff(FAR struct vi_s *vi);
#if 0 /* Not used */
static void     vi_cursorhome(FAR struct vi_s *vi);
#endif
static void     vi_setcursor(FAR struct vi_s *vi, uint16_t row,
                  uint16_t column);
static void     vi_clrtoeol(FAR struct vi_s *vi);
#if 0 /* Not used */
static void     vi_clrscreen(FAR struct vi_s *vi);
#endif

/* Final Line display */

static void     vi_printf(FAR struct vi_s *vi, FAR const char *prefix,
                  FAR const char *fmt, ...);

/* Line positioning */

static off_t    vi_linebegin(FAR struct vi_s *vi, off_t pos);
static off_t    vi_prevline(FAR struct vi_s *vi, off_t pos);
static off_t    vi_lineend(FAR struct vi_s *vi, off_t pos);
static off_t    vi_nextline(FAR struct vi_s *vi, off_t pos);

/* Text buffer management */

static bool     vi_extendtext(FAR struct vi_s *vi, off_t pos,
                  size_t increment);
static void     vi_shrinkpos(FAR struct vi_s *vi, off_t delpos,
                  size_t delsize, FAR off_t *pos);
static void     vi_shrinktext(FAR struct vi_s *vi, off_t pos, size_t size);

/* File access */

static bool     vi_insertfile(FAR struct vi_s *vi, off_t pos,
                  FAR const char *filename);
static bool     vi_savetext(FAR struct vi_s *vi, FAR const char *filename,
                  off_t pos, size_t size);
static bool     vi_checkfile(FAR struct vi_s *vi, FAR const char *filename);

/* Mode management */

static void     vi_setmode(FAR struct vi_s *vi, uint8_t mode, long value);
static void     vi_setsubmode(FAR struct vi_s *vi, uint8_t mode,
                  char prompt, long value);
static void     vi_exitsubmode(FAR struct vi_s *vi, uint8_t mode);

/* Display management */

static void     vi_windowpos(FAR struct vi_s *vi, off_t start, off_t end,
                  uint16_t *pcolumn, off_t *ppos);
static void     vi_scrollcheck(FAR struct vi_s *vi);
static void     vi_showtext(FAR struct vi_s *vi);
static void     vi_showlinecol(FAR struct vi_s *vi);

/* Command mode */

static void     vi_cusorup(FAR struct vi_s *vi, int nlines);
static void     vi_cursordown(FAR struct vi_s *vi, int nlines);
static off_t    vi_cursorleft(FAR struct vi_s *vi, off_t curpos,
                  int ncolumns);
static off_t    vi_cursorright(FAR struct vi_s *vi, off_t curpos,
                  int ncolumns);
static void     vi_delforward(FAR struct vi_s *vi);
static void     vi_delbackward(FAR struct vi_s *vi);
static void     vi_linerange(FAR struct vi_s *vi, off_t *start, off_t *end);
static void     vi_delline(FAR struct vi_s *vi);
static void     vi_deltoeol(FAR struct vi_s *vi);
static void     vi_yanktext(FAR struct vi_s *vi, off_t start, off_t end,
                  bool yankcharmode, bool del_after_yank);
static void     vi_yank(FAR struct vi_s *vi, bool del_after_yank);
static void     vi_paste(FAR struct vi_s *vi, bool paste_before);
static void     vi_gotoline(FAR struct vi_s *vi);
static void     vi_join(FAR struct vi_s *vi);
static void     vi_cmd_mode(FAR struct vi_s *vi);
static int      vi_gotoscreenbottom(FAR struct vi_s *vi, int rows);
static void     vi_gotofirstnonwhite(FAR struct vi_s *vi);

/* Command sub-modes */

static void     vi_cmdch(FAR struct vi_s *vi, char ch);
static void     vi_cmdbackspace(FAR struct vi_s *vi);

static void     vi_parsecolon(FAR struct vi_s *vi);
static void     vi_cmd_submode(FAR struct vi_s *vi);

static bool     vi_findstring(FAR struct vi_s *vi);
static bool     vi_revfindstring(FAR struct vi_s *vi);
static void     vi_parsefind(FAR struct vi_s *vi, bool revfind);
static void     vi_find_submode(FAR struct vi_s *vi, bool revfind);

static void     vi_replacech(FAR struct vi_s *vi, char ch);
static void     vi_replacech_submode(FAR struct vi_s *vi);

static void     vi_findinline_mode(FAR struct vi_s *vi);

/* Insert and replace modes */

static void     vi_insertch(FAR struct vi_s *vi, char ch);
static void     vi_insert_mode(FAR struct vi_s *vi);

/* Command line processing */

static void     vi_release(FAR struct vi_s *vi);
static void     vi_showusage(FAR struct vi_s *vi, FAR const char *progname,
                  int exitcode);

/* Find next / prev word processing */

static int      vi_chartype(char ch);
static off_t    vi_findnextword(FAR struct vi_s *vi);
static void     vi_gotonextword(FAR struct vi_s *vi);
static off_t    vi_findprevword(FAR struct vi_s *vi);
static void     vi_gotoprevword(FAR struct vi_s *vi);

/* Command repeat processing */

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
static void     vi_saverepeat(FAR struct vi_s *vi, uint16_t ch);
static void     vi_appendrepeat(FAR struct vi_s *vi, uint16_t ch);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* VT100 escape sequences */

static const char g_cursoron[]      = VT100_CURSORON;
static const char g_cursoroff[]     = VT100_CURSOROFF;
#if 0 /* Not used */
static const char g_cursorhome[]    = VT100_CURSORHOME;
#endif
static const char g_erasetoeol[]    = VT100_CLEAREOL;
#if 0 /* Not used */
static const char g_clrscreen[]     = VT100_CLEARSCREEN;
#endif
static const char g_index[]         = VT100_INDEX;
static const char g_revindex[]      = VT100_REVINDEX;
static const char g_attriboff[]     = VT100_MODESOFF;
static const char g_boldon[]        = VT100_BOLD;
static const char g_reverseon[]     = VT100_REVERSE;
#if 0 /* Not used */
static const char g_blinkon[]       = VT100_BLINK;
static const char g_boldoff[]       = VT100_BOLDOFF;
static const char g_reverseoff[]    = VT100_REVERSEOFF;
static const char g_blinkoff[]      = VT100_BLINKOFF;
#endif

static const char g_fmtcursorpos[]  = VT100_FMT_CURSORPOS;

/* Error format strings */

static const char g_fmtallocfail[]  = "Failed to allocate memory";
static const char g_fmtcmdfail[]    = "%s failed: %d";
static const char g_fmtnotfile[]    = "%s is not a regular file";
static const char g_fmtfileexists[] = "File exists (add ! to override)";
static const char g_fmtmodified[]   =
                            "No write since last change (add ! to override)";
static const char g_fmtnotvalid[]   = "Command not valid";
static const char g_fmtnotcmd[]     = "Not an editor command: %s";
static const char g_fmtsrcbot[]     = "search hit BOTTOM, continuing at TOP";
static const char g_fmtsrctop[]     = "search hit TOP, continuing at BOTTOM";
static const char g_fmtinsert[]     = "--INSERT--";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Low-level display and data entry functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_write
 *
 * Description:
 *   Write a sequence of bytes to the console device (stdout, fd = 1).
 *
 ****************************************************************************/

static void vi_write(FAR struct vi_s *vi, FAR const char *buffer,
                     size_t buflen)
{
  ssize_t nwritten;
  size_t  nremaining = buflen;

  /* Loop until all bytes have been successfully written (or until a
   * unrecoverable error is encountered)
   */

  do
    {
      /* Take the next gulp */

      nwritten = write(1, buffer, buflen);

      /* Handle write errors.  write() should neve return 0. */

      if (nwritten <= 0)
        {
          /* EINTR is not really an error; it simply means that a signal was
           * received while waiting for write.
           */

          int errcode = errno;
          if (nwritten == 0 || errcode != EINTR)
            {
              fprintf(stderr, "ERROR: write to stdout failed: %d\n",
                      errcode);
              exit(EXIT_FAILURE);
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
 * Name: vi_putch
 *
 * Description:
 *   Write a single character to the console device.
 *
 ****************************************************************************/

static void vi_putch(FAR struct vi_s *vi, char ch)
{
  vi_write(vi, &ch, 1);
}

/****************************************************************************
 * Name: vi_getch
 *
 * Description:
 *   Get a single character from the console device (stdin, fd = 0).
 *
 ****************************************************************************/

static int vi_getch(FAR struct vi_s *vi)
{
  char buffer;
  ssize_t nread;

  /* Loop until we successfully read a character (or until an unexpected
   * error occurs).
   */

  if (vi->tcurs != NULL)
    {
      int specialkey;
      int modifiers;

      /* Get key from termcurses */

      return termcurses_getkeycode(vi->tcurs, &specialkey, &modifiers);
    }
  else
    {
      do
        {
          /* Read one character from the incoming stream */

          nread = read(0, &buffer, 1);

          /* Check for error or end-of-file. */

          if (nread <= 0)
            {
              /* EINTR is not really an error; it simply means that a signal
               * we received while waiting for input.
               */

              int errcode = errno;
              if (nread == 0 || errcode != EINTR)
                {
                  fprintf(stderr, "ERROR: read from stdin failed: %d\n",
                          errcode);
                  exit(EXIT_FAILURE);
                }
            }
        }
      while (nread < 1);
    }

  /* On success, return the character that was read */

  viinfo("Returning: %c[%02x]\n", isprint(buffer) ? buffer : '.', buffer);
  return buffer;
}

/****************************************************************************
 * Name: vi_clearbottomline
 *
 * Description:
 *   Clear the bottom statusline.
 *
 ****************************************************************************/

static void vi_clearbottomline(FAR struct vi_s *vi)
{
  vi_setcursor(vi, vi->display.row - 1, 0);
  vi_clrtoeol(vi);
}

/****************************************************************************
 * Name: vi_boldon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

static void vi_boldon(FAR struct vi_s *vi)
{
  /* Send the VT100 BOLDON command */

  vi_write(vi, g_boldon, sizeof(g_boldon));
}

/****************************************************************************
 * Name: vi_reverseon
 *
 * Description:
 *   Enable the blinking attribute at the current cursor location
 *
 ****************************************************************************/

static void vi_reverseon(FAR struct vi_s *vi)
{
  /* Send the VT100 REVERSON command */

  vi_write(vi, g_reverseon, sizeof(g_reverseon));
}

/****************************************************************************
 * Name: vi_attriboff
 *
 * Description:
 *   Disable all previously selected attributes.
 *
 ****************************************************************************/

static void vi_attriboff(FAR struct vi_s *vi)
{
  /* Send the VT100 ATTRIBOFF command */

  vi_write(vi, g_attriboff, sizeof(g_attriboff));
}

/****************************************************************************
 * Name: vi_cursoron
 *
 * Description:
 *   Turn on the cursor
 *
 ****************************************************************************/

static void vi_cursoron(FAR struct vi_s *vi)
{
  /* Send the VT100 CURSORON command */

  vi_write(vi, g_cursoron, sizeof(g_cursoron));
}

/****************************************************************************
 * Name: vi_cursoroff
 *
 * Description:
 *   Turn off the cursor
 *
 ****************************************************************************/

static void vi_cursoroff(FAR struct vi_s *vi)
{
  /* Send the VT100 CURSOROFF command */

  vi_write(vi, g_cursoroff, sizeof(g_cursoroff));
}

/****************************************************************************
 * Name: vi_setcursor
 *
 * Description:
 *   Move the current cursor position to position (row,col)
 *
 ****************************************************************************/

static void vi_setcursor(FAR struct vi_s *vi, uint16_t row, uint16_t column)
{
  char buffer[16];
  int len;

  viinfo("row=%d column=%d\n", row, column);

  /* Format the cursor position command.  The origin is (1,1). */

  len = snprintf(buffer, 16, g_fmtcursorpos, row + 1, column + 1);

  /* Send the VT100 CURSORPOS command */

  vi_write(vi, buffer, len);
}

/****************************************************************************
 * Name: vi_clrtoeol
 *
 * Description:
 *   Clear the display from the current cursor position to the end of the
 *   current line.
 *
 ****************************************************************************/

static void vi_clrtoeol(FAR struct vi_s *vi)
{
  /* Send the VT100 ERASETOEOL command */

  vi_write(vi, g_erasetoeol, sizeof(g_erasetoeol));
}

/****************************************************************************
 * Name: vi_scrollup
 *
 * Description:
 *   Scroll the display up 'nlines' by sending the VT100 INDEX command.
 *
 ****************************************************************************/

static void vi_scrollup(FAR struct vi_s *vi, uint16_t nlines)
{
  viinfo("nlines=%d\n", nlines);

  /* Scroll for the specified number of lines */

  for (; nlines; nlines--)
    {
      /* Send the VT100 INDEX command */

      vi_write(vi, g_index, sizeof(g_index));
    }

  /* Ensure bottom line is cleared */

  vi_setcursor(vi, vi->display.row - 1, 0);
  vi_clrtoeol(vi);
}

/****************************************************************************
 * Name: vi_scrolldown
 *
 * Description:
 *   Scroll the display down 'nlines' by sending the VT100 REVINDEX command.
 *
 ****************************************************************************/

static void vi_scrolldown(FAR struct vi_s *vi, uint16_t nlines)
{
  viinfo("nlines=%d\n", nlines);

  /* Ensure the bottom line is cleared after the scroll */

  vi_setcursor(vi, vi->display.row - 2, 0);
  vi_clrtoeol(vi);

  /* Scroll for the specified number of lines */

  for (; nlines; nlines--)
    {
      /* Send the VT100 REVINDEX command */

      vi_write(vi, g_revindex, sizeof(g_revindex));
    }
}

/****************************************************************************
 * Name: vi_printf
 *
 * Description:
 *   Show a highlighted message at the final line of the display.
 *
 ****************************************************************************/

static void vi_printf(FAR struct vi_s *vi, FAR const char *prefix,
                      FAR const char *fmt, ...)
{
  struct vi_pos_s cursor;
  va_list ap;
  int len;

  /* Save the current cursor position */

  cursor.row    = vi->cursor.row;
  cursor.column = vi->cursor.column;

  /* Set up for a reverse text message on the final line */

  vi_setcursor(vi, vi->display.row - 1, 0);
  vi_reverseon(vi);

  /* Expand the prefix message in the scratch buffer */

  len = prefix ? snprintf(vi->scratch, SCRATCH_BUFSIZE, prefix) : 0;

  va_start(ap, fmt);
  len += vsnprintf(vi->scratch + len, SCRATCH_BUFSIZE - len, fmt, ap);
  vvidbg(fmt, ap);
  va_end(ap);

  /* Write the error message to the display in reverse text */

  vi_write(vi, vi->scratch, len);

  /* Restore normal attributes */

  vi_attriboff(vi);

  /* Reposition the cursor */

  vi_setcursor(vi, cursor.row, cursor.column);

  /* Remember that there is an error message on the last line of the display.
   * When the display is refreshed, the last line will not be altered until
   * the error is cleared.
   */

  vi->error = true;
  VI_BEL(vi);
}

/****************************************************************************
 * Line positioning
 ****************************************************************************/

/****************************************************************************
 * Name: vi_linebegin
 *
 * Description:
 *   Search backward for the beginning of the current line
 *
 ****************************************************************************/

static off_t vi_linebegin(FAR struct vi_s *vi, off_t pos)
{
  /* Search backward to find the previous newline character (or, possibly,
   * the beginning of the text buffer).
   */

  while (pos && vi->text[pos - 1] != '\n')
    {
      pos--;
    }

  viinfo("Return pos=%ld\n", (long)pos);
  return pos;
}

/****************************************************************************
 * Name: vi_prevline
 *
 * Description:
 *   Search backward for the beginning of the previous line
 *
 ****************************************************************************/

static off_t vi_prevline(FAR struct vi_s *vi, off_t pos)
{
  /* Find the beginning the of current line */

  pos = vi_linebegin(vi, pos);

  /* If this not the first line, then back up one more character to position
   * at the last byte of the previous line.
   */

  if (pos > 0)
    {
      pos = vi_linebegin(vi, pos - 1);
    }

  viinfo("Return pos=%ld\n", (long)pos);
  return pos;
}

/****************************************************************************
 * Name: vi_lineend
 *
 * Description:
 *   Search forward for the end of the current line
 *
 ****************************************************************************/

static off_t vi_lineend(FAR struct vi_s *vi, off_t pos)
{
  /* Search forward to find the next newline character. (or, possibly,
   * the end of the text buffer).
   */

  while (pos < vi->textsize && vi->text[pos] != '\n')
    {
      pos++;
    }

  if (vi->text[pos] == '\n')
    {
      pos--;
    }

  viinfo("Return pos=%ld\n", (long)pos);
  return pos;
}

/****************************************************************************
 * Name: vi_nextline
 *
 * Description:
 *   Search backward for the start of the next line
 *
 ****************************************************************************/

static off_t vi_nextline(FAR struct vi_s *vi, off_t pos)
{
  /* Position at the end of the current line */

  pos = vi_lineend(vi, pos) + 1;

  /* If this is not the last byte in the buffer, then increment by one
   * for position of the first byte of the next line.
   */

  if (pos < vi->textsize)
    {
      pos++;
    }

  viinfo("Return pos=%ld\n", (long)pos);
  return pos;
}

/****************************************************************************
 * Text buffer management
 ****************************************************************************/

/****************************************************************************
 * Name: vi_extendtext
 *
 * Description:
 *   Reallocate the in-memory file memory by (at least) 'increment' and make
 *   space for new text of size 'increment' at the specified cursor position.
 *
 ****************************************************************************/

static bool vi_extendtext(FAR struct vi_s *vi, off_t pos, size_t increment)
{
  FAR char *alloc;
  int i;

  viinfo("pos=%ld increment=%ld\n", (long)pos, (long)increment);

  /* Check if we need to reallocate */

  if (!vi->text || vi->textsize + increment > vi->txtalloc)
    {
      /* Allocate in chunksize so that we do not have to reallocate so
       * often.
       */

      size_t allocsize = ALIGN_GULP(vi->textsize + increment);
      alloc = realloc(vi->text, allocsize);
      if (alloc == NULL)
        {
          /* Reallocation failed */

          vi_error(vi, g_fmtallocfail);
          return false;
        }

      /* Save the new buffer information */

      vi->text    = alloc;
      vi->txtalloc = allocsize;
    }

  /* Move text to make space for new text of size 'increment' at the current
   * cursor position
   */

  for (i = vi->textsize - 1; i >= pos; i--)
    {
      vi->text[i + increment] = vi->text[i];
    }

  /* Adjust end of file position */

  vi->textsize += increment;
  vi->modified  = true;
  return true;
}

/****************************************************************************
 * Name: vi_shrinkpos
 *
 * Description:
 *   This is really part of vi_shrinktext.  When any text is deleted, any
 *   positions lying beyond the deleted region in the text buffer must be
 *   adjusted.
 *
 * Input Parameters:
 *   delpos  The position where text was deleted
 *   delsize The number of bytes deleted.
 *   pos     A pointer to a position that may need to be adjusted.
 *
 ****************************************************************************/

static void vi_shrinkpos(FAR struct vi_s *vi, off_t delpos, size_t delsize,
                         FAR off_t *pos)
{
  viinfo("delpos=%ld delsize=%ld pos=%ld\n",
         (long)delpos, (long)delsize, (long)*pos);

  /* Check if the position is beyond the deleted region */

  if (*pos > delpos + delsize)
    {
      /* Yes... just subtract the size of the deleted region */

      *pos -= delsize;
    }

  /* What if the position is within the deleted region?  Set it to the
   * beginning of the deleted region.
   */

  else if (*pos > delpos)
    {
      *pos = delpos;
    }

  /* Ensure the position is within the text bounds in case the
   * text at the end of the buffer is being deleted
   */

  if (*pos >= vi->textsize && vi->mode != MODE_INSERT &&
       vi->mode != MODE_REPLACE)
    {
      *pos = vi->textsize - 1;
    }

  /* Check pos for negative bounds */

  if (*pos < 0)
    {
      *pos = 0;
    }
}

/****************************************************************************
 * Name: vi_shrinktext
 *
 * Description:
 *   Delete a region in the text buffer by copying the end of the text buffer
 *   over the deleted region and adjusting the size of the region.  The text
 *   region may be reallocated in order to recover the unused memory.
 *
 ****************************************************************************/

static void vi_shrinktext(FAR struct vi_s *vi, off_t pos, size_t size)
{
  FAR char *alloc;
  size_t allocsize;
  int i;

  viinfo("pos=%ld size=%ld\n", (long)pos, (long)size);

  /* Close up the gap to remove 'size' characters at 'pos' */

  for (i = pos + size; i < vi->textsize; i++)
    {
      vi->text[i - size] = vi->text[i];
    }

  /* Ensure we are not shrinking more than we have */

  if (size > vi->textsize)
    {
      size = vi->textsize;
    }

  /* Adjust sizes and positions */

  vi->textsize -= size;
  vi->modified  = true;
  vi_shrinkpos(vi, pos, size, &vi->curpos);
  vi_shrinkpos(vi, pos, size, &vi->winpos);
  vi_shrinkpos(vi, pos, size, &vi->prevpos);

  /* Reallocate the buffer to free up memory no longer in use */

  allocsize = ALIGN_GULP(vi->textsize);
  if (allocsize == 0)
    {
      allocsize = TEXT_GULP_SIZE;
    }

  if (allocsize < vi->txtalloc)
    {
      alloc = realloc(vi->text, allocsize);
      if (!alloc)
        {
          vi_error(vi, g_fmtallocfail);
          return;
        }

      /* Save the new buffer information */

      vi->text    = alloc;
      vi->txtalloc = allocsize;
    }
}

/****************************************************************************
 * File access
 ****************************************************************************/

/****************************************************************************
 * Name: vi_insertfile
 *
 * Description:
 *   Insert the contents of a file into the text buffer
 *
 ****************************************************************************/

static bool vi_insertfile(FAR struct vi_s *vi, off_t pos,
                          FAR const char *filename)
{
  struct stat buf;
  FILE *stream;
  off_t filesize;
  size_t nread;
  int result;
  bool ret;

  viinfo("pos=%ld filename=\"%s\"\n", (long)pos, filename);

  /* Get the size of the file  */

  result = stat(filename, &buf);
  if (result < 0)
    {
      vi_message(vi, "\"%s\" [New File]", filename);
      return false;
    }

  /* Check for zero-length file */

  filesize = buf.st_size;
  if (filesize < 1)
    {
      return false;
    }

  /* Open the file for reading */

  stream = fopen(filename, "r");
  if (!stream)
    {
      vi_error(vi, g_fmtcmdfail, "open", errno);
      return false;
    }

  /* [Re]allocate the text buffer to hold the file contents at the current
   * cursor position.
   */

  ret = false;
  if (vi_extendtext(vi, pos, filesize))
    {
      /* Read the contents of the file into the text buffer at the
       * current cursor position.
       */

      nread = fread(vi->text + pos, 1, filesize, stream);
      if (nread < filesize)
        {
          /* Report the error (or partial read), EINTR is not handled */

          vi_error(vi, g_fmtcmdfail, "fread", errno);
          vi_shrinktext(vi, pos, filesize);
        }
      else
        {
          ret = true;
        }
    }

  vi->fullredraw = true;
  fclose(stream);
  return ret;
}

/****************************************************************************
 * Name: vi_savetext
 *
 * Description:
 *   Save a region of the text buffer to 'filename'
 *
 ****************************************************************************/

static bool vi_savetext(FAR struct vi_s *vi, FAR const char *filename,
                        off_t pos, size_t size)
{
  FAR FILE *stream;
  size_t nwritten;
  int len;

  viinfo("filename=\"%s\" pos=%ld size=%ld\n",
         filename, (long)pos, (long)size);

  /* Open the file for writing */

  stream = fopen(filename, "w");
  if (!stream)
    {
      vi_error(vi, g_fmtcmdfail, "fopen", errno);
      return false;
    }

  /* Write the region of the text buffer beginning at pos and extending
   * through pos + size -1.
   */

  nwritten = fwrite(vi->text + pos, 1, size, stream);
  if (nwritten < size)
    {
      /* Report the error (or partial write).  EINTR is not handled. */

      vi_error(vi, g_fmtcmdfail, "fwrite", errno);
      fclose(stream);
      return false;
    }

  fclose(stream);

  len = sprintf(vi->scratch, "%dC written", nwritten);
  vi_write(vi, vi->scratch, len);
  return true;
}

/****************************************************************************
 * Name: vi_checkfile
 *
 * Description:
 *   Check if a file by this name already exists.
 *
 ****************************************************************************/

static bool vi_checkfile(FAR struct vi_s *vi, FAR const char *filename)
{
  struct stat buf;
  int ret;

  viinfo("filename=\"%s\"\n", filename);

  /* Get the size of the file  */

  ret = stat(filename, &buf);
  if (ret < 0)
    {
      /* The file does not exist */

      return false;
    }

  /* It exists, but is it a regular file */

  if (!S_ISREG(buf.st_mode))
    {
      /* Report the error... there is really no good return value in
       * this case.
       */

      vi_error(vi, g_fmtnotfile, filename);
    }

  return true;
}

/****************************************************************************
 * Mode Management Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_setmode
 *
 * Description:
 *   Set the new mode (or command sub-mode) and reset all other common state
 *   variables.  NOTE that a numeric value may be passed to the new mode in
 *   the value field.
 *
 ****************************************************************************/

static void vi_setmode(FAR struct vi_s *vi, uint8_t mode, long value)
{
  viinfo("mode=%d value=%ld\n", mode, value);

  /* Set the mode and clear mode-dependent states that are not preserved
   * across mode changes.
   */

  vi->mode           = mode;
  vi->delarm         = false;
  vi->yankarm        = false;
  vi->toparm         = false;
  vi->chgarm         = false;
  vi->wqarm          = false;
  vi->value          = value;
  vi->cmdlen         = 0;
}

/****************************************************************************
 * Name: vi_setsubmode
 *
 * Description:
 *   Set up one of the data entry sub-modes of the command mode.  These are
 *   modes in which commands or search data will be entered on the final line
 *   of the display.
 *
 ****************************************************************************/

static void vi_setsubmode(FAR struct vi_s *vi, uint8_t mode, char prompt,
                          long value)
{
  viinfo("mode=%d prompt='%c' value=%ld\n", mode, prompt, value);

  /* Set up the new mode */

  vi_setmode(vi, mode, value);

  /* Save the previous cursor position (not required by all modes) */

  vi->cursave.row    = vi->cursor.row;
  vi->cursave.column = vi->cursor.column;

  /* Set up for data entry on the final line */

  vi->cursor.row     = vi->display.row - 1;
  vi->cursor.column  = 0;
  vi_setcursor(vi, vi->cursor.row, vi->cursor.column);

  /* Output the prompt character in bold text */

  vi_boldon(vi);
  vi_putch(vi, prompt);
  vi_attriboff(vi);

  /* Clear to the end of the line */

  vi_clrtoeol(vi);

  /* Update the cursor position */

  vi->cursor.column  = 1;
}

/****************************************************************************
 * Name:  vi_exitsubmode
 *
 * Description:
 *   Exit the data entry sub-mode and return to normal command mode.
 *
 ****************************************************************************/

static void vi_exitsubmode(FAR struct vi_s *vi, uint8_t mode)
{
  viinfo("mode=%d\n", mode);

  /* Set up the new mode */

  vi_setmode(vi, mode, 0);

  /* Restore the saved cursor position */

  vi->cursor.row    = vi->cursave.row;
  vi->cursor.column = vi->cursave.column;
}

/****************************************************************************
 * Display Management
 ****************************************************************************/

/****************************************************************************
 * Name: vi_windowpos
 *
 * Description:
 *   Based on the position of the cursor in the text buffer, determine the
 *   horizontal display cursor position, performing TAB expansion as
 *   necessary.
 *
 ****************************************************************************/

static void vi_windowpos(FAR struct vi_s *vi, off_t start, off_t end,
                         uint16_t *pcolumn, off_t *ppos)
{
  uint16_t column;
  off_t pos;

  viinfo("start=%ld end=%ld\n", (long)start, (long)end);

  /* Make sure that the end position is not beyond the end of the text. We
   * assume that the start position is okay.
   */

  if (end > vi->textsize)
    {
      end = vi->textsize;
    }

  /* Loop incrementing the text buffer position while text buffer position
   * is within range.
   */

  for (pos = start, column = 0; pos < end && (column < vi->reqcolumn ||
        vi->updatereqcol); pos++)
    {
      /* Is there a newline terminator at this position? */

      if (vi->text[pos] == '\n')
        {
          /* Yes... break out of the loop return the cursor column */

          if (vi->mode != MODE_INSERT && vi->mode != MODE_REPLACE &&
              pos != start)
            {
              pos--;
              column--;
            }

          break;
        }

      /* No... Is there a TAB at this position? */

      else if (vi->text[pos] == '\t')
        {
          /* Yes.. expand the TAB */

          column = NEXT_TAB(column);
        }

      /* No, then just increment the cursor column by one character */

      else
        {
          column++;
        }
    }

  /* Keep cursor in bounds of text (i.e. not at the '\n') */

  if (((pos == vi->textsize && column != 0) ||
       (vi->text[pos] == '\n' && pos != start)) &&
        vi->mode != MODE_INSERT && vi->mode != MODE_REPLACE)
    {
      pos--;
      column--;
    }

  /* Now return the requested values */

  if (ppos)
    {
      *ppos = pos;
    }

  if (pcolumn)
    {
      *pcolumn = column;
    }

  if (vi->updatereqcol)
    {
      vi->reqcolumn = column;
      vi->updatereqcol = false;
    }
}

/****************************************************************************
 * Name: vi_scrollcheck
 *
 * Description:
 *   Check if any operations will require that we scroll the display.
 *
 ****************************************************************************/

static void vi_scrollcheck(FAR struct vi_s *vi)
{
  off_t curline;
  off_t pos;
  uint16_t tmp;
  int column;
  int nlines;

  /* Sanity test */

  if (vi->curpos > vi->textsize)
    {
      vi->curpos = vi->textsize;
    }

  /* Get the text buffer offset to the beginning of the current line */

  curline = vi_linebegin(vi, vi->curpos);

  /* Check if the current line is above the first line on the display */

  while (curline < vi->winpos)
    {
      /* Yes.. move the window position up to the beginning of the previous
       * line line and check again
       */

      vi->winpos = vi_prevline(vi, vi->winpos);
      vi->vscroll--;
      vi->fullredraw = true;
    }

  /* Reset the cursor row position so that it is relative to the
   * top of the display.
   */

  vi->cursor.row = 0;
  for (pos = vi->winpos; pos < curline; pos = vi_nextline(vi, pos))
    {
      vi->cursor.row++;
    }

  /* Check if the cursor row position is below the bottom of the display */

  for (; vi->cursor.row >= vi->display.row - 1; vi->cursor.row--)
    {
      /* Yes.. move the window position down by one line and check again */

      vi->winpos = vi_nextline(vi, vi->winpos);
      vi->vscroll++;
      vi->fullredraw = true;
    }

  /* Check if the cursor column is on the display.  vi_windowpos returns the
   * unrestricted column number of cursor.  hscroll is the horizontal offset
   * in characters.
   */

  vi_windowpos(vi, curline, vi->curpos, &tmp, NULL);
  column = (int)tmp - (int)vi->hscroll;

  /* Force the cursor column to lie on the display.  First check if the
   * column lies to the left of the horizontal scrolling position.  If it
   * does, move the scroll position to the left by tabs until the cursor
   * lies on the display.
   */

  while (column < 0)
    {
      column      += VI_TABSIZE;
      vi->hscroll -= VI_TABSIZE;
      vi->fullredraw = true;
    }

  /* If the cursor column lies to the right of the display, then adjust
   * the horizontal scrolling position so that the cursor position does
   * lie on the display.
   */

  while (column >= vi->display.column)
    {
      column      -= VI_TABSIZE;
      vi->hscroll += VI_TABSIZE;
      vi->fullredraw = true;
    }

  /* That final adjusted position is the display cursor column */

  vi->cursor.column = column;

  /* Check if new window position is below the previous position.
   * In this case, we will need to scroll up until the new window
   * position is at the top of the display.
   */

  if (vi->winpos > vi->prevpos)
    {
      /* We will need to scroll up.  Count how many lines we
       * need to scroll.
       */

      for (nlines = 0, pos = vi->prevpos;
           pos != vi->winpos && nlines < vi->display.row - 1;
           nlines++)
        {
          pos = vi_nextline(vi, pos);
        }

      /* Then scroll up that number of lines */

      if (nlines < vi->display.row - 1)
        {
          vi_scrollup(vi, nlines);
          vi->fullredraw = true;
        }
    }

  /* Check if new window position is above the previous position.
   * In this case, we will need to scroll down until the new window
   * position is at the top of the display.
   */

  else if (vi->winpos < vi->prevpos)
    {
      for (nlines = 0, pos = vi->prevpos;
           pos != vi->winpos && nlines < vi->display.row - 1;
           nlines++)
        {
          pos = vi_prevline(vi, pos);
        }

      /* Then scroll down that number of lines */

      if (nlines < vi->display.row - 1)
        {
          vi_scrolldown(vi, nlines);
          vi->fullredraw = true;
        }
    }

  /* Save the previous top-of-display position for next time around.
   * This can be modified asynchronously by text deletion operations.
   */

  vi->prevpos = vi->winpos;
  viinfo("winpos=%ld hscroll=%d\n",
         (long)vi->winpos, (long)vi->hscroll);
}

/****************************************************************************
 * Name: vi_showtext
 *
 * Description:
 *   Update the display based on the last operation.  This function is
 *   called at the beginning of the processing loop in Command and Insert
 *   modes (and also in the continuous replace mode).
 *
 ****************************************************************************/

static void vi_showtext(FAR struct vi_s *vi)
{
  off_t pos;
  off_t writefrom;
  uint16_t row;
  uint16_t endrow;
  uint16_t column;
  uint16_t endcol;
  uint16_t tabcol;
  bool redraw_line;

  /* Check if any of the preceding operations will cause the display to
   * scroll.
   */

  vi_scrollcheck(vi);

  /* If no display updates needed after scrollcheck, just return */

  if (!vi->fullredraw && !vi->drawtoeos && !vi->redrawline)
    {
      return;
    }

  /* If there is an error message at the bottom of the display, then
   * do not update the last line.
   */

  endrow = vi->display.row - 1;

  /* Make sure that all character attributes are disabled; Turn off the
   * cursor during the update.
   */

  vi_attriboff(vi);
  vi_cursoroff(vi);

  /* Set loop control variables based on draw mode */

  if (vi->fullredraw)
    {
      /* Start from beginning of display */

      pos = vi->winpos;
      row = 0;

      /* Ensure drawtoeos and redraw line are also set */

      vi->drawtoeos  = true;
      vi->redrawline = true;
    }
  else
    {
      /* Start drawing from current row */

      pos = vi_linebegin(vi, vi->curpos);
      row = vi->cursor.row;

      if (vi->drawtoeos)
        {
          vi->redrawline = true;
        }
      else
        {
          endrow = row + 1;
        }
    }

  /* Write each line to the display, handling horizontal scrolling and
   * tab expansion.
   */

  for (; pos < vi->textsize && row < endrow; row++)
    {
      /* Test if this line needs to be redrawn */

      redraw_line = true;
      if (!vi->redrawline)
        {
          redraw_line = false;
        }
      else if (row + 1 < vi->cursor.row && !vi->fullredraw)
        {
          redraw_line = false;
        }
      else if (row > vi->cursor.row && !vi->drawtoeos)
        {
          redraw_line = false;
        }
      else if (row == vi->cursor.row && !vi->redrawline)
        {
          redraw_line = false;
        }

      /* Get the last column on this row.  Avoid writing into the last byte
       * on the screen which may trigger a scroll.
       */

      endcol = vi->display.column;
      if (row >= vi->display.row - 1)
        {
         endcol--;
        }

      /* Get the position into this line corresponding to display column 0,
       * accounting for horizontal scrolling and tab expansion.  Add that to
       * the line start offset to get the first offset to consider for
       * display.
       */

      vi_windowpos(vi, pos, pos + vi->hscroll, NULL, &pos);

      /* Set the cursor position to the beginning of this row and clear to
       * the end of the line.
       */

      if (redraw_line)
        {
          vi_setcursor(vi, row, 0);

          /* Loop for each column */

          writefrom = pos;
          for (column = 0; pos < vi->textsize && column < endcol; pos++)
            {
              /* Break out of the loop if we encounter the newline before the
               * last column is encountered.
               */

              if (vi->text[pos] == '\n')
                {
                  break;
                }

              /* Perform TAB expansion */

              else if (vi->text[pos] == '\t')
                {
                  /* Write collected characters */

                  if (writefrom != pos)
                    {
                      vi_write(vi, &vi->text[writefrom], pos - writefrom);
                    }

                  tabcol = NEXT_TAB(column);
                  if (tabcol < endcol)
                    {
                      for (; column < tabcol; column++)
                        {
                          vi_putch(vi, ' ');
                        }

                      writefrom = pos + 1;
                    }
                  else
                    {
                      /* Break out of the loop... there is nothing left on
                       * the line but whitespace.
                       */

                      writefrom = pos;
                      break;
                    }
                }

              /* Add the normal character to the display */

              else
                {
                  column++;
                }
            }

          /* Write collected characters */

          if (writefrom != pos)
            {
              vi_write(vi, &vi->text[writefrom], pos - writefrom);
            }

          vi_clrtoeol(vi);
        }

      /* Skip to the beginning of the next line */

      pos = vi_nextline(vi, pos);
    }

  if (pos == vi->textsize && vi->text[pos - 1] == '\n')
    {
      vi_setcursor(vi, row, 0);
      vi_clrtoeol(vi);
      row++;
    }

  /* If we are drawing to EOS, then draw trailing '~' */

  if (vi->drawtoeos)
    {
      /* If there was not enough text to fill the display, clear the
       * remaining lines (except for any possible error line at the
       * bottom of the display).
       */

      for (; row < endrow; row++)
        {
          /* Set the cursor position to the beginning of the row and clear to
           * the end of the line.
           */

          vi_setcursor(vi, row, 0);
          if (row != endrow && row != 0)
            {
              vi_putch(vi, '~');
            }

          vi_clrtoeol(vi);
        }
    }

  /* Turn the cursor back on */

  vi_cursoron(vi);
  vi->fullredraw = false;
  vi->drawtoeos = false;
  vi->redrawline = false;
}

/****************************************************************************
 * Name: vi_showlinecol
 *
 * Description:
 *   Update the current line/column on the display status line.
 *
 ****************************************************************************/

static void vi_showlinecol(FAR struct vi_s *vi)
{
  size_t len;

  /* Move to bototm line for display */

  vi_cursoroff(vi);
  vi_setcursor(vi, vi->display.row - 1, vi->display.column - 15);

  len = snprintf(vi->scratch, SCRATCH_BUFSIZE, "%d,%d",
                 vi->cursor.row + vi->vscroll + 1,
                 vi->cursor.column + vi->hscroll + 1);
  vi_write(vi, vi->scratch, len);

  vi_clrtoeol(vi);
  vi_cursoron(vi);
}

/****************************************************************************
 * Command Mode Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_cusorup
 *
 * Description:
 *   Move the cursor up one line in the text buffer.
 *
 ****************************************************************************/

static void vi_cusorup(FAR struct vi_s *vi, int nlines)
{
  int remaining;
  off_t start;
  off_t end;

  viinfo("nlines=%d\n", nlines);

  /* How many lines do we need to move?  Zero means 1 (so does 1) */

  remaining = (nlines < 1 ? 1 : nlines);

  /* Get the offset to the start of the current line */

  start = vi_linebegin(vi, vi->curpos);

  /* Now move the cursor position back the correct number of lines */

  for (; remaining > 0; remaining--)
    {
      /* Get the start position of the previous line */

      start = vi_prevline(vi, start);

      /* Find the cursor position on the next line corresponding to the
       * cursor position on the current line.
       */

      end = start + vi->reqcolumn;
      vi_windowpos(vi, start, end, NULL, &vi->curpos);
    }
}

/****************************************************************************
 * Name: vi_cursordown
 *
 * Description:
 *   Move the cursor down one line in the text buffer
 *
 ****************************************************************************/

static void vi_cursordown(FAR struct vi_s *vi, int nlines)
{
  int remaining;
  off_t start;
  off_t end;

  viinfo("nlines=%d\n", nlines);

  /* How many lines do we need to move?  Zero means 1 (so does 1) */

  remaining = (nlines < 1 ? 1 : nlines);

  /* Get the offset to the start of the current line */

  start = vi_linebegin(vi, vi->curpos);

  /* Now move the cursor position forward the correct number of lines */

  for (; remaining > 0; remaining--)
    {
      /* Get the start of the next line. */

      start = vi_nextline(vi, start);

      /* Find the cursor position on the next line corresponding to the
       * cursor position on the current line.
       */

      end = start + vi->reqcolumn;
      vi_windowpos(vi, start, end, NULL, &vi->curpos);
    }
}

/****************************************************************************
 * Name: vi_cursorleft
 *
 * Description:
 *   Move the cursor left 'ncolumns' columns in the text buffer (without
 *   moving to the preceding line).  Note that a repetition count of 0 means
 *  to  perform the movement once.
 *
 ****************************************************************************/

static off_t vi_cursorleft(FAR struct vi_s *vi, off_t curpos, int ncolumns)
{
  int remaining;

  viinfo("curpos=%ld ncolumns=%d\n", curpos, ncolumns);

  /* Loop decrementing the cursor position for each repetition count.  Break
   * out early if we hit either the beginning of the text buffer, or the end
   * of the previous line.
   */

  for (remaining = (ncolumns < 1 ? 1 : ncolumns);
       curpos > 0 && remaining > 0 && vi->text[curpos - 1] != '\n';
       curpos--, remaining--)
    {
    }

  return curpos;
}

/****************************************************************************
 * Name: vi_cursorright
 *
 * Description:
 *   Move the cursor right 'ncolumns' columns in the text buffer (without
 *   moving to the next line).  Note that a repetition count of 0 means to
 *   perform the movement once.
 *
 ****************************************************************************/

static off_t vi_cursorright(FAR struct vi_s *vi, off_t curpos, int ncolumns)
{
  int remaining;

  viinfo("curpos=%ld ncolumns=%d\n", curpos, ncolumns);

  /* Loop incrementing the cursor position for each repetition count.  Break
   * out early if we hit either the end of the text buffer, or the end of
   * the line.
   */

  for (remaining = (ncolumns < 1 ? 1 : ncolumns);
       curpos < vi->textsize && remaining > 0 && vi->text[curpos] != '\n';
       curpos++, remaining--)
    {
    }

#if 0
  if (vi->text[curpos] == '\n' || (curpos == vi->textsize &&
      vi->mode != MODE_INSERT && vi->mode != MODE_REPLACE))
    {
      curpos--;
    }
#endif

  return curpos;
}

/****************************************************************************
 * Name: vi_gotoscreenbottom
 *
 * Description:
 *   Move the cursor to the bottom of the screen or the bottom line of
 *   the file if it doesn't occupy the entire screen.
 *
 ****************************************************************************/

static int vi_gotoscreenbottom(FAR struct vi_s *vi, int rows)
{
  off_t pos;
  int row;
  int target = rows > 0 ? rows >> 1 : vi->display.row - 2;

  vi->curpos = vi->winpos;
  row = 0;
  while (row < target)
    {
      /* Get position of next row down */

      pos = vi_nextline(vi, vi->curpos);

      /* Test for end of file before bottom of screen */

      if (pos == vi->curpos)
        {
          break;
        }

      if (pos == vi->textsize)
        {
          /* Test for empty line at the bottom */

          row++;
          vi->curpos = pos;
          break;
        }

      vi->curpos = pos;
      row++;
    }

  /* Report the location of the bottom row */

  return row;
}

/****************************************************************************
 * Name: vi_gotofirstnonwhite
 *
 * Description:
 *   Move the cursor to the first non-whitespace character on the
 *   current line.
 *
 ****************************************************************************/

static void vi_gotofirstnonwhite(FAR struct vi_s *vi)
{
  vi->curpos = vi_linebegin(vi, vi->curpos);
  while (vi->curpos <= vi->textsize && (vi->text[vi->curpos] == ' ' ||
         vi->text[vi->curpos] == '\t'))
    {
      vi->curpos++;
    }
}

/****************************************************************************
 * Name: vi_delforward
 *
 * Description:
 *   Delete characters from the current cursor position forward
 *
 ****************************************************************************/

static void vi_delforward(FAR struct vi_s *vi)
{
  off_t start;
  off_t end;
  bool at_end = false;

  viinfo("curpos=%ld value=%ld\n", (long)vi->curpos, vi->value);

  /* Test for empty file */

  if (vi->textsize == 0)
    {
      return;
    }

  /* Test for empty line deletion and simply return */

  if (vi->cursor.column == 0)
    {
      /* If at end of file, just return */

      if (vi->curpos == vi->textsize ||
          vi->text[vi->curpos] == '\n')
        {
          return;
        }
    }

  /* Get the cursor position as if we would have move the cursor right N
   * times (which might be <N characters).
   */

  end  = vi_cursorright(vi, vi->curpos, vi->value);

  /* Test for deletion at end of the line */

  if (end == vi->curpos && vi->cursor.column > 0)
    {
      end++;
      at_end = true;
    }

  /* The difference from the current position then is the number of
   * characters to be deleted.
   */

  start = vi->curpos;

  vi_yanktext(vi, start, end - 1, true, true);
  vi->curpos = start;
  if (at_end)
    {
      vi->curpos--;
    }

  vi->redrawline = true;
}

/****************************************************************************
 * Name: vi_delbackward
 *
 * Description:
 *   Delete characters before the current cursor position
 *
 ****************************************************************************/

static void vi_delbackward(FAR struct vi_s *vi)
{
  off_t start;
  off_t end;
  off_t x;

  viinfo("curpos=%ld value=%ld\n", (long)vi->curpos, vi->value);

  /* Test if we are at beginning of line */

  if (vi->curpos == 0 || vi->text[vi->curpos] == '\n' ||
      vi->text[vi->curpos - 1] == '\n')
    {
      return;
    }

  /* Back up one character.  This is where the deletion will end */

  end = vi_cursorleft(vi, vi->curpos, 1);

  /* Get the cursor position as if we would have move the cursor left N
   * times (which might be <N characters).
   */

  if (vi->value > 1)
    {
      start = vi_cursorleft(vi, end, vi->value -1);
    }
  else
    {
      start = end;
    }

  for (x = end; x >= start; x--)
    {
      /* Test if \n' in the range.  Don't delete through \n */

      if (vi->text[x] == '\n')
        {
          start = x + 1;
          break;
        }
    }

  /* The difference from the current position then is the number of
   * characters to be deleted.
   */

  vi_yanktext(vi, start, end, true, true);
  vi->redrawline = true;
}

/****************************************************************************
 * Name: vi_linerange
 *
 * Description:
 *   Return the start and end positions for N lines in the text buffer,
 *   beginning at the current line.  This is logic common to yanking and
 *   deleting lines.
 *
 ****************************************************************************/

static void vi_linerange(FAR struct vi_s *vi, off_t *start, off_t *end)
{
  off_t next;
  int nlines;

  /* Get the offset in the text buffer to the beginning of the current line */

  *start = vi_linebegin(vi, vi->curpos);

  /* Move one line unless a repetition count was provided */

  nlines = (vi->value > 0 ? vi->value : 1);

  /* Search ahead to find the end of the last line to yank */

  for (next = *start; nlines > 1; nlines--)
    {
      next = vi_nextline(vi, next);
    }

  *end = vi_lineend(vi, next);
  if (*end != vi->textsize)
    {
      (*end)++;
    }
}

/****************************************************************************
 * Name: vi_delline
 *
 * Description:
 *   Delete N lines from the text buffer, beginning at the current line.
 *
 ****************************************************************************/

static void vi_delline(FAR struct vi_s *vi)
{
  /* Yank and remove text from the buffer */

  vi_yank(vi, true);
  vi->drawtoeos = true;
}

/****************************************************************************
 * Name: vi_deltoeol
 *
 * Description:
 *   Delete to end of line.
 *
 ****************************************************************************/

static void vi_deltoeol(FAR struct vi_s *vi)
{
  int start;
  int end;

  /* If we are at the end of the line, then return */

  if (vi->curpos == vi->textsize || vi->text[vi->curpos] == '\n')
    {
      return;
    }

  /* Determine start and end location */

  start = vi->curpos;
  end   = vi_lineend(vi, vi->curpos);
  if (end == vi->textsize || vi->text[end] == '\n')
    {
      end--;
    }

  /* Yank and remove text from the buffer */

  vi_yanktext(vi, start, end, true, true);
  if (start > 0 && start != vi->textsize && vi->text[start - 1] != '\n')
    {
      vi->curpos = start - 1;
    }
  else
    {
      vi->curpos = start;
    }

  vi->redrawline = true;
}

/****************************************************************************
 * Name: vi_yanktext
 *
 * Description:
 *   Yank specified text from the text buffer and delete if requested.
 *
 ****************************************************************************/

static void vi_yanktext(FAR struct vi_s *vi, off_t start, off_t end,
                        bool yankcharmode, bool del_after_yank)
{
  int append_lf = 0;
  size_t alloc;
  size_t size;

  /* At end of file, in line yank mode, if there is no LF, we append one */

  if (vi->text[end] != '\n' && !yankcharmode)
    {
      append_lf = 1;
    }

  /* Allocate a yank buffer big enough to hold the lines */

  size  = end - start + 1;
  alloc = size + append_lf;

  if (alloc < CONFIG_SYSTEM_VI_YANK_THRESHOLD)
    {
      alloc = CONFIG_SYSTEM_VI_YANK_THRESHOLD;
    }

  /* Free any previously yanked lines */

  if (vi->yank)
    {
      /* Free the buffer only if it is too small or if it is larger
       * than the YANK_THRESHOLD and we need less than that.
       */

      if (alloc > vi->yankalloc ||
          (alloc == CONFIG_SYSTEM_VI_YANK_THRESHOLD &&
           vi->yankalloc > CONFIG_SYSTEM_VI_YANK_THRESHOLD))
        {
          free(vi->yank);
          vi->yank = NULL;
        }
    }

  /* Allocate buffer if not already allocated */

  if (!vi->yank)
    {
      vi->yankalloc    = alloc;
      vi->yank         = (FAR char *)malloc(vi->yankalloc);
    }

  vi->yankcharmode = yankcharmode;

  if (!vi->yank)
    {
      vi_error(vi, g_fmtallocfail);
      vi->yankalloc = 0;
      vi->yanksize = 0;
      return;
    }

  /* Copy the block from the text buffer to the yank buffer */

  vi->yanksize = size;
  memcpy(vi->yank, &vi->text[start], size);

  /* Append \n if needed */

  if (append_lf > 0)
    {
      vi->yank[vi->yanksize] = '\n';
    }

  /* Remove the yanked text from the text buffer */

  if (del_after_yank)
    {
      vi_shrinktext(vi, start, vi->yanksize);
    }

  /* Account for appended lf in yankalloc */

  vi->yanksize += append_lf;
}

/****************************************************************************
 * Name: vi_yank
 *
 * Description:
 *   Remove N lines from the text buffer, beginning at the current line and
 *   copy the lines to an allocated yank buffer.
 *
 ****************************************************************************/

static void vi_yank(FAR struct vi_s *vi, bool del_after_yank)
{
  off_t start;
  off_t end;
  off_t yank_end;
  off_t textsize;
  int pos_increment = 0;
  bool empty_last_line = false;

  /* Get the offset in the text buffer corresponding to the range of lines to
   * be yanked
   */

  vi_linerange(vi, &start, &end);
  textsize = vi->textsize;

  /* Do end of file bounds checking */

  if (end >= textsize)
    {
      end = textsize - 1;
    }

  if (start >= textsize)
    {
      start = textsize -1;
    }

  /* When yanking last line with \n, don't delete the \n */

  yank_end = end;
  if (del_after_yank && end == textsize - 1 && start != end &&
      vi->text[end] == '\n')
    {
      yank_end--;
      pos_increment = 1;
    }

  viinfo("start=%ld end=%ld\n", (long)start, (long)end);

  /* Test if deleting last line with empty line above it */

  if ((end > 0 && start == end && end == vi->textsize -1 &&
      vi->text[end - 1] == '\n') || (start > 1 && end + 1 ==
      vi->textsize && vi->text[start - 2] == '\n'))
    {
      empty_last_line = true;
    }

  vi_yanktext(vi, start, yank_end, 0, del_after_yank);

  /* If the last line was yanked, then remove the '\n' on the
   * previous line.
   */

  if (end + 1 == textsize && start != end && del_after_yank)
    {
      vi_shrinktext(vi, vi->textsize - 1, 1);
    }

  /* Place cursor at beginning of the line */

  if (del_after_yank)
    {
      if (empty_last_line)
        {
          vi->curpos = vi->textsize;
        }
      else
        {
          vi->curpos = vi_linebegin(vi, vi->curpos + pos_increment);
        }
    }
}

/****************************************************************************
 * Name: vi_paste
 *
 * Description:
 *   Copy line(s) from the yank buffer, and past them after the current line.
 *   The contents of the yank buffer are released.
 *
 ****************************************************************************/

static void vi_paste(FAR struct vi_s *vi, bool paste_before)
{
  off_t start;
  off_t new_curpos;
  int count;

  viinfo("curpos=%ld yankalloc=%d\n", (long)vi->curpos, (long)vi->yankalloc);

  /* Make sure there is something to be pasted */

  if (!vi->yank || vi->yanksize <= 0)
    {
      return;
    }

  /* Get the command count */

  count = vi->value > 0 ? vi->value : 1;
  if (count > 1)
    {
      vi->fullredraw = true;
    }

  /* Test for char mode paste buffer */

  while (count > 0)
    {
      if (vi->yankcharmode)
        {
          off_t pos;

          /* Paste at next col to the right of cursor */

          if (vi->text[vi->curpos] == '\n' || vi->curpos == vi->textsize ||
              paste_before)
            {
              pos = vi->curpos;
            }
          else
            {
              pos = vi->curpos + 1;
            }

          if (vi_extendtext(vi, pos, vi->yanksize))
            {
              /* Copy the contents of the yank buffer into the text buffer
               * at the position where the start of the next line was.
               */

              memcpy(&vi->text[pos], vi->yank, vi->yanksize);

              /* Advance the cursor */

              vi->curpos = vi->curpos + vi->yanksize;
              if (vi->curpos > vi->textsize || vi->text[vi->curpos] == '\n')
                {
                  vi->curpos--;
                }
            }
        }
      else
        {
          off_t   size;

          /* Paste at the beginning of the next line */

          if (paste_before)
            {
              start = vi_linebegin(vi, vi->curpos);
            }
          else
            {
              start = vi_nextline(vi, vi->curpos);
            }

          size = vi->yanksize;

          /* Test if pasting at end of file */

          new_curpos = start;
          if ((start >= vi->textsize && vi->text[vi->textsize - 1] != '\n')
              || vi->curpos == vi->textsize)
            {
              off_t textsize = vi->textsize;
              bool at_end = vi->curpos == vi->textsize;

              vi->curpos = vi->textsize;
              vi_insertch(vi, '\n');
              start      = vi->textsize;
              new_curpos = start;

              /* Don't append the \n' in the yank buffer */

              if (vi->text[textsize - 1] != '\n' || at_end)
                {
                  size--;
                }
            }

          /* Ensure start <= textsize */

          else if (start >= vi->textsize)
            {
              start          = vi->textsize;
              new_curpos     = start;
              vi->fullredraw = true;
            }

          /* Reallocate the text buffer to hold the yank buffer contents at
           * the beginning of the next line.
           */

          if (vi_extendtext(vi, start, size))
            {
              /* Copy the contents of the yank buffer into the text buffer
               * at the position where the start of the next line was.
               */

              memcpy(&vi->text[start], vi->yank, size);

              /* Advance to next line */

              vi->curpos = new_curpos;
            }
        }

      count--;
    }

  /* Redraw everything below this point */

  vi->drawtoeos = true;
}

/****************************************************************************
 * Name: vi_join
 *
 * Description:
 *   Join line below with current line.
 *
 ****************************************************************************/

static void vi_join(FAR struct vi_s *vi)
{
  off_t start;
  off_t end;

  /* Test if we are at end of file */

  if (vi->curpos + 1 >= vi->textsize)
    {
      return;
    }

  start = vi_lineend(vi, vi->curpos);

  /* Ensure the line ends with '\n' */

  if (vi->text[start + 1] != '\n')
    {
      return;
    }

  /* Convert the '\n' to a space */

  vi->text[++start] = ' ';
  end = start + 1;

  /* Skip all spaces and tabs on next line */

  while ((vi->text[end] == ' ' || vi->text[end] == '\t') &&
      end < vi->textsize)
    {
      end++;
    }

  if (start + 1 != end)
    {
      vi_shrinktext(vi, start + 1, end - (start + 1));
    }

  vi->curpos    = start;
  vi->drawtoeos = true;
}

/****************************************************************************
 * Name: vi_gotoline
 *
 * Description:
 *   Position the cursor at the line specified by vi->value.  If
 *   vi->value is zero, then the cursor is position at the end of the text
 *   buffer.
 *
 ****************************************************************************/

static void vi_gotoline(FAR struct vi_s *vi)
{
  viinfo("curpos=%ld value=%ld\n", (long)vi->curpos, vi->value);

  /* Special case the first line */

  if (vi->value == 1)
    {
      vi->curpos = 0;
    }

  /* Work harder to position to lines in the middle */

  else if (vi->value > 0)
    {
      uint32_t line;

      /* Got to the line == value */

      for (line = vi->value, vi->curpos = 0;
          --line > 0 && vi->curpos < vi->textsize;
          )
        {
          vi->curpos = vi_nextline(vi, vi->curpos);
        }
    }

  /* No value means to go to beginning of the last line */

  else
    {
      /* Get the beginning of the last line */

      vi->curpos = vi_linebegin(vi, vi->textsize);
    }

  vi->fullredraw = true;
}

/****************************************************************************
 * Name: vi_chartype
 *
 * Description:
 *   Determine and return the type of character (i.e. alpha, space,
 *   punctuation).
 *
 ****************************************************************************/

static int vi_chartype(char ch)
{
  int type;

  if (ch == ' ' || ch == '\t')
    {
      type = VI_CHAR_SPACE;
    }

  /* Test for alpha, numeric or '_' */

  else if (isalnum(ch) || ch == '_')
    {
      type = VI_CHAR_ALPHA;
    }

  /* Test for CR or NL */

  else if (ch == '\r' || ch == '\n')
    {
      type = VI_CHAR_CRLF;
    }

  /* Must be punctuation */

  else
    {
      type = VI_CHAR_PUNCT;
    }

  return type;
}

/****************************************************************************
 * Name: vi_findnextword
 *
 * Description:
 *   Find the position in the text buffer of the start of the next word.
 *
 ****************************************************************************/

static off_t vi_findnextword(FAR struct vi_s *vi)
{
  int srch_type;
  int pos_type;
  off_t pos;

  /* Get the type of character under the cursor so we know what the
   * next "word" looks like.
   */

  srch_type = vi_chartype(vi->text[vi->curpos]);
  pos = vi->curpos + 1;

  for (; pos < vi->textsize; pos++)
    {
      /* Get type of the next character */

      pos_type = vi_chartype(vi->text[pos]);

      /* Skip CR and NL */

      if (pos_type == VI_CHAR_CRLF)
        {
          /* Change to search type SPACE */

          srch_type = VI_CHAR_SPACE;
          continue;
        }

      /* We we were over a space, then any non-space is the next word */

      if (srch_type == VI_CHAR_SPACE &&
          pos_type != VI_CHAR_SPACE)
        {
          break;
        }

      /* Test for next punctuation if search type is alpha */

      if (srch_type == VI_CHAR_ALPHA &&
          pos_type == VI_CHAR_PUNCT)
        {
          break;
        }

      /* Test for next alpha if search type is punctuation */

      if (srch_type == VI_CHAR_PUNCT &&
          pos_type == VI_CHAR_ALPHA)
        {
          break;
        }

      /* Test for alpha search followed by space. Then switch the search type
       * to space so we find whatever is next.
       */

      if ((srch_type == VI_CHAR_ALPHA || srch_type == VI_CHAR_PUNCT) &&
          pos_type == VI_CHAR_SPACE)
        {
          srch_type = VI_CHAR_SPACE;
        }
    }

  /* Limit position to within the valid text range */

  if (pos == vi->textsize)
    {
      pos--;
    }

  /* Return the position of the next word */

  return pos;
}

/****************************************************************************
 * Name: vi_gotonextword
 *
 * Description:
 *   Position the cursor at the start of the next word.
 *
 ****************************************************************************/

static void vi_gotonextword(FAR struct vi_s *vi)
{
  int count;
  int x;
  off_t start = vi->curpos;
  off_t end;
  off_t pos;
  bool crfound;

  /* Loop for the specified search count */

  count = vi->value > 0 ? vi->value : 1;
  for (x = 0; x < count; x++ )
    {
      /* Get position of next word */

      vi->curpos = vi_findnextword(vi);
    }

  /* Test if yank,  delete or change are armed */

  if (vi->yankarm || vi->delarm || vi->chgarm)
    {
      /* Rewind so we don't yank skipped whitespace */

      pos     = vi->curpos;
      crfound = false;

      while ((vi->text[pos - 1] == ' ' || vi->text[pos - 1] == '\t' ||
             vi->text[pos - 1] == '\n') && pos > start)
        {
          /* We rewind only if '\n' found before non-space */

          pos--;
          if (vi->text[pos] == '\n')
            {
              crfound = true;
            }
        }

      if (crfound)
        {
          vi->curpos = pos;
        }

      /* If the yank / delete count is 1, then limit the yank/delete so it
       * doesn't contain any '\n' characters.
       */

      if (count == 1)
        {
          /* Scan the text range and look for '\n' */

          for (x = start; x < vi->curpos; x++)
            {
              /* Test for '\n' */

              if (vi->text[x] == '\n')
                {
                  /* Modify the yank / delete range */

                  vi->curpos = x;
                  break;
                }
            }
        }
      else
        {
          /* Multi-line delete? */

          vi->fullredraw = vi->delarm || vi->chgarm;
        }

      /* Perform the yank */

      end = vi->curpos + 1 == vi->textsize ? vi->curpos : vi->curpos - 1;
      if (vi->chgarm)
        {
          end--;
        }

      /* Yank text if it isn't a single \n character */

      if (!(start == end && vi->text[start] == '\n'))
        {
          vi_yanktext(vi, start, end, 1, vi->delarm | vi->chgarm);
        }

      if (vi->delarm | vi->chgarm)
        {
          /* Redraw line if text deleted */

          vi->redrawline = true;
        }

      /* Restore the original curpos */

      vi->curpos = start;

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
      /* Setup command repeat */

      if (vi->delarm | vi->chgarm)
        {
          vi_saverepeat(vi, vi->delarm ? 'd' : 'c');
          vi_appendrepeat(vi, 'w');
        }
#endif

      /* If change text is armed, then enter insert mode */

      if (vi->chgarm)
        {
          vi_setmode(vi, MODE_INSERT, 0);
        }

      vi->delarm = false;
      vi->chgarm = false;
      vi->yankarm = false;
    }
}

/****************************************************************************
 * Name: vi_findprevword
 *
 * Description:
 *   Find the position in the text buffer of the start of the previous word.
 *
 ****************************************************************************/

static off_t vi_findprevword(FAR struct vi_s *vi)
{
  int srch_type;
  int pos_type;
  off_t pos;

  /* If we are basically at the beginning of the file, then the task
   * is simple.
   */

  if (vi->curpos < 2)
    {
      return 0;
    }

  /* Get the type of character under the cursor so we know what the
   * next "word" looks like.
   */

  srch_type = vi_chartype(vi->text[vi->curpos]);
  pos       = vi->curpos - 1;
  pos_type  = vi_chartype(vi->text[pos]);

  /* Test if we are at the beginning of a word */

  if (srch_type == pos_type)
    {
      /* Not at beginning of word.  Find beginning of word. */

      while (pos > 0)
        {
          pos_type = vi_chartype(vi->text[pos - 1]);

          if (pos_type != srch_type && pos_type != VI_CHAR_CRLF)
            {
              break;
            }

          pos--;
        }

      if ((srch_type != VI_CHAR_SPACE && srch_type != VI_CHAR_CRLF) ||
          pos == 0)
        {
          return pos;
        }

      /* We found the start of a string of spaces.  Decrement to first
       * non-space character.
       */

      pos_type = vi_chartype(vi->text[--pos]);
    }

  /* If the previous char is space, then skip them */

  while ((pos_type == VI_CHAR_SPACE || pos_type == VI_CHAR_CRLF) && pos > 0)
    {
      pos_type = vi_chartype(vi->text[--pos]);
    }

  if (pos == 0)
    {
      return pos;
    }

  /* Now find beginning of this new type */

  srch_type = vi_chartype(vi->text[pos]);
  while (pos > 0 && vi_chartype(vi->text[pos - 1]) == srch_type)
    {
      pos--;
    }

  /* Return the position of the next word */

  return pos;
}

/****************************************************************************
 * Name: vi_gotoprevword
 *
 * Description:
 *   Position the cursor at the start of the previous word.
 *
 ****************************************************************************/

static void vi_gotoprevword(FAR struct vi_s *vi)
{
  int count;

  /* Loop for the specified search count */

  count = vi->value > 0 ? vi->value : 1;
  while (count > 0)
    {
      /* Get position of next word */

      vi->curpos = vi_findprevword(vi);
      count--;
    }
}

/****************************************************************************
 * Name: vi_bottom_line_debug
 *
 * Description:
 *   Print text and paste buffers on bottom line for debug purposes
 *
 ****************************************************************************/

#ifdef ENABLE_BOTTOM_LINE_DEBUG
static void vi_bottom_line_debug(FAR struct vi_s *vi)
{
  off_t pos;
  int column;

  vi_clearbottomline(vi);

  vi_putch(vi, '"');
  pos    = 0;
  column = 0;

  while (pos < vi->textsize && column < vi->display.column)
    {
      if (vi->text[pos] == '\n')
        {
          vi_putch(vi, '\\');
          vi_putch(vi, 'n');
        }
      else if (vi->text[pos] == '\t')
        {
          vi_putch(vi, '\\');
          vi_putch(vi, 'n');
        }
      else
        {
          vi_putch(vi, vi->text[pos]);
        }

      pos++;
      column++;
    }

  vi_putch(vi, '"');
  if (!vi->yank)
    {
      return;
    }

  vi_putch(vi, ' ');
  vi_putch(vi, '"');
  pos = 0;

  while (pos < vi->yanksize && column < vi->display.column)
    {
      if (vi->yank[pos] == '\n')
        {
          vi_putch(vi, '\\');
          vi_putch(vi, 'n');
        }
      else if (vi->yank[pos] == '\t')
        {
          vi_putch(vi, '\\');
          vi_putch(vi, 'n');
        }
      else
        {
          vi_putch(vi, vi->yank[pos]);
        }

      pos++;
    }

  vi_putch(vi, '"');
}
#endif /* ENABLE_BOTTOM_LINE_DEBUG */

/****************************************************************************
 * Name: vi_findnext
 *
 * Description:
 *   Perform find operation again in forward direction
 *
 ****************************************************************************/

void vi_findnext(FAR struct vi_s *vi)
{
  if (vi->curpos < vi->textsize)
    {
      vi->curpos++;
    }

  /* Search for string */

  if (!vi_findstring(vi))
    {
      /* Restore original pos if not found */

      vi->curpos--;
      VI_BEL(vi);
    }
}

/****************************************************************************
 * Name: vi_findprev
 *
 * Description:
 *   Perform find operation again in reverse direction
 *
 ****************************************************************************/

void vi_findprev(FAR struct vi_s *vi)
{
  off_t pos = vi->curpos;

  /* Move the cursor to the left */

  if (vi->curpos > 0)
    {
      vi->curpos--;
    }
  else
    {
      vi->curpos = vi->textsize - strlen(vi->findstr);
    }

  /* Perform the search */

  if (!vi_revfindstring(vi))
    {
      /* Restore the position if not found */

      vi->curpos = pos;
    }
}

/****************************************************************************
 * Name: vi_saverepeat
 *
 * Description:
 *   Save ch as the first command repeat entry.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
static void vi_saverepeat(FAR struct vi_s *vi, uint16_t ch)
{
  /* If we are in command repeat mode, then don't initialize */

  if (vi->cmdrepeat)
    {
      return;
    }

  vi->cmdcount = 0;
  vi->cmdindex = 0;

  if (ch < 256)
    {
      vi->cmdbuf[vi->cmdcount++] = ch;
      vi->repeatvalue = vi->value;
    }
}
#endif /* CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT */

/****************************************************************************
 * Name: vi_appendrepeat
 *
 * Description:
 *   Save ch as the next command repeat entry.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
static void vi_appendrepeat(FAR struct vi_s *vi, uint16_t ch)
{
  /* If we are in command repeat mode, then don't append */

  if (vi->cmdrepeat || vi->cmdcount == 0 || ch > 255)
    {
      return;
    }

  /* Don't overflow the command repeat buffer */

  if (vi->cmdcount < CMD_BUFSIZE)
    {
      vi->cmdbuf[vi->cmdcount++] = ch;
    }
}
#endif /* CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT */

/****************************************************************************
 * Name: vi_cmd_mode
 *
 * Description:
 *   Command mode loop
 *
 ****************************************************************************/

static void vi_cmd_mode(FAR struct vi_s *vi)
{
  viinfo("Enter command mode\n");

  /* Loop while we are in command mode */

  while (vi->mode == MODE_COMMAND)
    {
      bool preserve;
      int ch;

      /* Make sure that the display reflects the current state */

      vi_showtext(vi);
      vi_showlinecol(vi);
      vi_setcursor(vi, vi->cursor.row, vi->cursor.column);

      /* Get the next character from the input */

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
      /* Test for end of command repeat */

      if (vi->cmdrepeat && vi->cmdindex == vi->cmdcount)
        {
          /* Terminate the command repeat */

          vi->cmdrepeat = false;
        }

      /* Test for active cmdrepeat */

      if (vi->cmdrepeat)
        {
          /* Read next command from command buffer */

          ch = vi->cmdbuf[vi->cmdindex++];
        }
      else
#endif
        {
          ch = vi_getch(vi);
        }

      /* Handle numeric input.  Zero (0) with no preceding value is a
       * special case: It means to go to the beginning o the line.
       */

      if (isdigit(ch) && (vi->value > 0 || ch != '0'))
        {
          uint32_t tmp = 10 * vi->value + (ch - '0');
          if (tmp > UINT16_MAX)
            {
              tmp = UINT16_MAX;
            }

          /* Update the command repetition count */

          vi->value = tmp;

          viinfo("Value=%ld\n", vi->value);
          continue;
        }

      /* Allow the following during yank / delete modes */

      if (ch != 'f' && ch != 't' && ch != 'w')
        {
          /* Anything other than 'd' disarms line deletion */

          if (ch != 'd')
            {
              vi->delarm = false;
            }

          /* Anything other than 'y' disarms line yanking */

          if (ch != 'y')
            {
              vi->yankarm = false;
            }

          /* Anything other than 'c' disarms line yanking */

          if (ch != 'c')
            {
              vi->chgarm = false;
            }
        }

      /* Anything other than'g' disarms goto top */

      if (ch != 'g')
        {
          vi->toparm = false;
        }

      /* Anything other than'Z' disarms :wq */

      if (ch != 'Z')
        {
          vi->wqarm = false;
        }

      /* Test for empty file */

      if (vi->textsize == 0)
        {
          /* We need some text before we can do anything.  Only accept
           * text insertion commands.
           */

          if (ch != KEY_CMDMODE_APPEND    && ch != KEY_CMDMODE_INSERT    &&
              ch != KEY_CMDMODE_OPENBELOW && ch != KEY_CMDMODE_APPENDEND &&
              ch != KEY_CMDMODE_INSBEGIN  && ch != KEY_CMDMODE_OPENABOVE &&
              ch != KEY_CMDMODE_COLONMODE)
            {
              continue;
            }
        }

      /* Any key press clears the error message */

      vi->error = false;

      /* Then handle the non-numeric character.  Normally the accumulated
       * value will be reset after processing the command.  There are a few
       * exceptions; 'preserve' will be set to 'true' in those exceptional
       * cases.
       */

      preserve = false;
      vi->updatereqcol = true;
      switch (ch)
        {
        case KEY_CMDMODE_UP: /* Move the cursor up one line */
        case KEY_UP:         /* Move the cursor up one line */
          {
            vi->updatereqcol = false;
            vi_cusorup(vi, vi->value);
          }
          break;

        case KEY_CMDMODE_DOWN: /* Move the cursor down one line */
        case KEY_DOWN:         /* Move the cursor down one line */
          {
            vi->updatereqcol = false;
            vi_cursordown(vi, vi->value);
          }
          break;

        case KEY_CMDMODE_LEFT: /* Move the cursor left N characters */
        case KEY_LEFT:         /* Move the cursor left N characters */
          {
            vi->curpos = vi_cursorleft(vi, vi->curpos, vi->value);
          }
          break;

        case KEY_CMDMODE_RIGHT: /* Move the cursor right one character */
        case KEY_RIGHT:         /* Move the cursor right one character */
          {
            if (vi->text[vi->curpos] != '\n' &&
                vi->text[vi->curpos + 1] != '\n')
              {
                vi->curpos = vi_cursorright(vi, vi->curpos, vi->value);
                if (vi->curpos >= vi->textsize)
                  {
                    vi->curpos = vi->textsize - 1;
                  }
              }
          }
          break;

        case KEY_CMDMODE_BEGINLINE: /* Move cursor to start of current line */
        case KEY_HOME:
          {
            vi->curpos = vi_linebegin(vi, vi->curpos);
          }
          break;

        case KEY_CMDMODE_ENDLINE: /* Move cursor to end of current line */
        case KEY_END:
          {
            vi->curpos = vi_lineend(vi, vi->curpos);
            vi->reqcolumn = 65535;
            vi->updatereqcol = false;
          }
          break;

        case KEY_CMDMODE_PAGEUP: /* Move up (backward) one screen */
        case KEY_PPAGE:
          {
            vi->updatereqcol = false;
            vi_cusorup(vi, vi->display.row);
          }
          break;

        case KEY_CMDMODE_PAGEDOWN: /* Move down (forward) one screen */
        case KEY_NPAGE:
          {
            vi->updatereqcol = false;
            vi_cursordown(vi, vi->display.row);
          }
          break;

        case KEY_CMDMODE_HALFUP: /* Move up (backward) one screen */
          {
            vi->updatereqcol = false;
            vi_cusorup(vi, vi->display.row >> 1);
          }
          break;

        case KEY_CMDMODE_HALFDOWN: /*  Move down (forward) one half screen */
          {
            vi->updatereqcol = false;
            vi_cursordown(vi, vi->display.row >> 1);
          }
          break;

        case KEY_CMDMODE_TOP:     /* Move to top of screen */
          {
            vi->curpos = vi->winpos;
          }
          break;

        case KEY_CMDMODE_BOTTOM:  /* Move to bottom of screen */
          {
            vi_gotoscreenbottom(vi, 0);
          }
          break;

        case KEY_CMDMODE_MIDDLE:  /* Move to middle of screen */
          {
            /* Find bottom row number, then move to half that */

            off_t pos = vi_gotoscreenbottom(vi, 0);
            vi_gotoscreenbottom(vi, pos);
          }
          break;

        case KEY_CMDMODE_FIRSTCHAR:
          {
            vi_gotofirstnonwhite(vi);
          }
          break;

        case KEY_CMDMODE_GOTOTOP: /* Go to top of document */
          {
            if (vi->toparm)
              {
                vi->curpos = 0;
                vi->redrawline = true;
                vi->toparm = false;
              }
            else
              {
                vi->toparm = true;
              }
          }
          break;

        case KEY_CMDMODE_FINDNEXT:
          {
            if (vi->revfind)
              {
                vi_findprev(vi);
              }
            else
              {
                vi_findnext(vi);
              }
            break;
          }

        case KEY_CMDMODE_FINDPREV:
          {
            if (vi->revfind)
              {
                vi_findnext(vi);
              }
            else
              {
                vi_findprev(vi);
              }
            break;
          }
          break;

        case ASCII_BS:  /* Delete N characters before the cursor */
          {
            /* Move the cursor to the left */

            if (vi->curpos > 0)
              {
                vi->curpos--;

                /* If we moved to \n on the previous line, skip it */

                if (vi->curpos > 0 && vi->text[vi->curpos] == '\n')
                  {
                    vi->curpos--;
                  }
              }
          }
          break;

        case KEY_CMDMODE_DEL_LINE: /* Delete the current line */
          {
            if (vi->delarm)
              {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
                vi_saverepeat(vi, ch);
                vi_appendrepeat(vi, ch);
#endif
                vi_delline(vi);
                vi->delarm = false;
              }
            else
              {
                vi->delarm = true;
                preserve = true;
              }
          }
          break;

        case KEY_CMDMODE_CHANGE:
          {
            if (vi->chgarm)
              {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
                vi_saverepeat(vi, ch);
                vi_appendrepeat(vi, ch);
#endif
                vi_gotofirstnonwhite(vi);
                vi_deltoeol(vi);
                vi_setmode(vi, MODE_INSERT, 0);
                if (vi->curpos == vi->textsize)
                  {
                    vi->curpos = vi_cursorright(vi, vi->curpos, 1) + 1;
                  }
                else
                  {
                    vi->curpos = vi_cursorright(vi, vi->curpos, 1);
                  }

                vi->chgarm = false;
              }
            else
              {
                vi->chgarm = true;
                preserve = true;
              }
          }
          break;

        case KEY_CMDMODE_DELTOEOL:  /* Delete to end of current line */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_deltoeol(vi);
          }
          break;

        case KEY_CMDMODE_DELBACKWARD:  /* Delete from cursor forward */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_delbackward(vi);
          }
          break;

        case KEY_CMDMODE_YANK:  /* Yank the current line(s) into the buffer */
          {
            if (vi->yankarm)
              {
                vi_yank(vi, false);
                vi->yankarm = false;
              }
            else
              {
                vi->yankarm = true;
                preserve = true;
              }
          }
          break;

        case KEY_CMDMODE_PASTE: /* Paste line(s) from into text after current line */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_paste(vi, false);
          }
          break;

        case KEY_CMDMODE_PASTEBEFORE: /* Paste text before cursor position */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_paste(vi, true);
          }
          break;

        case KEY_CMDMODE_REPLACECH: /* Replace character(s) under cursor */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_setmode(vi, SUBMODE_REPLACECH, vi->value);
            preserve = true;
          }
          break;

        case KEY_CMDMODE_REPLACE: /* Replace character(s) under cursor until ESC */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_setmode(vi, MODE_REPLACE, 0);
          }
          break; /* Not implemented */

        case KEY_CMDMODE_FINDINLINE:  /* Find character(s) in current line */
        case KEY_CMDMODE_TFINDINLINE: /* Find character(s) in current line */
          {
            vi->tfind = ch == KEY_CMDMODE_TFINDINLINE;
            vi->mode = MODE_FINDINLINE;
            preserve = true;
          }
          break;

        case KEY_CMDMODE_OPENBELOW: /* Enter insertion mode in new line below current */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_setmode(vi, MODE_INSERT, 0);

            /* Go forward to the end of the current line */

            vi->curpos = vi_lineend(vi, vi->curpos);
            if (vi->curpos != vi->textsize)
              {
                /* Include the '\n' */

                vi->curpos++;
              }

            /* Insert a newline to break the line.  The cursor now points
             * beginning of the new line.
             */

            vi_insertch(vi, '\n');

            /* Then enter insert mode */

            vi->drawtoeos = true;
          }
          break;

        case KEY_CMDMODE_OPENABOVE: /* Enter insertion mode in new line above current */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            /* Back up to the beginning of the end of the previous line */

            off_t pos  = vi_linebegin(vi, vi->curpos);
            if (pos == 0)
              {
                /* Insert newline at beginning of file, then move to previous
                 * line.
                 */

                vi->curpos = 0;
                vi_insertch(vi, '\n');
                vi->curpos = vi_prevline(vi, vi->curpos);
              }
            else
              {
                /* Insert a newline to open the line.  The cursor will now
                 * point to thebeginning of newly openly line before the
                 * current line.
                 */

                pos = vi_prevline(vi, pos);
                vi->curpos = vi_lineend(vi, pos)+1;
                vi_insertch(vi, '\n');
              }

            /* Then enter insert mode */

            vi_setmode(vi, MODE_INSERT, 0);
            vi->drawtoeos = true;
          }
          break;

        case KEY_CMDMODE_CHANGETOEOL:  /* Delete to end of current line */
          {
            /* First delete to end of line */

            vi_deltoeol(vi);
          }

          /* Now enter insert mode by falling through the case */

        case KEY_CMDMODE_APPEND: /* Enter insertion mode after the current
                                  * cursor position */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_setmode(vi, MODE_INSERT, 0);

            if (vi->curpos == vi->textsize)
              {
                vi->curpos = vi_cursorright(vi, vi->curpos, 1) + 1;
              }
            else
              {
                vi->curpos = vi_cursorright(vi, vi->curpos, 1);
              }
          }
          break;

        case KEY_CMDMODE_APPENDEND: /* Enter insertion mode at the end of the current line */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_setmode(vi, MODE_INSERT, 0);
            vi->curpos = vi_lineend(vi, vi->curpos) + 1;
          }
          break;

        case KEY_CMDMODE_SUBSTITUTE:
        case KEY_CMDMODE_DEL: /* Delete N characters at the cursor */
        case KEY_DC:
        case ASCII_DEL:
          {
            off_t pos = vi->curpos;

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            /* If we are at the end of the line, then delete backward */

            if (vi->text[pos] == '\n')
              {
                /* Nothing to do */

                break;
              }
            else if (pos + 1 != vi->textsize && vi->text[pos + 1] == '\n')
              {
                if (pos > 0)
                  {
                    vi_delforward(vi);
                    vi->curpos = vi_cursorleft(vi, vi->curpos, 1);
                  }
              }
            else
              {
                vi_delforward(vi);
                vi->redrawline = true;
              }
          }

          /* For 's'ubstitute key, we go into insert mode */

          if (ch == KEY_CMDMODE_SUBSTITUTE)
            {
              vi_setmode(vi, MODE_INSERT, 0);
            }

          break;

        case KEY_CMDMODE_INSBEGIN: /* Enter insertion mode at the beginning of the current line */
          {
            vi->curpos = vi_linebegin(vi, vi->curpos);
          }

          /* Fall through */

        case KEY_CMDMODE_INSERT: /* Enter insertion mode before the current cursor position */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_setmode(vi, MODE_INSERT, 0);
          }
          break;

        case KEY_CMDMODE_JOIN:  /* Join line below with current line */
          {
#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
            vi_saverepeat(vi, ch);
#endif
            vi_join(vi);
          }
          break;

        case KEY_CMDMODE_COLONMODE: /* Enter : command sub-mode */
          {
            vi->updatereqcol = false;
            vi_setsubmode(vi, SUBMODE_COLON, ':', 0);
          }
          break;

        case KEY_CMDMODE_SAVEQUIT:  /* Two of these is the same as :wq */
          {
            if (vi->wqarm)
              {
                /* Emulate :wq */

                strcpy(vi->scratch, "wq");
                vi->cmdlen = 2;
                vi_parsecolon(vi);

                /* If save quit succeeds, we won't return */
              }
            else
              {
                vi->wqarm = true;
              }
          }
          break;

        case KEY_CMDMODE_FINDMODE: /* Enter / find sub-mode */
          {
            vi->updatereqcol = false;
            vi_setsubmode(vi, SUBMODE_FIND, '/', 0);
          }
          break;

        case KEY_CMDMODE_REVFINDMODE: /* Enter / find sub-mode */
          {
            vi->updatereqcol = false;
            vi_setsubmode(vi, SUBMODE_REVFIND, '?', 0);
          }
          break;

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
        case KEY_CMDMODE_REPEAT: /* Repeat the last command */
          {
            if (vi->cmdcount < CMD_BUFSIZE)
              {
                vi->cmdindex = 0;
                vi->cmdrepeat = true;
                vi->value = vi->value > 0 ? vi->value : vi->repeatvalue > 0 ?
                            vi->repeatvalue : 1;
                preserve = true;
              }
            else
              {
                VI_BEL(vi);
              }
          }
          break;
#endif

        case KEY_CMDMODE_GOTO:  /* Go to line specified by the accumulated value */
          {
            vi_gotoline(vi);
          }
          break;

        case KEY_CMDMODE_WORDFWD: /* Go to line specified by the accumulated value */
          {
            vi_gotonextword(vi);
          }
          break;

        case KEY_CMDMODE_WORDBACK: /* Go to line specified by the accumulated value */
          {
            vi_gotoprevword(vi);
          }
          break;

#if defined(CONFIG_EOL_IS_CR)
        case KEY_CMDMODE_NEXTLINE:
        case '\r': /* CR terminates line */
          {
            vi->curpos = vi_nextline(vi, vi->curpos);
            vi_gotofirstnonwhite(vi);
          }
          break;

#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
       case '\r': /* Wait for the LF */
          break;
#endif

#if defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
        case KEY_CMDMODE_NEXTLINE:
        case '\n': /* LF terminates line */
          {
            vi->curpos = vi_nextline(vi, vi->curpos);
            vi_gotofirstnonwhite(vi);
          }
          break;
#endif

#ifdef CONFIG_EOL_IS_EITHER_CRLF
        case KEY_CMDMODE_NEXTLINE:
        case '\r': /* Either CR or LF terminates line */
        case '\n':
          {
            vi->curpos = vi_nextline(vi, vi->curpos);
            vi_gotofirstnonwhite(vi);
          }
          break;
#endif

        case KEY_CMDMODE_PREVLINE:
          {
            vi->curpos = vi_prevline(vi, vi->curpos);
            vi_gotofirstnonwhite(vi);
          }
          break;

        /* Unimplemented and invalid commands */

        case KEY_CMDMODE_REDRAW:  /* Redraws the screen */
        case KEY_CMDMODE_REDRAW2: /* Redraws the screen, removing deleted lines */
        case KEY_CMDMODE_MARK:    /* Place a mark beginning at the current cursor position */
        default:
          {
            if (ch == -1)
              {
                continue;
              }
            else
              {
                VI_BEL(vi);
              }
          }
          break;
        }

      /* Any non-numeric input will reset the accumulated value (after it has
       * been used).  There are a few exceptions:
       *
       * - For the double character sequences, we need to retain the value
       *   until the next character is entered.
       * - If we are changing modes, then we may need to preserve the 'value'
       *   as well; in some cases settings are passed to the new mode in
       *   'value' (vi_setmode() will have set or cleared 'value'
       *   appropriately).
       */

      if (!preserve)
        {
          vi->value = 0;
        }
    }
}

/****************************************************************************
 * Common Sub-Mode Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_cmdch
 *
 * Description:
 *   Insert one character into the data entry line
 *
 ****************************************************************************/

static void vi_cmdch(FAR struct vi_s *vi, char ch)
{
  int index = vi->cmdlen;
  int next  = index + 1;

  viinfo("cmdlen=%d ch=%c[%02x]\n", vi->cmdlen, isprint(ch) ? ch : '.', ch);

  /* Abort gracelessly if the scratch buffer becomes full */

  if (next >= SCRATCH_BUFSIZE)
    {
      vi_exitsubmode(vi, MODE_COMMAND);
      return;
    }

  /* Add the new character to the scratch buffer */

  vi->scratch[index] = ch;
  vi->cmdlen = next;

  /* Update the cursor position */

  vi->cursor.column = next + 1;

  /* And add the new character to the display */

  vi_putch(vi, ch);
  if (ch == '\n')
    {
      vi->drawtoeos = true;
    }
}

/****************************************************************************
 * Name:  vi_cmdbackspace
 *
 * Description:
 *   Process a backspace character in the data entry line
 *
 ****************************************************************************/

static void vi_cmdbackspace(FAR struct vi_s *vi)
{
  viinfo("cmdlen=%d\n", vi->cmdlen);

  if (vi->cmdlen > 0)
    {
      vi_setcursor(vi, vi->display.row - 1, vi->cmdlen);
      vi_clrtoeol(vi);

      /* Update the command index and cursor position */

      vi->cursor.column = vi->cmdlen;

      /* Decrement the number of characters on in the command */

      vi->cmdlen--;
    }
}

/****************************************************************************
 * Colon Data Entry Sub-Mode Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  vi_parsecolon
 *
 * Description:
 *   Parse the colon command collected in the scratch buffer
 *
 ****************************************************************************/

static void vi_parsecolon(FAR struct vi_s *vi)
{
  FAR const char *filename = NULL;
  uint8_t cmd = CMD_NONE;
  bool done = false;
  bool forced;
  int col;
  int ch;

  viinfo("Parse: \"%s\"\n", vi->scratch);

  /* NUL terminate the command */

  vi->scratch[vi->cmdlen] = '\0';

  /* Convert "wq" into "qw" */

  if (vi->cmdlen > 1 && vi->scratch[0] == KEY_COLMODE_WRITE &&
      vi->scratch[1] == KEY_COLMODE_QUIT)
    {
      vi->scratch[0] = KEY_COLMODE_QUIT;
      vi->scratch[1] = KEY_COLMODE_WRITE;
    }

  /* Then parse the contents of the scratch buffer */

  for (col = 0; col < vi->cmdlen && !done; col++)
    {
      /* Get the next command character from the scratch buffer */

      ch = vi->scratch[col];

      /* Check if the next after that is KEY_COLMODE_FORCE */

      forced = false;
      if (col < vi->cmdlen &&  vi->scratch[col + 1] == KEY_COLMODE_FORCE)
        {
          /* Yes.. the operation is forced */

          forced = true;
          col++;
        }

      /* Then process the command character */

      switch (ch)
        {
          case KEY_COLMODE_READ:
            {
              /* Reading a file should not be forced */

              if (cmd == CMD_NONE && !forced)
                {
                  cmd = CMD_READ;
                }
              else
                {
                  /* The read operation is not compatible with writing or
                   * quitting
                   */

                  goto errout_bad_command;
                }
            }
            break;

          case KEY_COLMODE_WRITE:
            {
              /* Are we just writing?  Or writing then quitting? */

              if (cmd == CMD_NONE)
                {
                  /* Just writing.. do we force overwriting? */

                  cmd = (forced ? CMD_OWRITE : CMD_WRITE);
                }
              else if (cmd == CMD_QUIT)
                {
                  /* Both ... do we force overwriting the file? */

                  cmd = (forced ? CMD_OWRITE_QUIT : CMD_WRITE_QUIT);
                }
              else
                {
                  /* Anything else,
                   * including a forced quit is a syntax error
                   */

                  goto errout_bad_command;
                }
            }
            break;

          case KEY_COLMODE_QUIT:
            {
              /* Are we just quitting?  Or writing then quitting? */

              if (cmd == CMD_NONE)
                {
                  /* Just quitting... should we discard any changes? */

                  cmd = (forced ? CMD_DISCARD : CMD_QUIT);
                }

              /* If we are also writing, then it makes no sense to force the
               * quit operation.
               */

              else if (cmd == CMD_WRITE && !forced)
                {
                  cmd = CMD_WRITE_QUIT;
                }
              else if (cmd == CMD_OWRITE && !forced)
                {
                  cmd = CMD_OWRITE_QUIT;
                }
              else
                {
                  /* Quit is not compatible with reading */

                  goto errout_bad_command;
                }
            }
            break;

          default:
            {
              /* Ignore whitespace */

              if (ch != ' ')
                {
                  /* Anything else terminates the loop */

                  done = true;

                  /* If there is anything else on the line, then it must be
                   * a file name.  If we are writing (or reading with an
                   * empty text buffer), then we will need to copy the file
                   * into the filename storage area.
                   */

                  if (ch != '\0')
                    {
                      /* For now, just remember where the file is in the
                       * scratch buffer.
                       */

                      filename = &vi->scratch[col];
                    }
                }
            }
            break;
        }
    }

  /* Did we find any valid command?  A read command requires a filename.
   * A filename where one is not needed is also an error.
   */

  viinfo("cmd=%d filename=\"%s\"\n", cmd, vi->filename);

  if (cmd == CMD_NONE || (IS_READ(cmd) && !filename) ||
      (!USES_FILE(cmd) && filename))
    {
      goto errout_bad_command;
    }

  /* Are we writing to a new filename?  If we are not forcing the write,
   * then we have to check if the file exists.
   */

  if (filename && IS_NOWRITE(cmd))
    {
      /* Check if the file exists */

      if (vi_checkfile(vi, filename))
        {
          /* It does... show an error and exit */

          vi_error(vi, g_fmtfileexists);
          goto errout;
        }
    }

  /* Check if we are trying to quit with un-saved changes.  The user must
   * force quitting in this case.
   */

  if (vi->modified && IS_NDISCARD(cmd))
    {
      /* Show an error and exit */

      vi_error(vi, g_fmtmodified);
      goto errout;
    }

  /* Are we now committed to reading the file? */

  if (IS_READ(cmd))
    {
      /* Was the text buffer empty? */

      bool empty = (vi->textsize == 0);

      /* Yes.. get the cursor position of the beginning of the next line */

      off_t pos = vi_nextline(vi, vi->curpos);

      /* Then read the file into the text buffer at that position. */

      if (vi_insertfile(vi, pos, filename))
        {
          /* Was the text buffer empty before we inserted the file? */

          if (empty)
            {
              /* Yes.. then we want to save the filename and mark the text
               * as unmodified.
               */

              strncpy(vi->filename, filename, MAX_FILENAME - 1);

             /* Make sure that the (possibly truncated) file name is NUL
              * terminated
              */

              vi->filename[MAX_FILENAME - 1] = '\0';
              vi->modified = false;
            }
          else
            {
              /* No.. then we want to retain the filename and mark the text
               * as modified.
               */

              vi->modified = true;
            }
        }
    }

  /* Are we now committed to writing the file? */

  if (IS_WRITE(cmd))
    {
      /* If we are writing to a new file, then we need to copy the filename
       * from the scratch buffer to the filename buffer.
       */

      if (filename)
        {
          strncpy(vi->filename, filename, MAX_FILENAME - 1);

         /* Make sure that the (possibly truncated) file name is NUL
          * terminated
          */

          vi->filename[MAX_FILENAME - 1] = '\0';
        }

      /* If it is not a new file and if there are no changes to the text
       * buffer, then ignore the write.
       */

      if (filename || vi->modified)
        {
          vi_clearbottomline(vi);
          vi_putch(vi, '"');
          vi_write(vi, vi->filename, strlen(vi->filename));
          vi_putch(vi, '"');
          vi_putch(vi, ' ');

          /* Now, finally, we can save the file */

          if (!vi_savetext(vi, vi->filename, 0, vi->textsize))
            {
              /* An error occurred while saving the file and we are
               * not forcing the quit operation.  So error out without
               * quitting until the user decides what to do about
               * the save failure.
               */

              goto errout;
            }

          /* The text buffer contents are no longer modified */

          if (!IS_QUIT(cmd))
            {
              vi_setcursor(vi, vi->cursor.row, vi->cursor.column);
            }

          vi->modified = false;
        }
    }

  /* Are we committed to exit-ing? */

  if (IS_QUIT(cmd))
    {
      /* Yes... free resources and exit */

      vi_putch(vi, '\n');
      vi_release(vi);
      exit(EXIT_SUCCESS);
    }

  /* Otherwise, revert to command mode */

  vi_exitsubmode(vi, MODE_COMMAND);
  return;

errout_bad_command:
  vi_error(vi, g_fmtnotcmd, vi->scratch);

errout:
  vi_exitsubmode(vi, MODE_COMMAND);
}

/****************************************************************************
 * Name: vi_cmd_submode
 *
 * Description:
 *   Colon command sub-mode of the command mode processing
 *
 ****************************************************************************/

static void vi_cmd_submode(FAR struct vi_s *vi)
{
  int ch;

  viinfo("Enter colon command sub-mode\n");

  /* Loop while we are in colon command mode */

  while (vi->mode == SUBMODE_COLON)
    {
      /* Get the next character from the input */

      ch = vi_getch(vi);

      /* Handle the newly received character */

      switch (ch)
        {
          case KEY_COLMODE_QUOTE: /* Quoted character follows */
            {
              /* Insert the next character unconditionally */

              vi_cmdch(vi, vi_getch(vi));
            }
            break;

          case ASCII_BS: /* Delete the character(s) before the cursor */
            {
              if (vi->cmdlen == 0)
                {
                  vi_exitsubmode(vi, MODE_COMMAND);

                  /* Ensure bottom line is cleared */

                  vi_clearbottomline(vi);
                }
              else
                {
                  vi_cmdbackspace(vi);
                }
            }
            break;

          case ASCII_ESC: /* Escape exits colon mode */
            {
              vi_exitsubmode(vi, MODE_COMMAND);
            }
            break;

          /* What do we do with carriage returns? line feeds? */

#if defined(CONFIG_EOL_IS_CR)
          case '\r': /* CR terminates line */
            {
              vi_parsecolon(vi);
            }
            break;

#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\r': /* Wait for the LF */
            break;
#endif

#if defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\n': /* LF terminates line */
            {
              vi_parsecolon(vi);
            }
            break;
#endif

#ifdef CONFIG_EOL_IS_EITHER_CRLF
          case '\r': /* Either CR or LF terminates line */
          case '\n':
            {
              vi_parsecolon(vi);
            }
            break;
#endif

          default:
            {
              /* Ignore all but printable characters */

              if (isprint(ch))
                {
                  /* Insert the filtered character into the scratch buffer */

                  vi_cmdch(vi, ch);
                }
              else
                {
                  VI_BEL(vi);
                }
            }
            break;
        }
    }
}

/****************************************************************************
 * Find Data Entry Sub-Mode Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_findstring
 *
 * Description:
 *   Find the string in the findstr buffer by searching for a matching
 *   sub-string in the text buffer, starting at the current cursor position.
 *
 ****************************************************************************/

static bool vi_findstring(FAR struct vi_s *vi)
{
  off_t pos;
  int len;

  viinfo("findstr: \"%s\"\n", vi->findstr);

  /* The search string is in the find buffer */

  len = strlen(vi->findstr);
  if (!len)
    {
      return false;
    }

  /* Search from the current cursor position forward for a
   * matching sub-string.  Stop loo
   */

  vi_clearbottomline(vi);
  for (pos = vi->curpos;
       pos + len <= vi->textsize;
       pos++)
    {
      /* Check for the matching sub-string */

      if (strncmp(vi->text + pos, vi->scratch, len) == 0)
        {
          /* Found it... save the cursor position and
           * return success.
           */

          vi->curpos = pos;
          return true;
        }
    }

  /* If we get here, then the search string was not found anywhere after the
   * current cursor position.  Start from beginning and search to curpos.
   */

  for (pos = 0; pos <= vi->curpos; pos++)
    {
      /* Check for the matching sub-string */

      if (strncmp(vi->text + pos, vi->scratch, len) == 0)
        {
          vi_write(vi, g_fmtsrcbot, sizeof(g_fmtsrcbot));

          /* Found it... save the cursor position and
           * return success.
           */

          vi->curpos = pos;
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: vi_revfindstring
 *
 * Description:
 *   Find the string in the findstr buffer by searching for a matching
 *   sub-string in the text buffer, starting at the current cursor position.
 *   The search is performed backward through the file.
 *
 ****************************************************************************/

static bool vi_revfindstring(FAR struct vi_s *vi)
{
  off_t pos;
  int len;

  viinfo("findstr: \"%s\"\n", vi->findstr);

  /* The search string is in the find buffer */

  len = strlen(vi->findstr);
  if (!len)
    {
      return false;
    }

  /* Search from the current cursor position forward for a
   * matching sub-string.  Stop loo
   */

  vi_clearbottomline(vi);
  for (pos = vi->curpos;
       pos > 0; pos--)
    {
      /* Check for the matching sub-string */

      if (strncmp(vi->text + pos, vi->scratch, len) == 0)
        {
          /* Found it... save the cursor position and
           * return success.
           */

          vi->curpos = pos;
          return true;
        }
    }

  /* If we get here, then the search string was not found anywhere before the
   * current cursor position.  Start from end and search to curpos.
   */

  for (pos = vi->textsize - len; pos > vi->curpos; pos--)
    {
      /* Check for the matching sub-string */

      if (strncmp(vi->text + pos, vi->scratch, len) == 0)
        {
          vi_write(vi, g_fmtsrctop, sizeof(g_fmtsrctop));

          /* Found it... save the cursor position and
           * return success.
           */

          vi->curpos = pos;
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name:  vi_parsefind
 *
 * Description:
 *   Find the string collected in the scratch buffer.
 *
 ****************************************************************************/

static void vi_parsefind(FAR struct vi_s *vi, bool revfind)
{
  /* Make certain that the scratch buffer contents are NUL terminated */

  vi->scratch[vi->cmdlen] = '\0';

  /* Is there anything in the scratch buffer? If not, then we will use the
   * string from the previous find operation.
   */

  viinfo("scratch: \"%s\"\n", vi->scratch);

  if (vi->cmdlen > 0)
    {
      /* Copy the new search string from the scratch to the find buffer */

      strncpy(vi->findstr, vi->scratch, MAX_STRING - 1);

      /* Make sure that the (possibly truncated) search string is NUL
       * terminated
       */

      vi->findstr[MAX_STRING - 1] = '\0';
    }

  /* Then attempt to find the string */

  vi->revfind = revfind;
  if (revfind)
    {
      vi_revfindstring(vi);
    }
  else
    {
      vi_findstring(vi);
    }

  /* Exit the sub-mode and revert to command mode */

  vi_exitsubmode(vi, MODE_COMMAND);
}

/****************************************************************************
 * Name: vi_find_submode
 *
 * Description:
 *   Find command sub-mode of the command mode processing
 *
 ****************************************************************************/

static void vi_find_submode(FAR struct vi_s *vi, bool revfind)
{
  int ch;

  viinfo("Enter find sub-mode\n");

  /* Loop while we are in find mode */

  while (vi->mode == SUBMODE_FIND || vi->mode == SUBMODE_REVFIND)
    {
      /* Get the next character from the input */

      ch = vi_getch(vi);

      /* Handle the newly received character */

      switch (ch)
        {
          case KEY_FINDMODE_QUOTE: /* Quoted character follows */
            {
              /* Insert the next character unconditionally */

              vi_cmdch(vi, vi_getch(vi));
            }
            break;

          case ASCII_BS: /* Delete the character before the cursor */
            {
              if (vi->cmdlen == 0)
                {
                  vi_exitsubmode(vi, MODE_COMMAND);

                  /* Ensure bottom line is cleared */

                  vi_clearbottomline(vi);
                }
              else
                {
                  vi_cmdbackspace(vi);
                }
            }
            break;

          case ASCII_ESC: /* Escape exits find mode */
            {
              vi_exitsubmode(vi, MODE_COMMAND);
            }
            break;

          /* What do we do with carriage returns? line feeds? */

#if defined(CONFIG_EOL_IS_CR)
          case '\r': /* CR terminates line */
            {
              vi_parsefind(vi, revfind);
            }
            break;

#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\r': /* Wait for the LF */
            break;
#endif

#if defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\n': /* LF terminates line */
            {
              vi_parsefind(vi, revfind);
            }
            break;
#endif

#ifdef CONFIG_EOL_IS_EITHER_CRLF
          case '\r': /* Either CR or LF terminates line */
          case '\n':
            {
              vi_parsefind(vi, revfind);
            }
            break;
#endif

          default:
            {
              /* Ignore all but printable characters */

              if (isprint(ch))
                {
                  /* Insert the filtered character into the scratch buffer */

                  vi_cmdch(vi, ch);
                }
              else
                {
                  VI_BEL(vi);
                }
            }
            break;
        }
    }
}

/****************************************************************************
 * Replace Text Sub-Mode Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_replacech
 *
 * Description:
 *   Replace the character at the current position.  If the current position
 *   is the end of line, then insert the character.
 *
 ****************************************************************************/

static void vi_replacech(FAR struct vi_s *vi, char ch)
{
  viinfo("curpos=%ld ch=%c[%02x]\n", vi->curpos, isprint(ch) ? ch : '.', ch);

  /* Is there a newline at the current cursor position? */

  if (vi->text[vi->curpos] == '\n')
    {
      /* Yes, then insert the new character before the newline */

      vi_insertch(vi, ch);
      vi->drawtoeos = true;
    }
  else
    {
      /* No, just replace the character and increment the cursor position */

      vi->text[vi->curpos++] = ch;
      vi->redrawline = true;
    }
}

/****************************************************************************
 * Name: vi_replacech_submode
 *
 * Description:
 *   Replace character command sub-mode of the command mode processing
 *
 ****************************************************************************/

static void vi_replacech_submode(FAR struct vi_s *vi)
{
  off_t end;
  long nchars;
  bool found = false;
  int ch = 0;

  /* Get the number of characters to replace */

  nchars = (vi->value > 0 ? vi->value : 1);

  viinfo("Enter replaces character(s) sub-mode: nchars=%d\n", nchars);

  /* Are there that many characters left on the line to be replaced? */

  end = vi_lineend(vi, vi->curpos) + 1;
  if (vi->curpos + nchars > end)
    {
      vi_error(vi, g_fmtnotvalid);
      vi_setmode(vi, MODE_COMMAND, 0);
    }

  /* Loop until we get the replacement character */

  while (vi->mode == SUBMODE_REPLACECH && !found)
    {
      /* Get the next character from the input */

      ch = vi_getch(vi);

      /* Handle the newly received character */

      vi->updatereqcol = true;
      switch (ch)
        {
          case KEY_FINDMODE_QUOTE: /* Quoted character follows */
            {
              /* Insert the next character unconditionally */

              ch    = vi_getch(vi);
              found = true;
            }
            break;

          case ASCII_ESC: /* Escape exits replace mode */
            {
              vi_setmode(vi, MODE_COMMAND, 0);
              if (vi->curpos > 0)
                {
                  --vi->curpos;
                }
            }
            break;

          /* What do we do with carriage returns? line feeds? */

#if defined(CONFIG_EOL_IS_CR)
          case '\r': /* CR terminates line */
            {
              ch = '\n';
              found = true;
            }
            break;

#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\r': /* Wait for the LF */
            break;
#endif

#if defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\n': /* LF terminates line */
            {
              found = true;
            }
            break;
#endif

#ifdef CONFIG_EOL_IS_EITHER_CRLF
          case '\r': /* Either CR or LF terminates line */
          case '\n':
            {
              ch = '\n';
              found = true;
            }
            break;
#endif

          default:
            {
              /* Ignore all but printable characters and tab */

              if (isprint(ch)  || ch == '\t')
                {
                  found = true;
                }
              else
                {
                  VI_BEL(vi);
                }
            }

            break;
        }
    }

  /* Now replace with the character nchar times */

  for (; nchars > 0; nchars--)
    {
      vi_replacech(vi, ch);
      vi->redrawline = true;
    }

  /* Revert to command mode */

  vi_setmode(vi, MODE_COMMAND, 0);
}

/****************************************************************************
 * Name: vi_findinline_mode
 *
 * Description:
 *   Find character in current line.
 *
 ****************************************************************************/

static void vi_findinline_mode(FAR struct vi_s *vi)
{
  int count;
  off_t pos;
  int ch = -1;

  /* Get the next character from the input */

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
  /* Test for active cmdrepeat */

  if (vi->cmdrepeat)
    {
      /* Read next command from command buffer */

      ch = vi->cmdbuf[vi->cmdindex++];
    }
  else
#endif
    {
      while (ch == -1)
        {
          ch = vi_getch(vi);
        }
    }

  /* Ignore all but printable characters and tab */

  if (!isprint(ch))
    {
      VI_BEL(vi);
      vi_setmode(vi, MODE_COMMAND, 0);
      return;
    }

  vi->updatereqcol = true;

  /* Now find the character */

  pos = vi->curpos + 1;
  count = vi->value > 0 ? vi->value : 1;

  while (count > 0 && pos < vi->textsize - 1 && vi->text[pos] != '\n')
    {
      /* Increment to next character */

      pos++;

      /* Test if this character matches */

      if (vi->text[pos] == ch)
        {
          count--;
        }
    }

  /* Test if found */

  if (count == 0)
    {
      if (vi->tfind)
        {
          pos--;
        }

      /* Test if yank or del armed */

      if (vi->yankarm || vi->delarm || vi->chgarm)
        {
          /* Yank the text and possibly delete */

          vi_yanktext(vi, vi->curpos, pos, 1, vi->delarm || vi->chgarm);
          if (vi->delarm || vi->chgarm)
            {
              /* Redraw line if text deleted */

              vi->redrawline = true;
            }

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
          /* Setup command repeat */

          if (vi->delarm || vi->chgarm)
            {
              vi_saverepeat(vi, vi->delarm ? 'd' : 'c');
              vi_appendrepeat(vi, vi->tfind ? 't' : 'f');
              vi_appendrepeat(vi, ch);
            }
#endif

          /* If change text is armed, then enter insert mode */

          if (vi->chgarm)
            {
              vi->chgarm = false;
              vi_setmode(vi, MODE_INSERT, 0);
              return;
            }

          vi->delarm = false;
          vi->yankarm = false;
        }
      else
        {
          /* Simply move to the specified location */

          vi->curpos = pos;
        }
    }

  /* Revert to command mode */

  vi_setmode(vi, MODE_COMMAND, 0);
}

/****************************************************************************
 * Insert and Replace Mode Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_insertch
 *
 * Description:
 *   Insert one character into the text buffer
 *
 ****************************************************************************/

static void vi_insertch(FAR struct vi_s *vi, char ch)
{
  viinfo("curpos=%ld ch=%c[%02x]\n", vi->curpos, isprint(ch) ? ch : '.', ch);

  /* Make space in the buffer for the new character */

  if (vi_extendtext(vi, vi->curpos, 1))
    {
      /* Add the new character to the buffer */

      vi->text[vi->curpos++] = ch;
    }
}

/****************************************************************************
 * Name: vi_insert_mode
 *
 * Description:
 *   Insert mode loop
 *
 ****************************************************************************/

static void vi_insert_mode(FAR struct vi_s *vi)
{
  off_t start = vi->curpos;
  int ch;

  viinfo("Enter insert mode\n");

  /* Print insert message */

  vi_clearbottomline(vi);
  vi_write(vi, g_fmtinsert, sizeof(g_fmtinsert));
  vi_setcursor(vi, vi->cursor.row, vi->cursor.column);
  vi->redrawline = true;

  /* Loop while we are in insert mode */

  while (vi->mode == MODE_INSERT || vi->mode == MODE_REPLACE)
    {
      /* Make sure that the display reflects the current state */

      if (vi->redrawline || vi->drawtoeos || vi->fullredraw)
        {
          vi_showtext(vi);
        }

      /* Display the line and col number */

      vi_showlinecol(vi);
      vi_setcursor(vi, vi->cursor.row, vi->cursor.column);

      /* Get the next character from the input */

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
      /* Test for active cmdrepeat */

      if (vi->cmdrepeat)
        {
          /* Read next command from command buffer */

          ch = vi->cmdbuf[vi->cmdindex++];
        }
      else
#endif
        {
          ch = vi_getch(vi);

#ifdef CONFIG_SYSTEM_VI_INCLUDE_COMMAND_REPEAT
          /* Any arrow, pgup, pgdn, etc. key resets command repeat */

          if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT ||
              ch == KEY_RIGHT || ch == KEY_HOME || ch == KEY_END ||
              ch == KEY_PPAGE || ch == KEY_NPAGE)
            {
              vi->cmdcount = 1;
              vi->redrawline = true;
            }
          else
            {
              vi_appendrepeat(vi, ch);
            }
#endif
        }

      /* Test for printable character first since we will get mostly those,
       * and this will give better performance.
       */

      vi->updatereqcol = true;
      if (!iscntrl(ch) || ch == '\t')
        {
          /* Insert the filtered character into the buffer */

          if (vi->mode == MODE_INSERT)
            {
              vi_insertch(vi, ch);
            }
          else
            {
              vi_replacech(vi, ch);
            }

          /* If we don't have to scroll the screen to display the
           * character, then do a simple putch, otherwise request
           * a line redraw.
           */

          if (vi->cursor.column + 1 < vi->display.column && ch != '\t' &&
              (vi->curpos + 1 == vi->textsize ||
               vi->text[vi->curpos + 1] == '\n'))
            {
              vi_putch(vi, ch);
            }
          else
            {
              vi->redrawline = true;
            }

          vi->cursor.column++;

          continue;
        }

      /* Any key press clears the error message */

      vi->error = false;
      vi->redrawline = true;

      /* Handle the newly received character */

      switch (ch)
        {
          case KEY_INSMODE_QUOTE: /* Quoted character follows */
            {
              /* Insert the next character unconditionally */

              vi_insertch(vi, vi_getch(vi));
            }
            break;

          case ASCII_DEL:
            {
              if (vi->curpos < vi->textsize)
                {
                  if (vi->text[vi->curpos] == '\n')
                    {
                      vi->drawtoeos = true;
                    }

                  vi_shrinktext(vi, vi->curpos, 1);
                }
            }
            break;

          case ASCII_BS:
            {
              /* Backspace changes based on mode */

              if (vi->mode == MODE_INSERT)
                {
                  /* In insert mode, we remove characters */

                  if (vi->curpos > 0)
                    {
                      if (vi->text[vi->curpos - 1] == '\n')
                        {
                          vi->drawtoeos = true;
                        }

                      vi_shrinktext(vi, vi->curpos - 1, 1);
                    }
                }
              else
                {
                  /* In replace mode, we simply move the cursor left */

                  if (vi->curpos > start)
                    {
                      vi->curpos = vi_cursorleft(vi, vi->curpos, 1);
                    }
                }
            }
            break;

          case ASCII_ESC: /* Escape exits insert mode */
            {
              vi_setmode(vi, MODE_COMMAND, 0);
              vi->updatereqcol = true;

              /* Move cursor 1 space to the left when exiting insert mode */

              if (vi->curpos > 0 && vi->text[vi->curpos - 1] != '\n')
                {
                  --vi->curpos;
                }
            }
            break;

          /* What do we do with carriage returns? */

#if defined(CONFIG_EOL_IS_CR)
          case '\r': /* CR terminates line */
            {
              if (vi->mode == MODE_INSERT)
                {
                  vi_insertch(vi, '\n');
                }
              else
                {
                  vi_replacech(vi, '\n');
                }

              vi->drawtoeos = true;
            }
            break;

#elif defined(CONFIG_EOL_IS_BOTH_CRLF)
         case '\r': /* Wait for the LF */
            break;
#endif

#if defined(CONFIG_EOL_IS_LF) || defined(CONFIG_EOL_IS_BOTH_CRLF)
          case '\n': /* LF terminates line */
            {
              if (vi->mode == MODE_INSERT)
                {
                  vi_insertch(vi, '\n');
                }
              else
                {
                  vi_replacech(vi, '\n');
                }

              vi->drawtoeos = true;
            }
            break;
#endif

#ifdef CONFIG_EOL_IS_EITHER_CRLF
          case '\r': /* Either CR or LF terminates line */
          case '\n':
            {
              if (vi->mode == MODE_INSERT)
                {
                  vi_insertch(vi, '\n');
                }
              else
                {
                  vi_replacech(vi, '\n');
                }

              vi_putch(vi, ' ');
              vi_clrtoeol(vi);
              vi->drawtoeos = true;
            }
            break;
#endif

          case KEY_UP:         /* Move the cursor up one line */
            {
              vi->updatereqcol = false;
              vi_cusorup(vi, 1);
            }
            break;

          case KEY_DOWN:         /* Move the cursor down one line */
            {
              vi->updatereqcol = false;
              vi_cursordown(vi, 1);
            }
            break;

          case KEY_LEFT:         /* Move the cursor left N characters */
            {
              vi->curpos = vi_cursorleft(vi, vi->curpos, 1);
            }
            break;

          case KEY_RIGHT:         /* Move the cursor right one character */
            {
              vi->curpos = vi_cursorright(vi, vi->curpos, 1);
              if (vi->curpos >= vi->textsize)
                {
                  vi->curpos = vi->textsize - 1;
                }
            }
            break;

          case KEY_HOME:
            {
              vi->curpos = vi_linebegin(vi, vi->curpos);
            }
            break;

          case KEY_END:
            {
              vi->curpos = vi_lineend(vi, vi->curpos);
            }
            break;

          case KEY_PPAGE:
            {
              vi->updatereqcol = false;
              vi_cusorup(vi, vi->display.row);
            }
            break;

          case KEY_NPAGE:
            {
              vi->updatereqcol = false;
              vi_cursordown(vi, vi->display.row);
            }
            break;

          default:
            {
              /* Don't print BEL if char is -1 from termcurses */

              if (ch == -1)
                {
                  continue;
                }
              else
                {
                  VI_BEL(vi);
                }
            }
            break;
        }
    }

  vi_clearbottomline(vi);
}

/****************************************************************************
 * Command line processing
 ****************************************************************************/

/****************************************************************************
 * Name: vi_showusage
 *
 * Description:
 *   Show command line arguments and exit.
 *
 ****************************************************************************/

static void vi_release(FAR struct vi_s *vi)
{
  if (vi)
    {
      if (vi->text)
        {
          free(vi->text);
        }

      if (vi->yank)
        {
          free(vi->yank);
        }

      if (vi->tcurs)
        {
          termcurses_deinitterm(vi->tcurs);
        }

      free(vi);
    }
}

/****************************************************************************
 * Name: vi_showusage
 *
 * Description:
 *   Show command line arguments and exit.
 *
 ****************************************************************************/

static void vi_showusage(FAR struct vi_s *vi, FAR const char *progname,
                         int exitcode)
{
  fprintf(stderr, "\nUSAGE:\t%s [-c <columns] [-r <rows>] [<filename>]\n",
          progname);
  fprintf(stderr, "\nUSAGE:\t%s -h\n\n",
          progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<filename>:\n");
  fprintf(stderr, "\t\tOptional name of the file to open\n");
  fprintf(stderr, "\t-c <columns>:\n");
  fprintf(stderr,
          "\t\tOptional width of the display in columns.  Default: %d\n",
          CONFIG_SYSTEM_VI_COLS);
  fprintf(stderr, "\t-r <rows>:\n");
  fprintf(stderr,
          "\t\tOptional height of the display in rows.  Default: %d\n",
          CONFIG_SYSTEM_VI_ROWS);
  fprintf(stderr, "\t-h:\n");
  fprintf(stderr, "\t\tShows this message and exits.\n");

  /* Release all allocated resources and exit */

  vi_release(vi);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vi_main
 *
 * Description:
 *   The main entry point into vi.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct vi_s *vi;
  int option;
  int ret;

  /* Allocate a vi state structure */

  vi = (FAR struct vi_s *)zalloc(sizeof(struct vi_s));
  if (vi == NULL)
    {
      vi_error(vi, g_fmtallocfail);
      return EXIT_FAILURE;
    }

  /* Initialize non-zero elements of the vi state structure */

  vi->display.row    = CONFIG_SYSTEM_VI_ROWS;
  vi->display.column = CONFIG_SYSTEM_VI_COLS;

  /* Parse command line arguments */

  while ((option = getopt(argc, argv, ":c:r:h")) != ERROR)
    {
      switch (option)
        {
          case 'c': /* Display width in columns */
            {
              unsigned long value = strtoul(optarg, NULL, 10);
              if (value <= UINT16_MAX)
                {
                  vi->display.column = (uint16_t)value;
                }
              else
                {
                  fprintf(stderr, "ERROR: Column value out of range: %lu\n",
                          value);
                  vi_showusage(vi, argv[0], EXIT_FAILURE);
                }
            }
            break;

          case 'r': /* Display width in columns */
            {
              unsigned long value = strtoul(optarg, NULL, 10);
              if (value <= UINT16_MAX)
                {
                  vi->display.row = (uint16_t)value;
                }
              else
                {
                  fprintf(stderr, "ERROR: Row value out of range: %lu\n",
                          value);
                  vi_showusage(vi, argv[0], EXIT_FAILURE);
                }
            }
            break;

          case 'h':
            {
              vi_showusage(vi, argv[0], EXIT_SUCCESS);
            }
            break;

          case ':':
            {
              fprintf(stderr, "ERROR: Missing parameter argument\n");
              vi_showusage(vi, argv[0], EXIT_FAILURE);
            }
            break;

          case '?':
          default:
            {
              fprintf(stderr, "ERROR: Unrecognized parameter\n");
              vi_showusage(vi, argv[0], EXIT_FAILURE);
            }
            break;
        }
    }

  /* Initialize termcurses */

  ret = termcurses_initterm(NULL, 0, 1, &vi->tcurs);
  if (ret == OK)
    {
      struct winsize winsz;

      ret = termcurses_getwinsize(vi->tcurs, &winsz);
      if (ret == OK)
        {
          vi->display.row    = winsz.ws_row;
          vi->display.column = winsz.ws_col;
        }
    }

  /* There maybe one additional argument on the command line: The filename */

  if (optind < argc)
    {
      /* Copy the file name into the file name buffer */

      if (argv[optind][0] == '/')
        {
          strncpy(vi->filename, argv[optind], MAX_STRING - 1);
        }
      else
        {
          /* Make file relative to current working directory */

          getcwd(vi->filename, MAX_STRING - 1);
          strncat(vi->filename, "/", MAX_STRING - 1);
          strncat(vi->filename, argv[optind], MAX_STRING - 1);
        }

      /* Make sure the (possibly truncated) file name is NUL terminated */

      vi->filename[MAX_STRING - 1] = '\0';

      /* Load the file into memory */

      vi_insertfile(vi, 0, vi->filename);
      vi->modified = false;

      /* Skip over the filename argument.  There should nothing after this */

      optind++;
    }

  /* If no file loaded, create an empty buffer for editing */

  if (vi->text == NULL)
    {
      vi_extendtext(vi, 0, TEXT_GULP_SIZE);
      vi->textsize = 0;
      vi->modified = 0;
    }

  if (optind != argc)
    {
      fprintf(stderr, "ERROR: Too many arguments\n");
      vi_showusage(vi, argv[0], EXIT_FAILURE);
    }

  /* The editor loop */

  vi->fullredraw = true;
  for (; ; )
    {
      /* We loop, processing each mode change */

      viinfo("mode=%d\n", vi->mode);

      switch (vi->mode)
        {
          default:
          case MODE_COMMAND:      /* Command mode */
            vi_cmd_mode(vi);
            break;

          case SUBMODE_COLON:     /* Colon data entry in command mode */
            vi_cmd_submode(vi);
            break;

          case SUBMODE_REVFIND:
          case SUBMODE_FIND:      /* Find data entry in command mode */
            vi_find_submode(vi, vi->mode == SUBMODE_REVFIND);
            break;

          case SUBMODE_REPLACECH: /* Replace characters in command mode */
            vi_replacech_submode(vi);
            break;

          case MODE_INSERT:       /* Insert mode */
          case MODE_REPLACE:      /* Replace character(s) under cursor until ESC */
            vi_insert_mode(vi);
            break;

          case MODE_FINDINLINE:       /* Insert mode */
            vi_findinline_mode(vi);
            break;
        }
    }

  /* We won't get here */
}
