/****************************************************************************
 * apps/games/NXDoom/src/i_swap.h
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
 *  Endianness handling, swapping 16bit and 32bit.
 *
 ****************************************************************************/

#ifndef __I_SWAP__
#define __I_SWAP__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <endian.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Endianness handling.
 * WAD files are stored little endian.
 */

/* These are deliberately cast to signed values; this is the behaviour
 * of the macros in the original source and some code relies on it.
 */

#define SHORT(x) ((signed short)le16toh(x))
#define LONG(x) ((signed int)le32toh(x))

/* Defines for checking the endianness of the system. */

#if BYTE_ORDER == BIG_ENDIAN
#define SYS_BIG_ENDIAN
#endif

#endif /* __I_SWAP__ */
