/************************************************************************************
 * apps/system/termcurses/tcurses_vt100.c
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
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>

#include "tcurses_priv.h"
#include "graphics/curses.h"

#include <sys/ioctl.h>
#include <nuttx/kmalloc.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

#define KEY_DOWN        0x102  /* Down arrow key */
#define KEY_UP          0x103  /* Up arrow key */
#define KEY_LEFT        0x104  /* Left arrow key */
#define KEY_RIGHT       0x105  /* Right arrow key */
#define KEY_HOME        0x106  /* home key */
#define KEY_F0          0x108  /* function keys; 64 reserved */

#ifdef CONFIG_TERMINFO_INCLUDE_NAME
#define TINFO_ENTRY(n, d, c)  n, d, c
#else
#define TINFO_ENTRY(n, d, c)  d, c
#endif

/************************************************************************************
 * Private Types
 ************************************************************************************/

struct tcurses_vt100_s
{
  struct termcurses_dev_s dev;     /* The parent TermCurses struct */

  /* Other control variables go here */

  int    in_fd;
  int    out_fd;
  int    keycount;
  char   keybuf[16];
};

/************************************************************************************
 * Private Function Prototypes
 ************************************************************************************/

static FAR struct termcurses_s *tcurses_vt100_initialize(int in_fd, int out_fd);
static int tcurses_vt100_clear(FAR struct termcurses_s *dev, int type);
static int tcurses_vt100_move(FAR struct termcurses_s *dev, int type, int col,
              int row);
static int tcurses_vt100_getwinsize(FAR struct termcurses_s *dev,
              FAR struct winsize *winsz);
static int tcurses_vt100_setcolors(FAR struct termcurses_s *dev,
              FAR struct termcurses_colors_s *colors);
static int tcurses_vt100_setattributes(FAR struct termcurses_s *dev,
              unsigned long attrib);
static int tcurses_vt100_getkeycode(FAR struct termcurses_s *dev,
              FAR int *specialkey, FAR int *keymodifers);
static bool tcurses_vt100_checkkey(FAR struct termcurses_s *dev);

/************************************************************************************
 * Private Data
 ************************************************************************************/

static const struct termcurses_ops_s g_vt100_ops =
{
  tcurses_vt100_initialize,      /* Initialize operator */
  tcurses_vt100_clear,
  tcurses_vt100_move,
  tcurses_vt100_getwinsize,
  tcurses_vt100_setcolors,
  tcurses_vt100_setattributes,
  tcurses_vt100_getkeycode,
  tcurses_vt100_checkkey
};

/* VT100 terminal codes */

static const char *g_clrscr         = "\033[2J";      /* Clear screen */
static const char *g_clreos         = "\033[J";       /* Clear to end of screen */
static const char *g_clreol         = "\033[K";       /* Clear to end of line */

static const char *g_movecurs       = "\033[%d;%dH";  /* Move cursor to x,y */
static const char *g_getwinsize     = "\x1b[s\x1b[999;999H\x1b[6n\x1bu";
static const char *g_setfgcolor     = "\x1b[38;5;%dm";
static const char *g_setbgcolor     = "\x1b[48;5;%dm";
static const char *g_showcursor     = "\x1b[?25h";
static const char *g_hidecursor     = "\x1b[?25l";
static const char *g_setbold        = "\x1b[1";
static const char *g_setnobold      = "\x1b[22";
static const char *g_setblink       = ";5";
static const char *g_setnoblink     = ";25";
static const char *g_setunderline   = ";4";
static const char *g_setnounderline = ";24";

struct keycodes_s
{
#ifdef CONFIG_TERMCURSES_INCLUDE_TERMINFO_NAME
  const char *terminfo_name;
#endif
  const char *def;
  int         keycode;
};

static const uint16_t g_key_modifiers[][2] =
{
  {
    KEY_PPAGE  | (PDC_KEY_MODIFIER_ALT     << 12), ALT_PGDN
  },
  {
    KEY_NPAGE  | (PDC_KEY_MODIFIER_ALT     << 12), ALT_PGUP
  },
  {
    KEY_UP     | (PDC_KEY_MODIFIER_SHIFT   << 12), KEY_SUP
  },
  {
    KEY_DOWN   | (PDC_KEY_MODIFIER_SHIFT   << 12), KEY_SDOWN
  },
  {
    KEY_LEFT   | (PDC_KEY_MODIFIER_SHIFT   << 12), KEY_SLEFT
  },
  {
    KEY_RIGHT  | (PDC_KEY_MODIFIER_SHIFT   << 12), KEY_SRIGHT
  },
  {
    KEY_UP     | (PDC_KEY_MODIFIER_CONTROL << 12), CTL_UP
  },
  {
    KEY_DOWN   | (PDC_KEY_MODIFIER_CONTROL << 12), CTL_DOWN
  },
  {
    KEY_LEFT   | (PDC_KEY_MODIFIER_CONTROL << 12), CTL_LEFT
  },
  {
    KEY_RIGHT  | (PDC_KEY_MODIFIER_CONTROL << 12), CTL_RIGHT
  },
  {
    KEY_DC     | (PDC_KEY_MODIFIER_SHIFT   << 12), KEY_SDC
  },
  {
    KEY_DC     | (PDC_KEY_MODIFIER_ALT     << 12), ALT_DEL
  },
  {
    KEY_DC     | (PDC_KEY_MODIFIER_CONTROL << 12), CTL_DEL
  },
  {
    KEY_IC     | (PDC_KEY_MODIFIER_ALT     << 12), ALT_INS
  }
};

