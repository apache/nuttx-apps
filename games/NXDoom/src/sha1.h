/****************************************************************************
 * apps/games/NXDoom/src/sha1.h
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
 *   SHA-1 digest.
 *
 ****************************************************************************/

#ifndef __SHA1_H__
#define __SHA1_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <crypto/sha1.h>

#include "doomtype.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define sha1_updatestring(ctx, str)                                          \
  sha1update((ctx), (byte *)(str), strlen((str)) + 1)

#define sha1_updateint32(ctx, val)                                           \
  do                                                                         \
    {                                                                        \
      byte __sha1__tmp__buf[4];                                              \
      __sha1__tmp__buf[0] = ((val) >> 24) & 0xff;                            \
      __sha1__tmp__buf[1] = ((val) >> 16) & 0xff;                            \
      __sha1__tmp__buf[2] = ((val) >> 8) & 0xff;                             \
      __sha1__tmp__buf[3] = (val) & 0xff;                                    \
      sha1update((ctx), __sha1__tmp__buf, 4);                                \
    }                                                                        \
  while (0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef byte sha1_digest_t[SHA1_DIGEST_LENGTH];

#endif /* __SHA1_H__ */
