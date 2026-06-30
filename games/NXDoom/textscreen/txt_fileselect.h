/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_fileselect.h
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
 * Routines for selecting files, and the txt_fileselect_t widget.
 *
 ****************************************************************************/

#ifndef TXT_FILESELECT_H
#define TXT_FILESELECT_H

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * File selection widget.
 *
 * A file selection widget resembles an input box (@ref txt_inputbox_t)
 * but opens a file selector dialog box when clicked.
 */

typedef struct txt_fileselect_s txt_fileselect_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/**
 * Special value to use for 'extensions' that selects a directory
 * instead of a file.
 */

extern const char *TXT_DIRECTORY[];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Create a new txt_fileselect_t widget.
 *
 * @param variable    Pointer to a char * variable in which the selected
 *                    file should be stored (UTF-8 format).
 * @param size        Width of the file selector widget in characters.
 * @param prompt      Pointer to a string containing a prompt to display
 *                    in the file selection window.
 * @param extensions  NULL-terminated list of filename extensions that
 *                    can be used for this widget, or @ref TXT_DIRECTORY
 *                    to select directories.
 */

txt_fileselect_t *txt_new_file_selector(char **variable, int size,
                                        const char *prompt,
                                        const char **extensions);

#endif /* TXT_FILESELECT_H */
