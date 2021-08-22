/****************************************************************************
 * apps/include/system/termcurses.h
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

#ifndef __APPS_INCLUDE_SYSTEM_TERMCURSES_H
#define __APPS_INCLUDE_SYSTEM_TERMCURSES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/fs/fs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TCURS_CLEAR_SCREEN      1
#define TCURS_CLEAR_LINE        2
#define TCURS_CLEAR_EOS         3
#define TCURS_CLEAR_EOL         4

#define TCURS_MOVE_UP           1
#define TCURS_MOVE_DOWN         2
#define TCURS_MOVE_LEFT         3
#define TCURS_MOVE_RIGHT        4
#define TCURS_MOVE_YX           5
#define TCURS_MOVE_TO_ROW       6
#define TCURS_MOVE_TO_COL       7

#define TCURS_COLOR_FG          0x01
#define TCURS_COLOR_BG          0x02

#define TCURS_ATTRIB_BLINK      0x0001
#define TCURS_ATTRIB_BOLD       0x0002
#define TCURS_ATTRIB_UNDERLINE  0x0004
#define TCURS_ATTRIB_INVIS      0x0008
#define TCURS_ATTRIB_CURS_HIDE  0x0010
#define TCURS_ATTRIB_CURS_SHOW  0x0020

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Used with TTY ioctls */

struct tcurses_ioctl_s
{
  uint16_t tc_row;                     /* Cursor row */
  uint16_t tc_col;                     /* Cursor col */
};

struct termcurses_dev_s;
struct termcurses_s;
struct termcurses_colors_s;

struct termcurses_ops_s
{
  /* Initialize a new instance bound to a file */

  CODE FAR struct termcurses_s * (*init)(int in_fd, int out_fd);

  /* Clear operations (screen, line, EOS, EOL) */

  CODE int (*clear)(FAR struct termcurses_s *dev, int type);

  /* Cursor Move operations */

  CODE int (*move)(FAR struct termcurses_s *dev, int type, int col, int row);

  /* Get terminal size in row/col dimensions */

  CODE int (*getwinsize)(FAR struct termcurses_s *dev,
                         FAR struct winsize *winsz);

  /* Set fg/bg colors */

  CODE int (*setcolors)(FAR struct termcurses_s *dev,
                        FAR struct termcurses_colors_s *colors);

  /* Set display attributes  */

  CODE int (*setattrib)(FAR struct termcurses_s *dev,
                        unsigned long attributes);

  /* Get a keycode value */

  CODE int (*getkeycode)(FAR struct termcurses_s *dev,
                         int *specialkey,
                         int *keymodifers);

  /* Check for cached keycode value */

  CODE bool (*checkkey)(FAR struct termcurses_s *dev);

  /* Terminate  */

  CODE int (*terminate)(FAR struct termcurses_s *dev);
};

struct termcurses_dev_s
{
  FAR const struct termcurses_ops_s    *ops;
  FAR struct termcurses_dev_s          *next;
  FAR const char                       *name;
};

struct termcurses_s
{
  FAR struct termcurses_dev_s          *dev;
  FAR struct file                      *filep;
};

struct termcurses_colors_s
{
  uint8_t   color_mask;
  uint8_t   fg_red;
  uint8_t   fg_green;
  uint8_t   fg_blue;
  uint8_t   bg_red;
  uint8_t   bg_green;
  uint8_t   bg_blue;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * termcurses_initterm:
 *
 * Finds a termcurses_s device based on terminal type string and initialize
 * a new context for device control.
 *
 ****************************************************************************/

int termcurses_initterm(FAR const char *term_type, int in_fd, int out_fd,
                        FAR struct termcurses_s **term);

/****************************************************************************
 * Name: termcurses_deinitterm
 *
 * Description:
 *    Free all space for the termcurses terminal and perform any specific
 *    de-initialization tasks.
 *
 ****************************************************************************/

int termcurses_deinitterm(FAR struct termcurses_s *term);

/****************************************************************************
 * Name: termcurses_moveyx
 *
 * Description:
 *   Configure output text to render in the specified fg/bg colors.
 *
 ****************************************************************************/

int termcurses_moveyx(FAR struct termcurses_s *term, int row, int col);

/****************************************************************************
 * Name: termcurses_setattribute
 *
 * Description:
 *   Configure output text to render in the specified fg/bg colors.
 *
 ****************************************************************************/

int termcurses_setattribute(FAR struct termcurses_s *term,
                            unsigned long attrib);

/****************************************************************************
 * Name: termcurses_setcolors
 *
 * Description:
 *   Configure output text to render in the specified fg/bg colors.
 *
 ****************************************************************************/

int termcurses_setcolors(FAR struct termcurses_s *term,
                         FAR struct termcurses_colors_s *colors);

/****************************************************************************
 * Name: termcurses_getwinsize
 *
 * Description:
 *   Determines the size of the terminal screen in terms of character rows
 *   and cols.
 *
 ****************************************************************************/

int termcurses_getwinsize(FAR struct termcurses_s *term,
                          FAR struct winsize *winsz);

/****************************************************************************
 * Name: termcurses_getkeycode
 *
 * Description:
 *   Get a translated key code from the terminal input.
 *
 ****************************************************************************/

int termcurses_getkeycode(FAR struct termcurses_s *term, FAR int *specialkey,
                          FAR int *keymodifiers);

/****************************************************************************
 * Name: termcurses_checkkey
 *
 * Description:
 *   Check if there is a key waiting to be processed.
 *
 ****************************************************************************/

bool termcurses_checkkey(FAR struct termcurses_s *term);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_TERMCURSES_H */
