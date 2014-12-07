/****************************************************************************
 * apps/graphics/traveler/include/trv_paltable.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_PALTABLE_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_PALTABLE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor
 ****************************************************************************/

/* Here some palette-related definitions. */

#define PALETTE_SIZE  256 /* This is the number of colors in the palette */
#define NUM_ZONES      16 /* This is the number of distance zones in the palette table */

/* Here are some macros used to access the palette table.  */

#define PAL_SCALE_BITS     2
#define GET_DISTANCE(x,y)  ( (x) >= (y) ? ((x) + ((y) >>1)) : ((y) + ((x) >>1)))
#define GET_ZONE(x,y)      (GET_DISTANCE(x,y) >> (sSHIFT+PAL_SCALE_BITS))
#define GET_PALINDEX(d)    ((d) >= NUM_ZONES ? (NUM_ZONES-1) : (d))
#define GET_PALPTR(d)      g_paltable[GET_PALINDEX(d)]

/* This is a special version which is used by the texture logic.  The
 * texture engine used 8 bits of fraction in many of its calculation
 */

#define GET_FZONE(x,y,n)   (GET_DISTANCE(x,y) >> (sSHIFT+PAL_SCALE_BITS-(n)))

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This is the palette table which is used to adjust the texture values
 * with distance
 */

extern trv_pixel_t *g_paltable[NUM_ZONES];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int trv_load_paltable(FAR const char *file);
void trv_release_paltable(void);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_PALTABLE_H */
