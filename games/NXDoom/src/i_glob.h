/****************************************************************************
 * apps/games/NXDoom/src/i_glob.h
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 2018 Simon Howard
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
 *  System specific file globbing interface.
 *
 ****************************************************************************/

#ifndef __I_GLOB__
#define __I_GLOB__

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define GLOB_FLAG_NOCASE 0x01
#define GLOB_FLAG_SORTED 0x02

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct glob_s glob_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: i_start_glob
 *
 * Description:
 *  Start reading a list of file paths from the given directory which match
 *  the given glob pattern. i_end_glob() must be called on completion.
 *
 ****************************************************************************/

glob_t *i_start_glob(const char *directory, const char *glob, int flags);

/****************************************************************************
 * Name: i_start_multi_glob
 *
 * Description:
 *  Same as i_start_glob but multiple glob patterns can be provided. The list
 *  of patterns must be terminated with NULL.
 *
 ****************************************************************************/

glob_t *i_start_multi_glob(const char *directory, int flags,
                           const char *glob, ...);

/****************************************************************************
 * Name: i_end_glob
 *
 * Description:
 *  Finish reading file list.
 *
 ****************************************************************************/

void i_end_glob(glob_t *glob);

/****************************************************************************
 * Name: i_next_glob
 *
 * Description:
 *  Read the name of the next globbed filename. NULL is returned if there
 *  are no more found.
 *
 ****************************************************************************/

const char *i_next_glob(glob_t *glob);

#endif /* __I_GLOB__ */
