/****************************************************************************
 * apps/games/NXDoom/src/i_input.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
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
 * DESCRIPTION:
 *   System-specific keyboard/mouse input.
 *
 ****************************************************************************/

#ifndef __I_INPUT__
#define __I_INPUT__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

#ifdef CONFIG_GAMES_NXDOOM_KEYBOARD
#include <nuttx/input/keyboard.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_MOUSE_BUTTONS 8

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern float mouse_acceleration;
extern int mouse_threshold;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void i_bind_input_variables(void);
void i_read_mouse(void);

/* i_start_text_input begins text input, activating the on-screen keyboard
 * (if one is used). The caller indicates that any entered text will be
 * displayed in the rectangle given by the provided set of coordinates.
 */

void i_start_text_input(int x1, int y1, int x2, int y2);

/* i_stop_text_input finishes text input, deactivating the on-screen keyboard
 * (if one is used).
 */

void i_stop_text_input(void);

void i_handle_keyboard_event(struct keyboard_event_s *kevent);
void i_handle_mouse_event(void);

#ifdef CONFIG_GAMES_NXDOOM_KEYBOARD
int get_kbd_event(struct keyboard_event_s *sample);
#endif

#endif /* __I_INPUT__ */
