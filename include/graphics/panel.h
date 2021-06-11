/****************************************************************************
 * apps/include/graphics/panel.h
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

/* Public Domain Curses -- Panels for PDCurses
 * $Id: panel.h,v 1.19 2008/07/13 16:08:16 wmcbrine Exp $
 */

#ifndef __APPS_INCLUDE_GRAPHICS_PANEL_H
#define __APPS_INCLUDE_GRAPHICS_PANEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "graphics/curses.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

#if defined(__cplusplus)
extern "C"
{
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

typedef struct panelobs
{
  struct panelobs *above;
  struct panel *pan;
} PANELOBS;

typedef struct panel
{
  WINDOW *win;
  int wstarty;
  int wendy;
  int wstartx;
  int wendx;
  struct panel *below;
  struct panel *above;
  const void *user;
  struct panelobs *obscure;
} PANEL;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int     bottom_panel(PANEL *pan);
int     del_panel(PANEL *pan);
int     hide_panel(PANEL *pan);
int     move_panel(PANEL *pan, int starty, int startx);
PANEL  *new_panel(WINDOW *win);
PANEL  *panel_above(const PANEL *pan);
PANEL  *panel_below(const PANEL *pan);
int     panel_hidden(const PANEL *pan);
const void *panel_userptr(const PANEL *pan);
WINDOW *panel_window(const PANEL *pan);
int     replace_panel(PANEL *pan, WINDOW *win);
int     set_panel_userptr(PANEL *pan, const void *uptr);
int     show_panel(PANEL *pan);
int     top_panel(PANEL *pan);
void    update_panels(void);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_GRAPHICS_PANEL_H */