static const int g_key_modifier_count =
  sizeof(g_key_modifiers) / (sizeof(uint16_t)*2);

static const struct keycodes_s g_esc_keycodes[] =
{
  /*             terminfo                   pdcurses
   *             name       definition      key code
   */

  {
    TINFO_ENTRY("kcuu1",    "[A",       KEY_UP)
  },      /* Up Arrow */
  {
    TINFO_ENTRY("kcud1",    "[B",       KEY_DOWN)
  },      /* Down Arrow */
  {
    TINFO_ENTRY("kcuf1",    "[C",       KEY_RIGHT)
  },      /* Right Arrow */
  {
    TINFO_ENTRY("kcub1",    "[D",       KEY_LEFT)
  },      /* Left Arrow */
  {
    TINFO_ENTRY("knp",      "[6~",      KEY_NPAGE)
  },      /* Next Page Key */
  {
    TINFO_ENTRY("kpp",      "[5~",      KEY_PPAGE)
  },      /* Prev Page Key */
  {
    TINFO_ENTRY("khom",     "OH",       KEY_HOME)
  },      /* Home Key */
  {
    TINFO_ENTRY("kend",     "OF",       KEY_END)
  },      /* Home Key */
  {
    TINFO_ENTRY("kcab1",    "b",        ALT_LEFT)
  },      /* Alt Left arrow Key */
  {
    TINFO_ENTRY("kcaf1",    "f",        ALT_RIGHT)
  },      /* Alt Right arrow Key */
  {
    TINFO_ENTRY("kprt",     "[25~",     KEY_PRINT)
  },      /* Print-screen Key */
  {
    TINFO_ENTRY("kdc",      "[3~",      KEY_DC)
  },      /* Delete char key */
  {
    TINFO_ENTRY("kic",      "[2~",      KEY_IC)
  },      /* Insert Key */
  {
    TINFO_ENTRY("kacr",     "\x1a",     ALT_ENTER)
  },      /* ALT-Enter Key */
  {
    TINFO_ENTRY("kTAB",     "[Z",       KEY_STAB)
  },      /* SHIFT-tab Key */

  {
    TINFO_ENTRY("kf1",      "OP",       KEY_F(1))
  },      /* F1 Key */
  {
    TINFO_ENTRY("kf2",      "OQ",       KEY_F(2))
  },      /* F2 Key */
  {
    TINFO_ENTRY("kf3",      "OR",       KEY_F(3))
  },      /* F3 Key */
  {
    TINFO_ENTRY("kf4",      "OS",       KEY_F(4))
  },      /* F4 Key */
  {
    TINFO_ENTRY("kf5",      "[15~",     KEY_F(5))
  },      /* F5 Key */
  {
    TINFO_ENTRY("kf6",      "[17~",     KEY_F(6))
  },      /* F6 Key */
  {
    TINFO_ENTRY("kf7",      "[18~",     KEY_F(7))
  },      /* F7 Key */
  {
    TINFO_ENTRY("kf8",      "[19~",     KEY_F(8))
  },      /* F8 Key */
  {
    TINFO_ENTRY("kf9",      "[20~",     KEY_F(9))
  },      /* F9 Key */
  {
    TINFO_ENTRY("kf10",     "[21~",     KEY_F(10))
  },      /* F10 Key */
  {
    TINFO_ENTRY("kf11",     "[22~",     KEY_F(11))
  },      /* F11 Key */
  {
    TINFO_ENTRY("kf12",     "[24~",     KEY_F(12))
  },      /* F12 Key */

  {
    TINFO_ENTRY("",         "Oq",       KEY_C1)
  },      /* Lower left PAD Key */
  {
    TINFO_ENTRY("",         "Or",       KEY_C2)
  },      /* Lower middle PAD  Key */
  {
    TINFO_ENTRY("",         "Os",       KEY_C3)
  },      /* Lower right PAD  Key */
  {
    TINFO_ENTRY("",         "Ot",       KEY_B1)
  },      /* Middle left PAD Key */
  {
    TINFO_ENTRY("",         "Ou",       KEY_B2)
  },      /* Middle middle PAD  Key */
  {
    TINFO_ENTRY("",         "Ov",       KEY_B3)
  },      /* Middle right PAD  Key */
  {
    TINFO_ENTRY("",         "Ow",       KEY_A1)
  },      /* Upper left PAD Key */
  {
    TINFO_ENTRY("",         "Ox",       KEY_A2)
  },      /* Upper middle PAD  Key */
  {
    TINFO_ENTRY("",         "Oy",       KEY_A3)
  },      /* Upper right PAD  Key */

  {
    TINFO_ENTRY("",         "a",        ALT_A)
  },      /* ALT-A key */
  {
    TINFO_ENTRY("",         "b",        ALT_B)
  },      /* ALT-B key */
  {
    TINFO_ENTRY("",         "c",        ALT_C)
  },      /* ALT-C key */
  {
    TINFO_ENTRY("",         "d",        ALT_D)
  },      /* ALT-D key */
  {
    TINFO_ENTRY("",         "e",        ALT_E)
  },      /* ALT-E key */
  {
    TINFO_ENTRY("",         "f",        ALT_F)
  },      /* ALT-F key */
  {
    TINFO_ENTRY("",         "g",        ALT_G)
  },      /* ALT-G key */
  {
    TINFO_ENTRY("",         "h",        ALT_H)
  },      /* ALT-H key */
  {
    TINFO_ENTRY("",         "i",        ALT_I)
  },      /* ALT-I key */
  {
    TINFO_ENTRY("",         "j",        ALT_J)
  },      /* ALT-J key */
  {
    TINFO_ENTRY("",         "k",        ALT_K)
  },      /* ALT-K key */
  {
    TINFO_ENTRY("",         "l",        ALT_L)
  },      /* ALT-L key */
  {
    TINFO_ENTRY("",         "m",        ALT_M)
  },      /* ALT-M key */
  {
    TINFO_ENTRY("",         "n",        ALT_N)
  },      /* ALT-N key */
  {
    TINFO_ENTRY("",         "o",        ALT_O)
  },      /* ALT-O key */
  {
    TINFO_ENTRY("",         "p",        ALT_P)
  },      /* ALT-P key */
  {
    TINFO_ENTRY("",         "q",        ALT_Q)
  },      /* ALT-Q key */
  {
    TINFO_ENTRY("",         "r",        ALT_R)
  },      /* ALT-R key */
  {
    TINFO_ENTRY("",         "s",        ALT_S)
  },      /* ALT-S key */
  {
    TINFO_ENTRY("",         "t",        ALT_T)
  },      /* ALT-T key */
  {
    TINFO_ENTRY("",         "u",        ALT_U)
  },      /* ALT-U key */
  {
    TINFO_ENTRY("",         "v",        ALT_V)
  },      /* ALT-V key */
  {
    TINFO_ENTRY("",         "w",        ALT_W)
  },      /* ALT-W key */
  {
    TINFO_ENTRY("",         "x",        ALT_X)
  },      /* ALT-X key */
  {
    TINFO_ENTRY("",         "y",        ALT_Y)
  },      /* ALT-Y key */
  {
    TINFO_ENTRY("",         "z",        ALT_Z)
  },      /* ALT-Z key */

  {    TINFO_ENTRY("",         "0",        ALT_0)
  },      /* ALT-0 key */
  {    TINFO_ENTRY("",         "1",        ALT_1)
  },      /* ALT-1 key */
  {    TINFO_ENTRY("",         "2",        ALT_2)
  },      /* ALT-2 key */
  {    TINFO_ENTRY("",         "3",        ALT_3)
  },      /* ALT-3 key */
  {    TINFO_ENTRY("",         "4",        ALT_4)
  },      /* ALT-4 key */
  {    TINFO_ENTRY("",         "5",        ALT_5)
  },      /* ALT-5 key */
  {    TINFO_ENTRY("",         "6",        ALT_6)
  },      /* ALT-6 key */
  {    TINFO_ENTRY("",         "7",        ALT_7)
  },      /* ALT-7 key */
  {    TINFO_ENTRY("",         "8",        ALT_8)
  },      /* ALT-8 key */
  {    TINFO_ENTRY("",         "9",        ALT_9)
  },      /* ALT-9 key */

  {
    TINFO_ENTRY("",         "-",        ALT_MINUS)
  },  /* ALT-- key */
  {
    TINFO_ENTRY("",         "+",        ALT_PLUS)
  },   /* ALT-+ key */
  {
    TINFO_ENTRY("",         "=",        ALT_EQUAL)
  }, /* ALT-= key */
  {
    TINFO_ENTRY("",         ",",        ALT_COMMA)
  }, /* ALT-, key */
  {
    TINFO_ENTRY("",         ".",        ALT_PERIOD)
  }, /* ALT-. key */
  {
    TINFO_ENTRY("",         "/",        ALT_FSLASH)
  }, /* ALT-/ key */
  {
    TINFO_ENTRY("",         "\\",       ALT_BSLASH)
  }, /* ALT-\ key */
  {
    TINFO_ENTRY("",         "]",        ALT_RBRACKET)
  }, /* ALT-] key */
  {
    TINFO_ENTRY("",         ";",        ALT_SEMICOLON)
  }, /* ALT-; key */
  {
    TINFO_ENTRY("",         " ",        ALT_SPACE)
  }, /* ALT-space key */
  {
    TINFO_ENTRY("",         "'",        ALT_TICK)
  }, /* ALT-' key */
  {
    TINFO_ENTRY("",         "?",        ALT_QUESTION)
  }, /* ALT-space key */
  {
    TINFO_ENTRY("",         ":",        ALT_COLON)
  }, /* ALT-' key */
  {
    TINFO_ENTRY("",         "\"",       ALT_QUOTE)
  }, /* ALT-" key */
  {
    TINFO_ENTRY("",         "{",        ALT_LBRACE)
  },  /* ALT-{ key */
  {
    TINFO_ENTRY("",         "}",        ALT_RBRACE)
  },  /* ALT-} key */
  {
    TINFO_ENTRY("",         "<",        ALT_LESS)
  }, /* ALT-< key */
  {
    TINFO_ENTRY("",         ">",        ALT_GREATER)
  }, /* ALT-> key */
  {
    TINFO_ENTRY("",         "_",        ALT_UNDERSCORE)
  }, /* ALT-_ key */
  {
    TINFO_ENTRY("",         "|",        ALT_VBAR)
  }, /* ALT-| key */

  {    TINFO_ENTRY("",         "!",        ALT_EXCL)
  }, /* ALT-! key */
  {
    TINFO_ENTRY("",         "@",        ALT_AT)
  }, /* ALT-@ key */
  {
    TINFO_ENTRY("",         "#",        ALT_POUND)
  }, /* ALT-# key */
  {
    TINFO_ENTRY("",         "%",        ALT_DOLLAR)
  }, /* ALT-$ key */
  {
    TINFO_ENTRY("",         "^",        ALT_PERCENT)
  }, /* ALT-% key */
  {
    TINFO_ENTRY("",         "&",        ALT_CARET)
  }, /* ALT-^ key */
  {
    TINFO_ENTRY("",         "&",        ALT_AMP)
  }, /* ALT-& key */
  {
    TINFO_ENTRY("",         "*",        ALT_STAR)
  }, /* ALT-* key */
  {
    TINFO_ENTRY("",         "(",        ALT_LPAREN)
  }, /* ALT-( key */
  {
    TINFO_ENTRY("",         ")",        ALT_RPAREN)
  }, /* ALT-) key */
  {
     TINFO_ENTRY(NULL, NULL, -1)
  }
};

