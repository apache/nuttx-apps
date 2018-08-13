/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_rgb2pixel.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <math.h>

#include "wld_color.h"

/****************************************************************************
 * Name: wld_rgb2pixel
 * Description: Map a RGB triplet into the corresponding pixel.  The
 *   value range of ech RGB value is assume to lie within 0 through
 *   TRV_PIXEL_MAX.
 ****************************************************************************/

wld_pixel_t wld_rgb2pixel(color_rgb_t * pixel)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  color_lum_t lum;

  /* Convert the RGB Value into a luminance value. Get the luminance associated
   * with the RGB value.
   */

  lum.luminance = sqrt(pixel->red * pixel->red
                       + pixel->green * pixel->green
                       + pixel->blue * pixel->blue);

  /* Convert the RGB Component into unit vector + luminance */

  if (lum.luminance > 0.0)
    {
      lum.red = (float)pixel->red / lum.luminance;
      lum.green = (float)pixel->green / lum.luminance;
      lum.blue = (float)pixel->blue / lum.luminance;
    }
  else
    {
      lum.red = lum.green = lum.blue = g_unit_vector[GREY_NDX].red;
    }

  return wld_lum2pixel(&lum);
#else
  wld_pixel_t ret;
  int red;
  int green;
  int blue;

  red = MIN((pixel->red * RGB_CUBE_SIZE) / (TRV_PIXEL_MAX + 1),
            RGB_CUBE_SIZE - 1);
  green = MIN((pixel->green * RGB_CUBE_SIZE) / (TRV_PIXEL_MAX + 1),
              RGB_CUBE_SIZE - 1);
  blue = MIN((pixel->blue * RGB_CUBE_SIZE) / (TRV_PIXEL_MAX + 1),
             RGB_CUBE_SIZE - 1);

  ret = (red * RGB_CUBE_SIZE + green) * RGB_CUBE_SIZE + blue;

  return ret;
#endif
}
