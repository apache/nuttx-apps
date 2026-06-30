/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_sdl.h
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
 * Text mode emulation in SDL
 *
 ****************************************************************************/

#ifndef TXT_SDL_H
#define TXT_SDL_H

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Set a callback function to call in the SDL event loop.  Useful for
 * intercepting events.  Pass callback=NULL to clear an existing
 * callback function.
 * user_data is a void pointer to be passed to the callback function.
 */

void txt_sdl_set_event_callback(void *user_data);

#endif /* TXT_SDL_H */