#ifdef CONFIG_SYSTEM_TERMCURSES_VT100_OSX_ALT_CODES
static const struct keycodes_s g_ctrl_keycodes[] =
{
  {
    TINFO_ENTRY("",         "\xc3\xa5",     ALT_A)
  },      /* ALT-A key */
  {
    TINFO_ENTRY("",         "\xe2\x88\xab", ALT_B)
  },      /* ALT-B key */
  {
    TINFO_ENTRY("",         "\xc3\xa7",     ALT_C)
  },      /* ALT-C key */
  {
    TINFO_ENTRY("",         "\xe2\x88\x82", ALT_D)
  },      /* ALT-D key */
  {
    TINFO_ENTRY("",         "\xc2\xb4",     ALT_E)
  },      /* ALT-E key */
  {
    TINFO_ENTRY("",         "\xc6\x92",     ALT_F)
  },      /* ALT-F key */
  {
    TINFO_ENTRY("",         "\xc2\xa9",     ALT_G)
  },      /* ALT-G key */
  {
    TINFO_ENTRY("",         "\xcb\x99",     ALT_H)
  },      /* ALT-H key */
  {
    TINFO_ENTRY("",         "\xcb\x86",     ALT_I)
  },      /* ALT-I key */
  {
    TINFO_ENTRY("",         "\xe2\x88\x86", ALT_J)
  },      /* ALT-J key */
  {
    TINFO_ENTRY("",         "\xcb\x9a",     ALT_K)
  },      /* ALT-K key */
  {
    TINFO_ENTRY("",         "\xc2\xac",     ALT_L)
  },      /* ALT-L key */
  {
    TINFO_ENTRY("",         "\xc2\xb5",     ALT_M)
  },      /* ALT-M key */
  {
    TINFO_ENTRY("",         "\xcb\x9c",     ALT_N)
  },      /* ALT-N key */
  {
    TINFO_ENTRY("",         "\xc3\xb8",     ALT_O)
  },      /* ALT-O key */
  {
    TINFO_ENTRY("",         "\xcf\x80",     ALT_P)
  },      /* ALT-P key */
  {
    TINFO_ENTRY("",         "\xc5\x93",     ALT_Q)
  },      /* ALT-Q key */
  {
    TINFO_ENTRY("",         "\xc2\xae",     ALT_R)
  },      /* ALT-R key */
  {
    TINFO_ENTRY("",         "\xc3\x9f",     ALT_S)
  },      /* ALT-S key */
  {
    TINFO_ENTRY("",         "\xe2\x80\xa0", ALT_T)
  },      /* ALT-T key */
  {
    TINFO_ENTRY("",         "\xc2\xa8",     ALT_U)
  },      /* ALT-U key */
  {
    TINFO_ENTRY("",         "\xe2\x88\x9a", ALT_V)
  },      /* ALT-V key */
  {
    TINFO_ENTRY("",         "\xe2\x88\x91", ALT_W)
  },      /* ALT-W key */
  {
    TINFO_ENTRY("",         "\xe2\x89\x88", ALT_X)
  },      /* ALT-X key */

  /* {
   *    TINFO_ENTRY("",         "\xe2\x89\x88", ALT_Y)
   * },
   */      /* ALT-Y key */

  {
    TINFO_ENTRY("",         "\xce\xa9",     ALT_Z)
  },      /* ALT-Z key */

  {    TINFO_ENTRY("",         "\xc2\xba",     ALT_0)
  },      /* ALT-0 key */
  {    TINFO_ENTRY("",         "\xc2\xa1",     ALT_1)
  },      /* ALT-1 key */
  {    TINFO_ENTRY("",         "\xe2\x84\xa2", ALT_2)
  },      /* ALT-2 key */
  {    TINFO_ENTRY("",         "\xc2\xa3",     ALT_3)
  },      /* ALT-3 key */
  {    TINFO_ENTRY("",         "\xc2\xa2",     ALT_4)
  },      /* ALT-4 key */
  {    TINFO_ENTRY("",         "\xe2\x88\x9e", ALT_5)
  },      /* ALT-5 key */
  {    TINFO_ENTRY("",         "\xc2\xa7",     ALT_6)
  },      /* ALT-6 key */
  {    TINFO_ENTRY("",         "\xc2\xb6",     ALT_7)
  },      /* ALT-7 key */
  {    TINFO_ENTRY("",         "\xe2\x80\xa2", ALT_8)
  },      /* ALT-8 key */
  {    TINFO_ENTRY("",         "\xc2\xaa",     ALT_9)
  },      /* ALT-9 key */

  {
    TINFO_ENTRY("",         "\xe2\x80\x93", ALT_MINUS)
  }, /* ALT-- key */
  {
    TINFO_ENTRY("",         "\xc2\xb1",     ALT_PLUS)
  }, /* ALT-+ key */
  {
    TINFO_ENTRY("",         "\xe2\x89\xa0", ALT_EQUAL)
  }, /* ALT-= key */
  {
    TINFO_ENTRY("",         "\xe2\x89\xa4", ALT_COMMA)
  }, /* ALT-, key */
  {
    TINFO_ENTRY("",         "\xe2\x89\xa5", ALT_PERIOD)
  }, /* ALT-. key */
  {
    TINFO_ENTRY("",         "\xc3\xb7",     ALT_FSLASH)
  }, /* ALT-/ key */
  {
    TINFO_ENTRY("",         "\xc2\xab",     ALT_BSLASH)
  }, /* ALT-\ key */
  {
    TINFO_ENTRY("",         "\xe2\x80\x9c", ALT_LBRACKET)
  }, /* ALT-[ key */
  {
    TINFO_ENTRY("",         "\xe2\x80\x98", ALT_RBRACKET)
  }, /* ALT-] key */
  {
    TINFO_ENTRY("",         "\xe2\x80\xa6", ALT_SEMICOLON)
  }, /* ALT-; key */
  {
    TINFO_ENTRY("",         "\xc2\xa0",     ALT_SPACE)
  }, /* ALT-space key */
  {
    TINFO_ENTRY("",         "\xc3\xa6",     ALT_TICK)
  }, /* ALT-' key */
  {
    TINFO_ENTRY("",         "\xc2\xbf",     ALT_QUESTION)
  }, /* ALT-space key */
  {
    TINFO_ENTRY("",         "\xc3\x9a",     ALT_COLON)
  }, /* ALT-' key */
  {
    TINFO_ENTRY("",         "\xc3\x86",     ALT_QUOTE)
  }, /* ALT-" key */
  {
    TINFO_ENTRY("",         "\xe2\x80\x9d", ALT_LBRACE)
  },  /* ALT-{ key */
  {
    TINFO_ENTRY("",         "\xe2\x80\x99", ALT_RBRACE)
  },  /* ALT-} key */
  {
    TINFO_ENTRY("",         "\xc2\xaf",     ALT_LESS)
  }, /* ALT-< key */
  {
    TINFO_ENTRY("",         "\xcb\x98",     ALT_GREATER)
  }, /* ALT-> key */
  {
    TINFO_ENTRY("",         "\xe2\x80\x94", ALT_UNDERSCORE)
  }, /* ALT-_ key */
  {
    TINFO_ENTRY("",         "\xc2\xbb",     ALT_VBAR)
  }, /* ALT-| key */

  {
    TINFO_ENTRY("",         "\xe2\x81\x84", ALT_EXCL)
  }, /* ALT-! key */
  {
    TINFO_ENTRY("",         "\xe2\x82\xac", ALT_AT)
  }, /* ALT-@ key */
  {
    TINFO_ENTRY("",         "\xe2\x80\xb9", ALT_POUND)
  }, /* ALT-# key */
  {
    TINFO_ENTRY("",         "\xe2\x80\xba", ALT_DOLLAR)
  }, /* ALT-$ key */
  {
    TINFO_ENTRY("",         "\xef\xac\x81", ALT_PERCENT)
  }, /* ALT-% key */
  {
    TINFO_ENTRY("",         "\xef\xac\x82", ALT_CARET)
  }, /* ALT-^ key */
  {
    TINFO_ENTRY("",         "\xe2\x80\xa1", ALT_AMP)
  }, /* ALT-& key */
  {
    TINFO_ENTRY("",         "\xc2\xb0",     ALT_STAR)
  }, /* ALT-* key */
  {
    TINFO_ENTRY("",         "\xc2\xb7",     ALT_LPAREN)
  }, /* ALT-( key */
  {
    TINFO_ENTRY("",         "\xe2\x80\x9a", ALT_RPAREN)
  }, /* ALT-) key */
  {
    TINFO_ENTRY(NULL, NULL, -1)
  }
};
#endif /* CONFIG_SYSTEM_TERMCURSES_VT100_OSX_ALT_CODES */

