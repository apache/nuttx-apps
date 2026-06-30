/****************************************************************************
 * apps/games/NXDoom/src/m_argv.h
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
 ****************************************************************************/

#ifndef __M_ARGV__
#define __M_ARGV__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomtype.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int myargc;
extern char **myargv;

extern char *exedir;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void m_set_exe_dir(void);

/****************************************************************************
 * Name: m_check_parm
 *
 * Description:
 *  Returns the position of the given parameter in the arg list.
 *
 * Returns:
 *  The position of the given parameter in the arg list (0 if not found).
 *
 ****************************************************************************/

int m_check_parm(const char *check);

/****************************************************************************
 * Name: m_check_parm_with_args
 *
 * Description:
 *  Same as m_check_parm, but checks that num_args arguments are available
 *  following the specified argument.
 *
 ****************************************************************************/

int m_check_parm_with_args(const char *check, int num_args);

void m_find_response_file(void);
void m_add_loose_files(void);

/****************************************************************************
 * Name: m_parm_exists
 *
 * Description:
 *  Parameter has been specified?
 *
 ****************************************************************************/

boolean m_parm_exists(const char *check);

/****************************************************************************
 * Name: m_get_executable_name
 *
 * Description:
 *  Get name of executable used to run this program.
 *
 ****************************************************************************/

const char *m_get_executable_name(void);

#endif
