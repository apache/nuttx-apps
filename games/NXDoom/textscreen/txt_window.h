/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_window.h
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

#ifndef TXT_WINDOW_H
#define TXT_WINDOW_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "txt_table.h"
#include "txt_widget.h"

#include "txt_window_action.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Callback function for window key presses */

typedef int (*txt_window_keypress_t)(txt_window_t *window, int key,
                                 void *user_data);
typedef int (*txt_window_mouse_press_t)(txt_window_t *window, int x, int y,
                                        int b, void *user_data);

struct txt_window_s
{
  /* Base class: all windows are tables with one column. */

  txt_table_t table;

  /* Window title */

  char *title;

  /* Screen coordinates of the window */

  txt_vert_align_t vert_align;
  txt_horiz_align_t horiz_align;
  int x;
  int y;

  /* Actions that appear in the box at the bottom of the window */

  txt_widget_t *actions[3];

  /* Callback functions to invoke when keys/mouse buttons are pressed */

  txt_window_keypress_t key_listener;
  void *key_listener_data;
  txt_window_mouse_press_t mouse_listener;
  void *mouse_listener_data;

  /* These are set automatically when the window is drawn */

  int window_x;
  int window_y;
  unsigned int window_w;
  unsigned int window_h;

  /* URL of a webpage with help about this window. If set, a help key
   * indicator is shown while this window is active.
   */

  const char *help_url;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Open a new window.
 *
 * @param title        Title to display in the titlebar of the new window
 *                     (UTF-8 format).
 * @return             Pointer to a new @ref txt_window_t structure
 *                     representing the new window.
 */

txt_window_t *txt_new_window(const char *title);

/**
 * Close a window.
 *
 * @param window       Tine window to close.
 */

void txt_close_window(txt_window_t *window);

/**
 * Set the position of a window on the screen.
 *
 * The position is specified as a pair of x, y, coordinates on the
 * screen. These specify the position of a particular point on the
 * window. The following are some examples:
 *
 * <code>
 *   // Centered on the screen:
 *
 *   txt_set_window_position(window, TXT_HORIZ_CENTER, TXT_VERT_CENTER,
 *                                 TXT_SCREEN_W / 2, TXT_SCREEN_H / 2);
 *
 *   // Horizontally centered, with top of the window on line 6:
 *
 *   txt_set_window_position(window, TXT_HORIZ_CENTER, TXT_VERT_TOP,
 *                                 TXT_SCREEN_W / 2, 6);
 *
 *   // Top-left of window at 20, 6:
 *
 *   txt_set_window_position(window, TXT_HORIZ_LEFT, TXT_VERT_TOP, 20, 6);
 * </code>
 *
 * @param window       The window.
 * @param horiz_align  Horizontal location on the window for the X
 *                     position.
 * @param vert_align   Vertical location on the window for the Y position.
 * @param x            X coordinate (horizontal axis) for window position.
 * @param y            Y coordinate (vertical axis) for window position.
 */

void txt_set_window_position(txt_window_t *window,
                             txt_horiz_align_t horiz_align,
                             txt_vert_align_t vert_align, int x, int y);

/**
 * Set a window action for a given window.
 *
 * Each window can have up to three window actions, which provide
 * keyboard shortcuts that can be used within a given window.
 *
 * @param window      The window.
 * @param position    The window action slot to set (left, center or right).
 * @param action      The window action widget.  If this is NULL, any
 *                    current window action in the given slot is removed.
 */

void txt_set_window_action(txt_window_t *window, txt_horiz_align_t position,
                           TXT_UNCAST_ARG(action));

/**
 * Set a callback function to be invoked whenever a key is pressed within
 * a window.
 *
 * @param window        The window.
 * @param key_listener  Callback function.
 * @param user_data     User-specified pointer to pass to the callback
 *                      function.
 */

void txt_set_key_listener(txt_window_t *window,
                          txt_window_keypress_t key_listener,
                          void *user_data);

/**
 * Set a callback function to be invoked whenever a mouse button is pressed
 * within a window.
 *
 * @param window          The window.
 * @param mouse_listener  Callback function.
 * @param user_data       User-specified pointer to pass to the callback
 *                        function.
 */

void txt_set_mouse_listener(txt_window_t *window,
                            txt_window_mouse_press_t mouse_listener,
                            void *user_data);

/**
 * Open a window displaying a message.
 *
 * @param title           Title of the window (UTF-8 format).
 * @param message         The message to display in the window
 *                        (UTF-8 format).
 * @return                The new window.
 */

txt_window_t *txt_message_box(const char *title, const char *message, ...);

/**
 * Set the help URL for the given window.
 *
 * @param window          The window.
 * @param help_url        String containing URL of the help page for this
 *                        window, or NULL to set no help for this window.
 */

void txt_set_window_help_url(txt_window_t *window, const char *help_url);

/**
 * Open the help URL for the given window, if one is set.
 *
 * @param window          The window.
 */

void txt_open_window_help_url(txt_window_t *window);

#endif /* TXT_WINDOW_H */