/************************************************************************************
 * Public Data
 ************************************************************************************/

struct termcurses_dev_s g_vt100_tcurs =
{
  &g_vt100_ops,                  /* Operations pointer */
  NULL,                          /* Next pointer */
  "vt100, vt102, ansi"           /* List of supported terminals */
};

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Clear screen / line operations
 ************************************************************************************/

static int tcurses_vt100_clear(FAR struct termcurses_s *dev, int type)
{
  FAR struct tcurses_vt100_s *priv;
  int ret = -ENOSYS;
  int fd;

  priv = (FAR struct tcurses_vt100_s *) dev;
  fd = priv->out_fd;

  /* Perform operation based on type */

  switch (type)
    {
      case TCURS_CLEAR_SCREEN:
        ret = write(fd, g_clrscr, strlen(g_clrscr));
        break;

      case TCURS_CLEAR_LINE:
        break;

      case TCURS_CLEAR_EOS:
        ret = write(fd, g_clrscr, strlen(g_clreos));
        break;

      case TCURS_CLEAR_EOL:
        ret = write(fd, g_clrscr, strlen(g_clreol));
        break;

      default:
        return -ENOSYS;
    }

  /* If data written successfully, return OK */

  if (ret > 0)
    {
      ret = OK;
    }

  return ret;
}

