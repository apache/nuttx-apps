/****************************************************************************
 * apps/games/NXDoom/src/m_config.h
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
 *  Configuration file interface.
 *
 ****************************************************************************/

#ifndef __M_CONFIG__
#define __M_CONFIG__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char *configdir;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void m_load_defaults(void);
void m_save_defaults(void);
void m_save_defaults_alternate(const char *main, const char *extra);
void m_set_config_dir(const char *dir);
void m_set_music_pack_dir(void);
void m_bind_int_variable(const char *name, int *variable);
void m_bind_float_variable(const char *name, float *variable);
void m_bind_string_variable(const char *name, char **variable);
boolean m_set_variable(const char *name, const char *value);
const char *m_get_string_variable(const char *name);
void m_set_config_filenames(const char *main_config,
                            const char *extra_config);
char *m_get_save_game_dir(const char *iwadname);
char *m_get_autoload_dir(const char *iwadname);

#endif /* __M_CONFIG__ */
