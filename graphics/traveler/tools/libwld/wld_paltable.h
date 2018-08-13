/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_paltable.h
 * This file contains definitions for the world model
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PALTABLE_H
#define __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PALTABLE_H

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*************************************************************************
 * Included Files
 *************************************************************************/

#include "wld_color.h"

/*************************************************************************
 * Pre-processor Definitions
 *************************************************************************/

/* Here some palette-related definitions. */

#define PALETTE_SIZE  256 /* This is the number of colors in the palette */
#define NUM_ZONES      16 /* This is the number of distance zones in the palette table */

/* Here are some macros used to access the g_pal_table.  */

#define PAL_SCALE_BITS     2
#define GET_DISTANCE(x,y)  ( (x) >= (y) ? ((x) + ((y) >>1)) : ((y) + ((x) >>1)))
#define GET_ZONE(x,y)      (GET_DISTANCE(x,y) >> (sSHIFT+PAL_SCALE_BITS))
#define GET_PALINDEX(d)    ((d) >= NUM_ZONES ? (NUM_ZONES-1) : (d))
#define GET_PALPTR(d)      g_pal_table[GET_PALINDEX(d)]

/* This is a special version which is used by the texture logic.  The
 * texture engine used 8 bits of fraction in many of its calculation
 */

#define GET_FZONE(x,y,n)   (GET_DISTANCE(x,y) >> (sSHIFT+PAL_SCALE_BITS-(n)))

/*************************************************************************
 * Public Type Definitions
 *************************************************************************/

/* World file return codes */

enum
{
  PALR_FILE_OPEN_ERROR = 150,
  PALR_TOO_MANY_RANGES
};

/*************************************************************************
 * Public Data
 *************************************************************************/

/* This is the palette table which is used to adjust the texture values
 * with distance
 */

extern wld_pixel_t *g_pal_table[NUM_ZONES];

/*************************************************************************
 * Pulblic Function Prototypes
 *************************************************************************/

uint8_t wld_load_paltable(char *file);
void  wld_discard_paltable(void);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PALTABLE_H */