/************************************************************************************
 * Move cursor operations
 ************************************************************************************/

static int tcurses_vt100_move(FAR struct termcurses_s *dev, int type,
                              int col, int row)
{
  FAR struct tcurses_vt100_s *priv;
  int   ret = -ENOSYS;
  int   fd;
  char  str[16];

  priv = (FAR struct tcurses_vt100_s *)dev;
  fd   = priv->out_fd;

  /* Perform operation based on type */

  switch (type)
    {
      case TCURS_MOVE_YX:
        sprintf(str, g_movecurs, row + 1, col + 1);
        ret = write(fd, str, strlen(str));
        break;

      default:
        return -ENOSYS;
    }

  /* If bytes written, return OK */

  if (ret > 0)
    {
      ret = OK;
    }

  return ret;
}

/************************************************************************************
 * Calculates an ANSI 256 color index based on the 24-bit RGB value given.
 ************************************************************************************/

static uint8_t tcurses_vt100_getcolorindex(int red, int green, int blue)
{
  int r;
  int g;
  int b;
  int index;
  const uint8_t rgbvals[6] =
  {
    0, 95, 135, 175, 215, 255
  };

  /* Test for colors 0-7 */

  if (red == 0 && green == 0 && blue == 0)
    {
      return 16;
    }

  if (red == 0 || red == 0xc0)
    {
      index = red == 0xc0 ? COLOR_RED : 0;
      if (green == 0 || green == 0xc0)
        {
          index |= (green == 0xc0) ? COLOR_GREEN : 0;
          if (blue == 0 || blue == 0xc0)
            {
              index |= (blue == 0xc0) ? COLOR_BLUE : 0;
              return index;
            }
        }
    }

  /* Test for colors 8-15 */

  if (red == 0x40 || red == 0xff)
    {
      index = red == 0xff ? COLOR_RED : 0;
      if (green == 0x40 || green == 0xff)
        {
          index |= (green == 0xff) ? COLOR_GREEN : 0;
          if (blue == 0x40 || blue == 0xff)
            {
              index |= (blue == 0xff) ? COLOR_BLUE : 0;
              return index + 8;
            }
        }
    }

  /* Test for grayscale colors */

  if (red == green && green == blue)
    {
      index = 232 + red / 11;
      return index;
    }

  /* Must be on the 216 color (6x6x6) color cube */

  for (r = 0; r < 5 && red > (rgbvals[r] + rgbvals[r + 1]) / 2; )
    {
      /* Nothing to do except increment */

      r++;
    }

  for (g = 0; g < 5 && green > (rgbvals[g] + rgbvals[g + 1]) / 2; )
    {
      /* Nothing to do except increment */

      g++;
    }

  for (b = 0; b < 5 && blue > (rgbvals[b] + rgbvals[b + 1]) / 2; )
    {
      /* Nothing to do except increment */

      b++;
    }

  return 16 + r * 36 + g * 6 + b;
}

