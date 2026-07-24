/****************************************************************************
 * apps/games/NXDoom/src/i_system.h
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
 *  System specific interface stuff.
 *
 ****************************************************************************/

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "d_ticcmd.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*atexit_func_t)(void);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Called by DoomMain. */

void i_init(void);

/* Called by startup code to get the amount of memory to malloc for the zone
 * management.
 */

byte *i_zone_base(int *size);

boolean i_console_stdout(void);

/* Asynchronous interrupt functions should maintain private queues
 * that are read by the synchronous functions
 * to be converted into events.
 */

#if 0 /* Unused */

/* Either returns a null ticcmd, or calls a loadable driver to build it.
 * This ticcmd will then be modified by the gameloop for normal input.
 */

ticcmd_t *i_base_ticcmd(void);
#endif

/* Called by m_responder when quit is selected.
 * Clean exit, displays sell blurb.
 */

void i_quit(void) NORETURN;

/* Installs a SIGTERM handler that only sets a flag (async-signal-safe) -
 * the actual i_quit() cleanup (unmapping the framebuffer, closing fds)
 * runs later from i_poll_quit_signal(), called once per tic from a known
 * safe point in the main loop rather than from the signal handler itself,
 * so a supervisor process (nxstore) requesting an exit can never land in
 * the middle of a frame's worth of direct framebuffer/heap access.
 */

void i_install_quit_signal(void);

/* Checks the flag set by the SIGTERM handler and calls i_quit() if it's
 * set.  Must only be called from a safe point in the main loop - see
 * i_install_quit_signal().
 */

void i_poll_quit_signal(void);

void i_error(const char *error, ...) NORETURN PRINTF_ATTR(1, 2);

void i_tactile(int on, int off, int total);

void *i_realloc(void *ptr, size_t size);

boolean i_get_memory_value(unsigned int offset, void *value, int size);

/* Schedule a function to be called when the program exits.
 * If run_if_error is true, the function is called if the exit
 * is due to an error (i_error)
 */

void i_at_exit(atexit_func_t func, boolean run_if_error);

/* Add all system-specific config file variable bindings. */

void i_bind_variables(void);

/* Print startup banner copyright message. */

void i_print_startup_banner(const char *gamedescription);

/* Print a centered text banner displaying the given string. */

void i_print_banner(const char *text);

/* Print a dividing line for startup banners. */

void i_print_divider(void);

#endif /* __I_SYSTEM__ */
