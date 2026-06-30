/****************************************************************************
 * apps/games/NXDoom/src/i_video.h
 *
 * SPDX-License-Identifier: GPLv2
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
 *  System specific interface stuff.
 *
 ****************************************************************************/

#ifndef __I_VIDEO__
#define __I_VIDEO__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Screen width and height. */

#define SCREENWIDTH 320
#define SCREENHEIGHT 200

/* Screen height used when aspect_ratio_correct=true. */

#define SCREENHEIGHT_4_3 240

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef boolean (*grabmouse_callback_t)(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern char *video_driver;
extern boolean screenvisible;

extern int vanilla_keyboard_mapping;
extern boolean screensaver_mode;
extern int usegamma;
extern pixel_t *i_video_buffer;

extern int screen_width;
extern int screen_height;
extern int fullscreen;
extern int aspect_ratio_correct;
extern int integer_scaling;
extern int smooth_pixel_scaling;
extern int vga_porch_flash;
extern int force_software_renderer;

extern int png_screenshots;

extern char *window_position;

/* Joystic/gamepad hysteresis */

extern unsigned int joywait;

extern int usemouse;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: i_init_graphics
 *
 * Description:
 *  Called by d_doom_main, determines the hardware configuration and sets up
 *  the video mode.
 *
 ****************************************************************************/

void i_init_graphics(void);

void i_graphics_check_commandline(void);

void i_shutdown_graphics(void);

/****************************************************************************
 * Name: i_set_palette
 *
 * Description:
 *  Takes full 8 bit values.
 *
 ****************************************************************************/

void i_set_palette(byte *palette);

int i_get_palette_index(int r, int g, int b);

void i_update_no_blit(void);
void i_finish_update(void);

void i_read_screen(pixel_t *scr);

void i_set_window_title(const char *title);

void i_check_is_screensaver(void);
void i_set_grab_mouse_callback(grabmouse_callback_t func);

void i_display_fps_dots(boolean dots_on);
void i_bind_video_variables(void);

void i_init_window_title(void);

/****************************************************************************
 * Name: i_start_frame
 *
 * Description:
 *  Called before processing any tics in a frame (just after displaying a
 *  frame). Time consuming synchronous operations are performed here
 *  (joystick reading).
 *
 ****************************************************************************/

void i_start_frame(void);

/****************************************************************************
 * Name: i_start_tic
 *
 * Description:
 *  Called before processing each tic in a frame. Quick synchronous
 *  operations are performed here.
 *
 ****************************************************************************/

void i_start_tic(void);

void i_get_window_position(int *x, int *y, int w, int h);

#endif /* __I_VIDEO__ */