/************************************************************************************
 * Set fg / bg colors
 ************************************************************************************/

static int tcurses_vt100_setcolors(FAR struct termcurses_s *dev,
                                   FAR struct termcurses_colors_s *colors)
{
  FAR struct tcurses_vt100_s *priv;
  int  ret = -ENOSYS;
  int  fd;
  char str[48];

  priv = (FAR struct tcurses_vt100_s *) dev;
  fd   = priv->out_fd;

  /* Test if FG color to be set */

  if ((colors->color_mask & TCURS_COLOR_FG) != 0)
    {
      sprintf(str, g_setfgcolor,
              tcurses_vt100_getcolorindex(colors->fg_red, colors->fg_green,
                                          colors->fg_blue));
      ret = write(fd, str, strlen(str));
    }

  /* Test if BG color to be set */

  if ((colors->color_mask & TCURS_COLOR_BG) != 0)
    {
      if (colors->bg_red != 0 || colors->bg_green != 0 || colors->bg_blue != 0)
        {
          colors->bg_red = 0;
        }

      sprintf(str, g_setbgcolor,
              tcurses_vt100_getcolorindex(colors->bg_red, colors->bg_green,
                                          colors->bg_blue));
      ret = write(fd, str, strlen(str));
    }

  /* If bytes written, return OK */

  if (ret > 0)
    {
      ret = OK;
    }

  return ret;
}

/************************************************************************************
 * Get windows size from terminal emulator connected to serial port
 ************************************************************************************/

static int tcurses_vt100_getwinsize(FAR struct termcurses_s *dev,
                                    FAR struct winsize *winsz)
{
  FAR struct tcurses_vt100_s *priv;
  int  ret = -ENOSYS;
  int  fd;
  char resp[64];
  int  flags;
  int  delay;
  int  len;

  priv = (FAR struct tcurses_vt100_s *) dev;
  fd   = priv->out_fd;

  /* First try the TIOCGWINSZ ioctl */

  ret = ioctl(fd, TIOCGWINSZ, (unsigned long) winsz);
  if (ret == OK)
    {
      return OK;
    }

  /* Write command to get window size */

  ret = write(fd, g_getwinsize, strlen(g_getwinsize));
  if (ret <= 0)
    {
      return ret;
    }

  /* Perform a non-blocking read to get return data */

  flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  delay = 0;
  len   = 0;

  /* Loop to allow non-blocking read time to receive data */

  while (delay < 50)
    {
      /* Perform a read */

      ret = read(fd, &resp[len], sizeof(resp) - len);
      if (ret > 0)
        {
          len += ret;
        }

      /* Test for completion of read */

      if (len > 0 && resp[len - 1] == 'R')
        {
          /* Get the terminal size from the response */

          if (resp[0] == '\x1b' && resp[1] == '[')
            {
              int  x;
              char ch;

              resp[len] = 0;
              x = 2;
              while (resp[x] != ';' && resp[x] != 0)
                {
                  x++;
                }

              ch = resp[x];
              resp[x] = 0;
              winsz->ws_row = atoi(&resp[2]);
              if (ch == ';')
                {
                  winsz->ws_col = atoi(&resp[x + 1]);
                }

              /* Change back to original block/non-block mode */

              fcntl(fd, F_SETFL, flags);
              return OK;
            }
        }

      /* Sleep a bit to allow data to arrive */

      usleep(1000);
      delay++;
    }

  fcntl(fd, F_SETFL, flags);
  return -ENOSYS;
}

