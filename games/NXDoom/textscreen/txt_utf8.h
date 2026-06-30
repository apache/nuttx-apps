/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_utf8.h
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
 ****************************************************************************/

#ifndef TXT_UTF8_H
#define TXT_UTF8_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

char *txt_encode_utf8(char *p, unsigned int c);
unsigned int txt_decode_utf8(const char **ptr);
unsigned int txt_utf8_strlen(const char *s);
char *txt_utf8_skip_chars(const char *s, unsigned int n);

#endif /* TXT_UTF8_H */
