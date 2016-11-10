/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_loadpaltable.c
 * This file contains the logic that creates the range palette table that is
 * used to modify the palette with range to hit
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

/*************************************************************************
 * Included files
 *************************************************************************/

#include <math.h>

#include "trv_types.h"
#include "wld_paltable.h"
#include "wld_color.h"

/*************************************************************************
 * Private Data
 *************************************************************************/

static float g_cube2pixel = (float)TRV_PIXEL_MAX / (float)(RGB_CUBE_SIZE-1);

/*************************************************************************
 * Function: wld_pixel2lum
 * Description: Convert a pixel value into RGB-Luminance value.
 ************************************************************************/

void wld_pixel2lum(trv_pixel_t pixel_value, color_lum_t *lum)
{
  dev_pixel_t devpixel = g_devpixel_lut[pixel_value];

  /* Convert the pixel to its RGB components */

  lum->red   = (float)RGB2RED(devpixel)   / (float)RGB_MAX_RED;
  lum->green = (float)RGB2GREEN(devpixel) / (float)RGB_MAX_GREEN;
  lum->blue  = (float)RGB2BLUE(devpixel)  / (float)RGB_MAX_BLUE;

  /* Get the luminance associated with the RGB value */

  lum->luminance = sqrt(lum->red   * lum->red +
                        lum->green * lum->green +
                        lum->blue  * lum->blue);

   /* Convert the RGB Component into unit vector + luminance */

   if (lum->luminance > 0.0)
     {
       lum->red   /= lum->luminance;
       lum->blue  /= lum->luminance;
       lum->green /= lum->luminance;
     }
}
