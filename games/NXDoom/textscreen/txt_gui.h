/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_gui.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Text mode emulation in SDL
 *
 ****************************************************************************/

#ifndef TXT_GUI_H
#define TXT_GUI_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TXT_INACTIVE_WINDOW_BACKGROUND TXT_COLOR_BLACK
#define TXT_ACTIVE_WINDOW_BACKGROUND TXT_COLOR_BLUE
#define TXT_HOVER_BACKGROUND TXT_COLOR_CYAN

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void txt_draw_window_frame(const char *title, int x, int y, int w, int h);
void txt_draw_separator(int x, int y, int w);
void txt_draw_code_page_string(const char *s);
void txt_draw_string(const char *s);

void txt_draw_horiz_scrollbar(int x, int y, int w, int cursor, int range);
void txt_draw_vert_scrollbar(int x, int y, int h, int cursor, int range);

void txt_init_clip_area(void);
void txt_push_clip_area(int x1, int x2, int y1, int y2);
void txt_pop_clip_area(void);

#endif /* TXT_GUI_H */
