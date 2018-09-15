/****************************************************************************
 * apps/include/graphics/panel.h
 * Public Domain Curses -- Panels for PDCurses
 * $Id: panel.h,v 1.19 2008/07/13 16:08:16 wmcbrine Exp $
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_INCLUDE_GRAPHICS_PANEL_H
#define __APPS_INCLUDE_GRAPHICS_PANEL_H 1

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