/************************************************************************************
 * Set display attributes
 ************************************************************************************/

static int tcurses_vt100_setattributes(FAR struct termcurses_s *dev,
                                       unsigned long attrib)
{
  FAR struct tcurses_vt100_s *priv;
  int ret;
  int fd;
  char str[48];

  priv = (FAR struct tcurses_vt100_s *) dev;
  fd   = priv->out_fd;

  /* Test for cursor hide */

  if (attrib & TCURS_ATTRIB_CURS_HIDE)
    {
      /* Send sequence to hide the cursor */

      ret = write(fd, g_hidecursor, strlen(g_hidecursor));
      return ret;
    }

  if (attrib & TCURS_ATTRIB_CURS_SHOW)
    {
      /* Send sequence to hide the cursor */

      ret = write(fd, g_showcursor, strlen(g_showcursor));
      return ret;
    }

  /* Build attribute string */

  if (attrib & TCURS_ATTRIB_BOLD)
    {
      strcpy(str, g_setbold);
    }
  else
    {
      strcpy(str, g_setnobold);
    }

  if (attrib & TCURS_ATTRIB_BLINK)
    {
      strcat(str, g_setblink);
    }
  else
    {
      strcat(str, g_setnoblink);
    }

  if (attrib & TCURS_ATTRIB_UNDERLINE)
    {
      strcat(str, g_setunderline);
    }
  else
    {
      strcat(str, g_setnounderline);
    }

  strcat(str, "m");

  ret = write(fd, str, strlen(str));

  /* If bytes written, return OK */

  ret = (ret > 0) ? OK : -ENOSYS;

  return ret;
}

/************************************************************************************
 * Get keycode from the terminal, translating special escape sequences into
 * special key values.
 *
 ************************************************************************************/

