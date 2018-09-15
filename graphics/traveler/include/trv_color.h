/****************************************************************************
 * apps/graphics/traveler/include/trv_color.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_COLOR_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_COLOR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_graphics.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_GRAPHICS_TRAVELER_RGB16_565)
/* This macro creates RGB16 (5:6:5)
 *
 *   R[7:3] -> RGB[15:11]
 *   G[7:2] -> RGB[10:5]
 *   B[7:3] -> RGB[4:0]
 */

#  define TRV_MKRGB(r,g,b) \
  ((((uint16_t)(r) << 8) & 0xf800) | (((uint16_t)(g) << 3) & 0x07e0) | (((uint16_t)(b) >> 3) & 0x001f))

/* And these macros perform the inverse transformation */

#  define RGB2RED(rgb)   (((rgb) >> 8) & 0xf8)
#  define RGB2GREEN(rgb) (((rgb) >> 3) & 0xfc)
#  define RGB2BLUE(rgb)  (((rgb) << 3) & 0xf8)

#elif defined(CONFIG_GRAPHICS_TRAVELER_RGB32_888)
/* This macro creates RGB24 (8:8:8)
 *
 *   R[7:3] -> RGB[15:11]
 *   G[7:2] -> RGB[10:5]
 *   B[7:3] -> RGB[4:0]
 */

#  define TRV_MKRGB(r,g,b) \
  ((uint32_t)((r) & 0xff) << 16 | (uint32_t)((g) & 0xff) << 8 | (uint32_t)((b) & 0xff))

/* And these macros perform the inverse transformation */

#  define RGB2RED(rgb)   (((rgb) >> 16) & 0xff)
#  define RGB2GREEN(rgb) (((rgb) >> 8)  & 0xff)
#  define RGB2BLUE(rgb)  ( (rgb)        & 0xff)

#else
#  error No color format defined
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct trv_color_rgb_s
{
  uint8_t red;    /* red   component of color 0-63 */
  uint8_t green;  /* green component of color 0-63 */
  uint8_t blue;   /* blue  component of color 0-63 */
};

struct trv_color_lum_s
{
  float red;
  float green;
  float blue;
  float luminance;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void trv_color_allocate(FAR struct trv_palette_s *pinfo);
void trv_color_endmapping(void);
void trv_color_free(FAR struct trv_palette_s *pinfo);
trv_pixel_t trv_color_rgb2pixel(FAR struct trv_color_rgb_s *pixel);
void trv_color_pixel2lum(trv_pixel_t pixel, FAR struct trv_color_lum_s *lum);
trv_pixel_t trv_color_lum2pixel(FAR struct trv_color_lum_s *lum);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_COLOR_H */
