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

#include "trv_types.h"
#include "wld_bitmaps.h"
#include "wld_color.h"

/****************************************************************************
 * Name: wld_lum2formtype
 *
 * Description:
 *   Convert an ordered RGB-Luminance value a color form  (index into
 *   XXXForm arrays).
 *
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
static uint8_t wld_lum2formtype(color_form_t * lum)
{
  float factor1;
  float factor2;
  float factor3;
  float error;
  float lse;
  uint8_t formno;
  int i;

  /* Initialize for the search */

  factor1 = g_wld_colorform[0].max - lum->max;
  factor2 = g_wld_colorform[0].mid - lum->mid;
  factor3 = g_wld_colorform[0].min - lum->min;
  lse = factor1 * factor1 + factor2 * factor2 + factor3 * factor3;
  formno = 0;

  /* Now, search the rest of the table, keeping the form with least squared
   * error value
   */

  for (i = 1; i < NCOLOR_FORMS; i++)
    {
      factor1 = g_wld_colorform[i].max - lum->max;
      factor2 = g_wld_colorform[i].mid - lum->mid;
      factor3 = g_wld_colorform[i].min - lum->min;
      error = factor1 * factor1 + factor2 * factor2 + factor3 * factor3;
      if (error < lse)
        {
          lse = error;
          formno = i;
        }
    }

  return formno;
}
#endif

/****************************************************************************
 * Name: wld_lum2colorform
 *
 * Description:
 *   Convert an RGB-Luminance value into a color form code (index into
 *   g_unit_vector array).
 *
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
static enum unit_vector_index_e wld_lum2colorform(color_lum_t * lum)
{
  color_form_t orderedLum;
  enum unit_vector_index_e uvndx;

  /* Get an ordered representation of the luminance value */

  if (lum->red >= lum->green)
    {
      if (lum->red >= lum->blue)
        {
          /* RED >= GREEN && RED >= BLUE */

          if (lum->green >= lum->blue)
            {
              /* RED >= GREEN >= BLUE */

              orderedLum.max = lum->red;
              orderedLum.mid = lum->green;
              orderedLum.min = lum->blue;
              uvndx = g_wld_rgbform_map[wld_lum2formtype(&orderedLum)];
            }
          else
            {
              /* RED >= BLUE > GREEN */

              orderedLum.max = lum->red;
              orderedLum.mid = lum->blue;
              orderedLum.min = lum->green;
              uvndx = g_wld_rbgform_map[wld_lum2formtype(&orderedLum)];
            }
        }
      else
        {
          /* BLUE > RED >= GREEN */

          orderedLum.max = lum->blue;
          orderedLum.mid = lum->red;
          orderedLum.min = lum->green;
          uvndx = g_wld_brgform_map[wld_lum2formtype(&orderedLum)];
        }
    }
  else
    {
      /* lum->red < lum->green) */

      if (lum->green >= lum->blue)
        {
          /* GREEN > RED && GREEN >= BLUE */

          if (lum->red >= lum->blue)
            {
              /* GREEN > RED >= BLUE */

              orderedLum.max = lum->green;
              orderedLum.mid = lum->red;
              orderedLum.min = lum->blue;
              uvndx = g_wld_grbform_map[wld_lum2formtype(&orderedLum)];
            }
          else
            {
              /* GREEN >= BLUE > RED */

              orderedLum.max = lum->green;
              orderedLum.mid = lum->blue;
              orderedLum.min = lum->red;
              uvndx = g_wld_gbrform_map[wld_lum2formtype(&orderedLum)];
            }
        }
      else
        {
          /* BLUE > GREEN > RED */

          orderedLum.max = lum->blue;
          orderedLum.mid = lum->green;
          orderedLum.min = lum->red;
          uvndx = g_wld_bgrform_map[wld_lum2formtype(&orderedLum)];
        }
    }

  return uvndx;
}
#endif

/****************************************************************************
 * Name: wld_color_lum2pixel
 * Description: Convert an RGB-Luminance value into a pixel
 ****************************************************************************/

wld_pixel_t wld_lum2pixel(color_lum_t * lum)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  enum unit_vector_index_e uvndx;
  uint8_t lumndx;

  /* Get the g_unit_vector array index associated with this lum */

  uvndx = wld_lum2colorform(lum);

  /* Get the luminance number associated with this lum at this index Make sure
   * that the requested luminance does not exceed the maximum allowed for this
   * unit vector.
   */

  if (lum->luminance >= g_unit_vector[uvndx].luminance)
    {
      lumndx = (NLUMINANCES - 1);
    }
  else
    {
      lumndx = (uint8_t) ((float)NLUMINANCES
                          * lum->luminance / g_unit_vector[uvndx].luminance);
      if (lumndx > 0)
        {
          lumndx--;
        }
    }

  /* Now get the pixel value from the unit vector index and the luminance
   * number.  We will probably have to expand this later from the 8-bit index
   * to a wider representation.
   */

  return WLD_UVLUM2NDX(uvndx, lumndx);

#else
  color_rgb_t rgb;

  /* Convert the luminance value to its RGB components */

  rgb.red = lum->red * lum->luminance;
  rgb.green = lum->green * lum->luminance;
  rgb.blue = lum->blue * lum->luminance;

  return wld_rgb2pixel(&rgb);
#endif
}
