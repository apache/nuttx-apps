/****************************************************************************
 * apps/games/NXDoom/src/doomtype.h
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
 *  Simple basic typedefs, isolated here to make it easier
 *  separating modules.
 *
 ****************************************************************************/

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <strings.h>

/* What is really wanted here is stdint.h; however, some old versions
 * of Solaris don't have stdint.h and only have inttypes.h (the
 * pre-standardisation version).  inttypes.h is also in the C99
 * standard and defined to include stdint.h, so include this.
 */

#include <inttypes.h>
#include <stdbool.h>

#include <limits.h>

#include "config.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"
#define PATH_SEPARATOR ':'

#define arrlen(array) (sizeof(array) / sizeof(*array))

/* The packed attribute forces structures to be packed into the minimum
 * space necessary.  If this is not done, the compiler may align structure
 * fields differently to optimize memory access, inflating the overall
 * structure size.  It is important to use the packed attribute on certain
 * structures where alignment is important, particularly data read/written
 * to disk.
 */

#ifdef __GNUC__

#define PACKEDATTR __attribute__((packed))

#define PRINTF_ATTR(fmt, first) __attribute__((format(printf, fmt, first)))
#define PRINTF_ARG_ATTR(x) __attribute__((format_arg(x)))
#define NORETURN __attribute__((noreturn))

#else
#if defined(_MSC_VER)
#define PACKEDATTR __pragma(pack(pop))
#else
#define PACKEDATTR
#endif
#define PRINTF_ATTR(fmt, first)
#define PRINTF_ARG_ATTR(x)
#define NORETURN
#endif /* __GNUC__ */

#define PACKEDPREFIX

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* C99 integer types; with gcc we just use this.  Other compilers
 * should add conditional statements that define the C99 types.
 */

#if defined(__cplusplus) || defined(__bool_true_false_are_defined)

/* The C++/C99 bool type (or _Bool that is) can only have two values:
 * 0 or 1. However, the Doom source code assumes any non-zero value
 * to evaluate to true, so we have to use an int type here.
 */

typedef int boolean;

#else

typedef enum
{
  false,
  true
} boolean;

#endif

typedef uint8_t byte;
typedef uint8_t pixel_t;
typedef int16_t dpixel_t;

#endif /* __DOOMTYPE__ */
