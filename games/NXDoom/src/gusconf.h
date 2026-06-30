/****************************************************************************
 * apps/games/NXDoom/src/gusconf.h
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
 * DESCRIPTION:
 *   GUS emulation code.
 *
 ****************************************************************************/

#ifndef __GUSCONF_H__
#define __GUSCONF_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern char *gus_patch_path;
extern int gus_ram_kb;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

boolean gus_write_config(char *path);

#endif /* __GUSCONF_H__ */
