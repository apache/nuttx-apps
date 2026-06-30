/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_desktop.h
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
 ****************************************************************************/

#ifndef TXT_DESKTOP_H
#define TXT_DESKTOP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_window.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*txt_idle_callback_f)(void *user_data);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void txt_add_desktop_window(txt_window_t *win);
void txt_remove_desktop_window(txt_window_t *win);
void txt_draw_desktop(void);
void txt_dispatch_events(void);
void txt_draw_window(txt_window_t *window);
void txt_set_window_focus(txt_window_t *window, int focused);
int txt_window_keypress(txt_window_t *window, int c);

/**
 * Set the title displayed at the top of the screen.
 *
 * @param title         The title to display (UTF-8 format).
 */

void txt_set_desktop_title(const char *title);

/**
 * Start the main event loop.  At least one window must have been
 * opened prior to running this function.  When no windows are left
 * open, the event loop exits.
 *
 * It is possible to trigger an exit from this function using the
 * @ref TXT_ExitMainLoop function.
 */

void txt_gui_mainloop(void);

/**
 * Get the top window on the desktop that is currently receiving
 * inputs.
 *
 * @return    The active window, or NULL if no windows are present.
 */

txt_window_t *txt_get_active_window(void);

/**
 * Set a callback function to be invoked periodically by the main
 * loop code.
 *
 * @param callback      The callback to invoke, or NULL to cancel
 *                      an existing callback.
 * @param user_data     Extra data to pass to the callback.
 * @param period        Maximum time between invoking each callback.
 *                      eg. a value of 200 will cause the callback
 *                      to be invoked at least once every 200ms.
 */

void txt_set_periodic_callback(txt_idle_callback_f callback, void *user_data,
                             unsigned int period);
/**
 * Lower the z-position of the given window relative to other windows.
 *
 * @param window        The window to make inactive.
 * @return              Non-zero if the window was lowered successfully,
 *                      or zero if the window could not be lowered further.
 */

int txt_lower_window(txt_window_t *window);

#endif /* TXT_DESKTOP_H */
