/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_rgblookup.c
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

#include <stdlib.h>
#include <math.h>

#include "trv_types.h"
#include "wld_mem.h"
#include "wld_utils.h"
#include "wld_color.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS

/* These arrays map color forms into g_unit_vector array indices */

static const enum unit_vector_index_e g_wld_bgrform_map[NCOLOR_FORMS] =
  {
    BLUE_NDX, GREENERBLUE_NDX, BLUEGREEN_NDX, LIGHTBLUE_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_wld_brgform_map[NCOLOR_FORMS] =
  {
    BLUE_NDX, BLUEVIOLET_NDX, VIOLET_NDX, LIGHTBLUE_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_wld_gbrform_map[NCOLOR_FORMS] =
  {
    GREEN_NDX, BLUERGREN_NDX, BLUEGREEN_NDX, LIGHTGREEN_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_wld_grbform_map[NCOLOR_FORMS] =
  {
    GREEN_NDX, YELLOWGREEN_NDX, YELLOW_NDX, LIGHTGREEN_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_wld_rbgform_map[NCOLOR_FORMS] =
  {
    RED_NDX, REDVIOLET_NDX, VIOLET_NDX, PINK_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_wld_rgbform_map[NCOLOR_FORMS] =
  {
    RED_NDX, ORANGE_NDX, YELLOW_NDX, PINK_NDX, GREY_NDX
  };
#else
static color_rgb_t *g_devpixel_lut = NULL;
static float g_wld_cube2pixel;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
color_lum_t *g_pixel2um_lut;

/* The following defines the "form" of each color in the g_unit_vector array */

const color_form_t g_wld_colorform[NCOLOR_FORMS] =
{
  {1.0, 0.0, 0.0},
  {0.875, 0.4841229, 0.0},
  {0.7071068, 0.7071068, 0.0},
  {0.6666667, 0.5270463, 0.5270463},
  {0.5773503, 0.5773503, 0.5773503}
};

/* This array defines each color supported in the luminance model */

const color_lum_t g_unit_vector[NUNIT_VECTORS] =
{
  {0.5773503, 0.5773503, 0.5773503, 441.672932,},       /* GREY_NDX */

  {0.0, 0.0, 1.0, 255.0},       /* BLUE_NDX */
  {0.0, 0.4841229, 0.875, 291.428571},  /* GREENERBLUE_NDX */
  {0.0, 0.7071068, 0.7071068, 360.624445},      /* BLUEGREEN_NDX */
  {0.4841229, 0.0, 0.875, 291.428571},  /* BLUEVIOLET_NDX */
  {0.7071068, 0.0, 0.7071068, 360.624445},      /* VIOLET_NDX */
  {0.5270463, 0.5270463, 0.6666667, 382.499981},        /* LIGHTBLUE_NDX */

  {0.0, 1.0, 0.0, 255.0},       /* GREEN_NDX */
  {0.0, 0.875, 0.4841229, 291.428571},  /* BLUERGREN_NDX */
  {0.4841229, 0.875, 0.0, 291.428571},  /* YELLOWGREEN_NDX */
  {0.7071068, 0.7071068, 0.0, 360.624445},      /* YELLOW_NDX */
  {0.5270463, 0.6666667, 0.5270463, 382.499981},        /* LIGHTGREEN_NDX */

  {1.0, 0.0, 0.0, 255.0},       /* RED_NDX */
  {0.875, 0.0, 0.4841229, 291.428571},  /* REDVIOLET_NDX */
  {0.875, 0.4841229, 0.0, 291.428571},  /* ORANGE_NDX */
  {0.6666667, 0.5270463, 0.5270463, 382.499981},        /* PINK_NDX */
};
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wld_rgblookup_allocate
 *
 * Description:
 ****************************************************************************/

void wld_rgblookup_allocate(void)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  dev_pixel_t *lut;
  int uvndx;
  int lumndx;
  int index;

  /* Check if a color lookup table has been allocated */

  g_devpixel_lut = (dev_pixel_t *)
    wld_malloc(sizeof(dev_pixel_t) * (NUNIT_VECTORS * NLUMINANCES));

  if (!g_devpixel_lut)
    {
      wld_fatal_error("ERROR: Failed to allocate color lookup table\n");
    }

  lut = g_devpixel_lut;

  /* Save the color information and color lookup table for use in color mapping 
   * below. */

  g_pixel2um_lut = (color_lum_t *)
    wld_malloc(sizeof(color_lum_t) * (NUNIT_VECTORS * NLUMINANCES));

  if (!g_pixel2um_lut)
    {
      wld_fatal_error("ERROR: Failed to allocate luminance table\n");
    }

  /* Allocate each color at each luminance value */

  index = 0;
  for (uvndx = 0; uvndx < NUNIT_VECTORS; uvndx++)
    {
      for (lumndx = 0; lumndx < NLUMINANCES; lumndx++)
        {
          color_rgb_t color;
          color_lum_t *lum;

          /* Get a convenience pointer to the lookup table entry */

          lum = &g_pixel2um_lut[index];
          *lum = g_unit_vector[uvndx];

          /* Get the luminance associated with this lum for this unit vector. */

          lum->luminance = (lum->luminance * (float)(lumndx + 1)) / NLUMINANCES;

          /* Convert to RGB and allocate the color */

          color.red = (short)(lum->red * lum->luminance);
          color.green = (short)(lum->green * lum->luminance);
          color.blue = (short)(lum->blue * lum->luminance);

          /* Save the RGB to pixel lookup data */

          lut[index] = WLD_MKRGB(color.red, color.green, color.blue);
        }
    }

#else
  dev_pixel_t *lut;
  int index;
  color_rgb_t rgb;

  /* Check if a color lookup table has been allocated */

  g_devpixel_lut = (dev_pixel_t *)
    wld_malloc(sizeof(dev_pixel_t) * (WLD_PIXEL_MAX + 1));

  if (!g_devpixel_lut)
    {
      wld_fatal_error("ERROR: Failed to allocate color lookup table\n");
    }

  /* Save the color information and color lookup table for use in subsequent
   * color mapping. */

  lut = g_devpixel_lut;

  /* Check if a Pixel-to-RGB color mapping table has been allocated */

  g_devpixel_lut = (color_rgb_t *)
    wld_malloc(sizeof(color_rgb_t) * (WLD_PIXEL_MAX + 1));

  if (!g_devpixel_lut)
    {
      wld_fatal_error("ERROR: Failed to allocate luminance table\n");
    }

  for (index = 0; index <= WLD_PIXEL_MAX; index++)
    {
      g_devpixel_lut[index].red
        = g_devpixel_lut[index].green = g_devpixel_lut[index].blue = 0;
    }

  /* Calculate the cube to trv_pixel_t scale factor.  This factor will convert
   * an RGB component in the range {0..RGB_CUBE_SIZE-1} to a value in the range 
   * {0..WLD_PIXEL_MAX}. */

  g_wld_cube2pixel = (float)WLD_PIXEL_MAX / (float)(RGB_CUBE_SIZE - 1);

  /* Allocate each color in the RGB Cube */

  for (rgb.red = 0; rgb.red < RGB_CUBE_SIZE; rgb.red++)
    for (rgb.green = 0; rgb.green < RGB_CUBE_SIZE; rgb.green++)
      for (rgb.blue = 0; rgb.blue < RGB_CUBE_SIZE; rgb.blue++)
        {
          color_rgb_t color;

          color.red = (short)(rgb.red * 65535 / (RGB_CUBE_SIZE - 1));
          color.green = (short)(rgb.green * 65535 / (RGB_CUBE_SIZE - 1));
          color.blue = (short)(rgb.blue * 65535 / (RGB_CUBE_SIZE - 1));

          /* Save the RGB to pixel lookup data */

          lut[index] = WLD_MKRGB(color.red, color.green, color.blue);

          /* Save the pixel to RGB lookup data */

          if (color.pixel <= WLD_PIXEL_MAX)
            {
              g_devpixel_lut[color.pixel] = rgb;
            }
        }
#endif
}

/****************************************************************************
 * Name: wld_color_endmapping
 *
 * Description:
 *   When all color mapping has been performed, this function should be
 *   called to release all resources dedicated to color mapping.
 *
 ****************************************************************************/

void wld_color_endmapping(void)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  if (g_pixel2um_lut)
    {
      wld_free(g_pixel2um_lut);
      g_pixel2um_lut = NULL;
    }
#else
  if (g_devpixel_lut)
    {
      wld_free(g_devpixel_lut);
      g_devpixel_lut = NULL;
    }
#endif
}

/****************************************************************************
 * Name: wld_rgblookup_free
 *
 * Description:
 *   Free the color lookup table
 *
 ****************************************************************************/

void wld_rgblookup_free(void)
{
  if (g_devpixel_lut)
    {
      wld_free(g_devpixel_lut);
      g_devpixel_lut = NULL;
    }
}