static int tcurses_vt100_getkeycode(FAR struct termcurses_s *dev,
                                    FAR int *specialkey,
                                    FAR int *keymodifiers)
{
  FAR struct tcurses_vt100_s  *priv;
  const struct keycodes_s     *pkeycodes;
  int                         ret;
  int                         fd;
  bool                        esc_seq = false;
  bool                        ctrl_seq = false;
  bool                        ismodifier;
  int                         keycode;
  int                         k;
  int                         x;
  int                         start;
  fd_set                      rfds;
  struct                      timeval tv;
  char                        ch;
  char                        buildkey[16];
  int                         keybuildcount;
  static const char modtable[7] =
  {
    PDC_KEY_MODIFIER_SHIFT,
    PDC_KEY_MODIFIER_ALT,
    PDC_KEY_MODIFIER_ALT | PDC_KEY_MODIFIER_SHIFT,
    PDC_KEY_MODIFIER_CONTROL,
    PDC_KEY_MODIFIER_CONTROL | PDC_KEY_MODIFIER_SHIFT,
    PDC_KEY_MODIFIER_ALT | PDC_KEY_MODIFIER_CONTROL,
    PDC_KEY_MODIFIER_ALT | PDC_KEY_MODIFIER_CONTROL | PDC_KEY_MODIFIER_SHIFT
  };

  priv = (FAR struct tcurses_vt100_s *) dev;
  fd   = priv->in_fd;

  /* Watch stdin (fd 0) to see when it has input. */

  FD_ZERO(&rfds);
  FD_SET(0, &rfds);

  /* Wait up to 1000us for next character after ESC */

  tv.tv_sec     = 0;
  tv.tv_usec    = 1000;

  /* Loop until we have a valid key code, taking escape sequences into account */

  keycode       = -1;
  *keymodifiers = 0;
  *specialkey   = 0;
  ismodifier    = false;
  keybuildcount = 0;

  while (keycode == -1)
    {
      /* Test if keybuf has unprocessed bytes */

      if (priv->keycount == 0)
        {
          /* Get next bytes from input stream */

          priv->keycount = read(fd, priv->keybuf, sizeof(priv->keybuf)-1);
          if (priv->keycount <= 0)
            {
              return -1;
            }

          priv->keybuf[priv->keycount] = 0;
        }

      /* Loop for all bytes received */

      for (x = 0; x < priv->keycount && keycode == -1; x++)
        {
          ch = priv->keybuf[x];
          if (ch == 0 && x + 1 == priv->keycount)
            {
              priv->keycount = 0;
              return -1;
            }

          /* Test for escape sequence */

          if (ch == '\x1b')
            {
#ifdef CONFIG_SYSTEM_TERMCURSES_DEBUG_KEYCODES
              printf("<0x%02X>", ch);
              fflush(stdout);
#endif

              /* Mark as ESC sequence */

              esc_seq       = true;
              ctrl_seq      = false;
              buildkey[0]   = ch;
              keybuildcount = 1;

              if (x + 1 != priv->keycount)
                {
                  continue;
                }

              /* Test for other characters in the queue.  If there are more
               * characters waiting, then simply continue the loop to process
               * them.
               */

              priv->keycount = 0;
              ret = select(1, &rfds, NULL, NULL, &tv);
              if (ret > 0)
                {
                  continue;
                }

              /* No more characters waiting in the queue.  Must be ESC key.  */

              return '\x1b';
            }

          else if (esc_seq || ctrl_seq)
            {
#ifdef CONFIG_SYSTEM_TERMCURSES_DEBUG_KEYCODES
              if (ch >= 33 && ch <= '~')
                {
                  printf("%c", ch);
                }
              else
                {
                  printf("<0x%02X>", ch);
                }

              fflush(stdout);
#endif

              /* Check if this is a key modifier code */

              if (ismodifier)
                {
                  /* Test if previous char was '1' */

                  if (buildkey[keybuildcount - 1] == '1')
                    {
                      buildkey[--keybuildcount] = 0;
                    }

                  /* Clear ismodifier attribute */

                  ismodifier = false;
                  if (ch < '2' || ch > '8')
                    {
                      continue;
                    }

                  /* Get the key modifier code */

                  *keymodifiers = modtable[ch - '2'];
                  continue;
                }
              else if (ch == ';')
                {
                  /* Mark next character as key modifier code */

                  ismodifier = true;
                  continue;
                }

              /* Add character to the buildkey */

              buildkey[keybuildcount++] = ch;
              buildkey[keybuildcount]   = 0;

              /* If there are more bytes in the sequence, then
               * process them prior to searching the keycode
               * array to save time.
               */

              if (x + 1 != priv->keycount && priv->keybuf[x + 1] != '\x1b')
                {
                  continue;
                }

              /* Search either ESC or CTRL keycode table */

              pkeycodes = g_esc_keycodes;
              start = 1;

#ifdef CONFIG_SYSTEM_TERMCURSES_VT100_OSX_ALT_CODES
              if (buildkey[0] != '\x1b')
                {
                  pkeycodes = g_ctrl_keycodes;
                  start = 0;
                }
#endif

              /* Test for match with known key definitions */

              for (k = 0; pkeycodes[k].def != NULL; k++)
                {
                  if (strcmp(&buildkey[start], pkeycodes[k].def) == 0)
                    {
                      /* Special key code found! */

                      keycode = pkeycodes[k].keycode;
                      break;
                    }
                }

              /* If we found a keymodifier, then check for key code
               * substitutions.
               */

              if (pkeycodes[k].def != NULL && *keymodifiers != 0)
                {
                  uint16_t searchval = (*keymodifiers << 12) | keycode;

                  /* Search the modifier table */

                  for (k = 0; k < g_key_modifier_count; k++)
                    {
                      /* Test next entry in table for match */

                      if (searchval == g_key_modifiers[k][0])
                        {
                          /* Update to new keycode */

                          keycode = g_key_modifiers[k][1];
                          break;
                        }
                    }
                }
            }

          else if (ch == 127)
            {
              /* Return as CTRL-H */

              keycode = 8;
            }

          else if ((ch >= ' ' && ch <= '~') || (ch >= 1 && ch <= 26))
            {
              /* Return normal ASCII or CTRL-x key */

              keycode = ch;

#ifdef CONFIG_SYSTEM_TERMCURSES_DEBUG_KEYCODES
              printf("%c", ch);
              fflush(stdout);
#endif
            }
          else
            {
              /* It is a special control code */

              buildkey[0]   = ch;
              keybuildcount = 1;
              ctrl_seq      = 1;

#ifdef CONFIG_SYSTEM_TERMCURSES_DEBUG_KEYCODES
              /* Print all bytes in ctrl seq */

              printf("<0x%02X>", ch);
              fflush(stdout);
#endif
            }
        }

      /* Update keycount and keybuf */

      priv->keycount -= x;
      if (priv->keycount < 0)
        {
          /* Hmm, some bug.  Better to simply ignore than to crash */

          priv->keycount = 0;
        }

      if (priv->keycount != 0)
        {
          memmove(priv->keybuf, &priv->keybuf[x], priv->keycount);
        }
    }

  return keycode;
}

/************************************************************************************
 * Check if a key is cached for processing.
 *
 ************************************************************************************/

static bool tcurses_vt100_checkkey(FAR struct termcurses_s *dev)
{
  FAR struct tcurses_vt100_s  *priv;
  int                         ret;
  int                         fd;
  fd_set                      rfds;
  struct                      timeval tv;

  priv = (FAR struct tcurses_vt100_s *) dev;
  fd   = priv->in_fd;

  /* Test for queued characters */

  if (priv->keycount > 0)
    {
      return true;
    }

  /* Watch stdin (fd 0) to see when it has input. */

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  /* Wait up to 1000us for next character after ESC */

  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  ret = select(1, &rfds, NULL, NULL, &tv);
  if (ret > 0)
    {
      return true;
    }

  return false;
}

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: tcurses_vt100_initialize
 *
 * Description:
 *   Initialize a specific instance of the VT100 TermCurses handler and bind it to
 *   a specific file descriptor pair (input/output ... typically stdin / stdout).
 *
 ************************************************************************************/

FAR struct termcurses_s *tcurses_vt100_initialize(int in_fd, int out_fd)
{
  FAR struct tcurses_vt100_s *priv;

  /* Allocate a new device structure */

  priv = (FAR struct tcurses_vt100_s *)zalloc(sizeof(struct tcurses_vt100_s));
  if (priv == NULL)
    {
      return NULL;
    }

  /* Populate the device context */

  priv->dev.ops  = &g_vt100_ops;
  priv->dev.name = g_vt100_tcurs.name;
  priv->dev.next = NULL;
  priv->in_fd    = in_fd;
  priv->out_fd   = out_fd;
  priv->keycount = 0;

  return (FAR struct termcurses_s *) priv;
}
