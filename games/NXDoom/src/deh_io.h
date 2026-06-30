/****************************************************************************
 * apps/games/NXDoom/src/deh_io.h
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
 * Dehacked I/O code (does all reads from dehacked files)
 *
 ****************************************************************************/

#ifndef DEH_IO_H
#define DEH_IO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "deh_defs.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

deh_context_t *deh_open_file(const char *filename);
deh_context_t *deh_open_lump(int lumpnum);
void deh_close_file(deh_context_t *context);
int deh_get_char(deh_context_t *context);
char *deh_read_line(deh_context_t *context, boolean extended);
void deh_error(deh_context_t *context, const char *msg, ...)
    PRINTF_ATTR(2, 3);
void deh_warning(deh_context_t *context, const char *msg, ...)
    PRINTF_ATTR(2, 3);
boolean deh_had_error(deh_context_t *context);

#endif /* DEH_IO_H */
