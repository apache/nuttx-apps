/****************************************************************************
 * apps/graphics/traveler/include/trv_rayrend.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYEND_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYEND_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The following macro calculates the texture index for the given the
 * horizontal position (h), the vertical position (v) and the log2 bitmap
 * size (s).
 */

#define TMASK(s)       ((1 << (s)) - 1)
#define TFIX(x,s)      ((x) << (s))
#define TFRAC(x,m)     ((x) & (m))
#define TNDX(h,v,s,m)  (TFIX( TFRAC( (~(v)), m ), s) + TFRAC( h, m ) )

/* Here is a macro to extract one pixel from a WALL texture.  This is used
 * by the raycasters to determine a hit in a transparent wall hit an
 * opaque portion of the texture
 */

#define GET_FRONT_PIXEL(r,h,v) \
   trv_get_rectpixel(h,v,g_even_bitmaps[ (r)->texture ], (r)->scale)
#define GET_BACK_PIXEL(r,h,v) \
   trv_get_rectpixel(h,v,g_odd_bitmaps[ (r)->texture ], (r)->scale)

/* This is special value of a pixel in a "grandparent" rectangle which means
 * that the pixel is transparent.
 */

#define INVISIBLE_PIXEL 0

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct trv_camera_s;
struct trv_graphics_info_s;
struct trv_bitmap_s;

void trv_rend_backdrop(FAR struct trv_camera_s *camera,
                       FAR struct trv_graphics_info_s *ginfo);
void trv_rend_cell(uint8_t row, uint8_t col, uint8_t height, uint8_t width);
void trv_rend_row(uint8_t row, uint8_t col, uint8_t width);
void trv_rend_column(uint8_t row, uint8_t col, uint8_t height);
void trv_rend_pixel(uint8_t row, uint8_t col);
trv_pixel_t trv_get_rectpixel(int16_t hPos, int16_t vPos,
                              FAR struct trv_bitmap_s *bmp, uint8_t scale);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYEND_H */
