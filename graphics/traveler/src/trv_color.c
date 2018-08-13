/****************************************************************************
 * apps/graphics/traveler/src/trv_color.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_mem.h"
#include "trv_bitmaps.h"
#include "trv_main.h"
#include "trv_graphics.h"
#include "trv_color.h"

#include <stdlib.h>
#include <math.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* This defines the size of the RGB cube that can be supported by trv_pixel_t
 * Use of the RGB cube gives us a fine control of the color space.  However
 * the lighting logic in this program requires fine control over luminance.
 * If the RGB_CUBE_SIZE is small, then an alternative luminance model is
 * used.
 */

#define RGB_CUBE_SIZE 6
#define MIN_LUM_LEVELS 8

/* These are definitions needed to support the luminance model */

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
#define NUNIT_VECTORS  16
#define NLUMINANCES    16    /* ((TRV_PIXEL_MAX+1)/NUNIT_VECTORS) */
#define MAX_LUM_INDEX 255

/* The following macros perform conversions between unit vector index and
 * luminance code into pixels and vice versa.  NOTE:  These macros assume
 * on the above values for NUNIT_VECTORS and NLUMINANCES
 */

#define TRV_UVLUM2NDX(u,l) (((int)(u) << 4) + (l))
#define TRV_NDX2UV(p)      ((p) >> 4)
#define TRV_NDX2LUM(p)     ((p) & 0x0f)

/* Each color is one of NCOLOR_FORMS */

#define NCOLOR_FORMS 5

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/* The following enumeration defines indices into the g_unit_vector array */

enum unit_vector_index_e
{
  GREY_NDX = 0,
  BLUE_NDX,
  GREENERBLUE_NDX,
  BLUEGREEN_NDX,
  BLUEVIOLET_NDX,
  VIOLET_NDX,
  LIGHTBLUE_NDX,
  GREEN_NDX,
  BLUERGREN_NDX,
  YELLOWGREEN_NDX,
  YELLOW_NDX,
  LIGHTGREEN_NDX,
  RED_NDX,
  REDVIOLET_NDX,
  ORANGE_NDX,
  PINK_NDX
};

#endif

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
struct color_form_s
{
  float max;
  float mid;
  float min;
} ;
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
static FAR struct trv_color_lum_s *g_pixel2um_lut;

/* The following defines the "form" of each color in the g_unit_vector array */

static const struct color_form_s g_trv_colorform[NCOLOR_FORMS] =
{
  { 1.0,       0.0,       0.0       },
  { 0.875,     0.4841229, 0.0       },
  { 0.7071068, 0.7071068, 0.0       },
  { 0.6666667, 0.5270463, 0.5270463 },
  { 0.5773503, 0.5773503, 0.5773503 }
};

/* These arrays map color forms into g_unit_vector array indices */

static const enum unit_vector_index_e g_trv_bgrform_map[NCOLOR_FORMS] =
  {
    BLUE_NDX, GREENERBLUE_NDX, BLUEGREEN_NDX, LIGHTBLUE_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_trv_brgform_map[NCOLOR_FORMS] =
  {
    BLUE_NDX, BLUEVIOLET_NDX, VIOLET_NDX, LIGHTBLUE_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_trv_gbrform_map[NCOLOR_FORMS] =
  {
    GREEN_NDX, BLUERGREN_NDX, BLUEGREEN_NDX, LIGHTGREEN_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_trv_grbform_map[NCOLOR_FORMS] =
  {
    GREEN_NDX, YELLOWGREEN_NDX, YELLOW_NDX, LIGHTGREEN_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_trv_rbgform_map[NCOLOR_FORMS] =
  {
    RED_NDX, REDVIOLET_NDX, VIOLET_NDX, PINK_NDX, GREY_NDX
  };

static const enum unit_vector_index_e g_trv_rgbform_map[NCOLOR_FORMS] =
  {
    RED_NDX, ORANGE_NDX, YELLOW_NDX, PINK_NDX, GREY_NDX
  };

/* This array defines each color supported in the luminance model */

static const struct trv_color_lum_s g_unit_vector[NUNIT_VECTORS] =
{
  { 0.5773503, 0.5773503, 0.5773503, 441.672932,}, /* GREY_NDX */

  { 0.0,       0.0,       1.0,       255.0 },      /* BLUE_NDX */
  { 0.0,       0.4841229, 0.875,     291.428571 }, /* GREENERBLUE_NDX */
  { 0.0,       0.7071068, 0.7071068, 360.624445 }, /* BLUEGREEN_NDX */
  { 0.4841229, 0.0,       0.875,     291.428571 }, /* BLUEVIOLET_NDX */
  { 0.7071068, 0.0,       0.7071068, 360.624445 }, /* VIOLET_NDX */
  { 0.5270463, 0.5270463, 0.6666667, 382.499981 }, /* LIGHTBLUE_NDX */

  { 0.0,       1.0,       0.0,       255.0 },      /* GREEN_NDX */
  { 0.0,       0.875,     0.4841229, 291.428571 }, /* BLUERGREN_NDX */
  { 0.4841229, 0.875,     0.0,       291.428571 }, /* YELLOWGREEN_NDX */
  { 0.7071068, 0.7071068, 0.0,       360.624445 }, /* YELLOW_NDX */
  { 0.5270463, 0.6666667, 0.5270463, 382.499981 }, /* LIGHTGREEN_NDX */

  { 1.0,       0.0,       0.0,       255.0 },      /* RED_NDX */
  { 0.875,     0.0,       0.4841229, 291.428571 }, /* REDVIOLET_NDX */
  { 0.875,     0.4841229, 0.0,       291.428571 }, /* ORANGE_NDX */
  { 0.6666667, 0.5270463, 0.5270463, 382.499981 }, /* PINK_NDX */
};
#else
static struct trv_color_rgb_s *g_pixl2rgb_lut = NULL;
static float g_trv_cube2pixel;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_lum2formtype
 *
 * Description:
 *   Convert an ordered RGB-Luminance value a color form  (index into
 *   XXXForm arrays).
 *
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
static uint8_t trv_lum2formtype(struct color_form_s *lum)
{
   float factor1, factor2, factor3, error, lse;
   uint8_t formNumber;
   int i;

   /* Initialize for the search */
   factor1 = g_trv_colorform[0].max - lum->max;
   factor2 = g_trv_colorform[0].mid - lum->mid;
   factor3 = g_trv_colorform[0].min - lum->min;
   lse = factor1*factor1 + factor2*factor2 + factor3*factor3;
   formNumber = 0;

   /* Now, search the rest of the table, keeping the form with least
    * squared error value
    */

   for (i = 1; i < NCOLOR_FORMS; i++)
     {
       factor1 = g_trv_colorform[i].max - lum->max;
       factor2 = g_trv_colorform[i].mid - lum->mid;
       factor3 = g_trv_colorform[i].min - lum->min;
       error = factor1*factor1 + factor2*factor2 + factor3*factor3;
       if (error < lse)
         {
           lse = error;
           formNumber = i;
         }
     }
   return formNumber;
}
#endif

/****************************************************************************
 * Name: trv_lum2colorform
 *
 * Description:
 *   Convert an RGB-Luminance value into a color form code (index into
 *   g_unit_vector array).
 *
 ****************************************************************************/

#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
static enum unit_vector_index_e trv_lum2colorform(struct trv_color_lum_s *lum)
{
   struct color_form_s orderedLum;
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
               uvndx = g_trv_rgbform_map[trv_lum2formtype(&orderedLum)];
             }
           else
             {
               /* RED >= BLUE > GREEN */

               orderedLum.max = lum->red;
               orderedLum.mid = lum->blue;
               orderedLum.min = lum->green;
               uvndx = g_trv_rbgform_map[trv_lum2formtype(&orderedLum)];
             }
         }
       else
         {
           /* BLUE > RED >= GREEN */

           orderedLum.max = lum->blue;
           orderedLum.mid = lum->red;
           orderedLum.min = lum->green;
           uvndx = g_trv_brgform_map[trv_lum2formtype(&orderedLum)];
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
               uvndx = g_trv_grbform_map[trv_lum2formtype(&orderedLum)];
             }
           else
             {
               /* GREEN >= BLUE > RED*/

               orderedLum.max = lum->green;
               orderedLum.mid = lum->blue;
               orderedLum.min = lum->red;
               uvndx = g_trv_gbrform_map[trv_lum2formtype(&orderedLum)];
             }
         }
       else
         {
           /* BLUE > GREEN > RED */

           orderedLum.max = lum->blue;
           orderedLum.mid = lum->green;
           orderedLum.min = lum->red;
           uvndx = g_trv_bgrform_map[trv_lum2formtype(&orderedLum)];
         }
     }

   return uvndx;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_color_allocate
 *
 * Description:
 ****************************************************************************/

void trv_color_allocate(FAR struct trv_palette_s *pinfo)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  dev_pixel_t *lut;
  int uvndx;
  int lumndx;
  int index;

  /* Check if a color lookup table has been allocated */

  pinfo->lut = (dev_pixel_t*)
    trv_malloc(sizeof(dev_pixel_t) * (NUNIT_VECTORS*NLUMINANCES));

  if (!pinfo->lut)
    {
      trv_abort("ERROR: Failed to allocate color lookup table\n");
    }

  lut = pinfo->lut;

  /* Save the color information and color lookup table for use in
   * color mapping below.
   */

  g_pixel2um_lut = (struct trv_color_lum_s*)
    trv_malloc(sizeof(struct trv_color_lum_s) * (NUNIT_VECTORS*NLUMINANCES));

  if (!g_pixel2um_lut)
    {
      trv_abort("ERROR: Failed to allocate luminance table\n");
    }

  /* Allocate each color at each luminance value */

  pinfo->ncolors = 0;
  index = 0;

  for (uvndx = 0; uvndx < NUNIT_VECTORS; uvndx++)
    {
      for (lumndx = 0; lumndx < NLUMINANCES; lumndx++)
        {
          struct trv_color_rgb_s color;
          FAR struct trv_color_lum_s *lum;

          /* Get a convenience pointer to the lookup table entry */

           lum = &g_pixel2um_lut[index];
          *lum = g_unit_vector[uvndx];

          /* Get the luminance associated with this lum for this
           * unit vector.
           */

          lum->luminance = (lum->luminance * (float)(lumndx + 1)) / NLUMINANCES;

          /* Convert to RGB and allocate the color */

          color.red   = (short) (lum->red   * lum->luminance);
          color.green = (short) (lum->green * lum->luminance);
          color.blue  = (short) (lum->blue  * lum->luminance);

          /* Save the RGB to pixel lookup data */

          lut[index] = TRV_MKRGB(color.red, color.green, color.blue);
          pinfo->ncolors = ++index;
        }
    }

#else
  dev_pixel_t *lut;
  int index;
  struct trv_color_rgb_s rgb;

  /* Check if a color lookup table has been allocated */

  pinfo->lut = (dev_pixel_t*)
    trv_malloc(sizeof(dev_pixel_t) * (TRV_PIXEL_MAX+1));

  if (!pinfo->lut)
    {
      trv_abort("ERROR: Failed to allocate color lookup table\n");
    }

  /* Save the color information and color lookup table for use in
   * subsequent color mapping.
   */

  lut = pinfo->lut;

  /* Check if a Pixel-to-RGB color mapping table has been allocated */

  g_pixl2rgb_lut = (struct trv_color_rgb_s*)
    trv_malloc(sizeof(struct trv_color_rgb_s) * (TRV_PIXEL_MAX+1));

  if (!g_pixl2rgb_lut)
    {
      trv_abort("ERROR: Failed to allocate luminance table\n");
    }

  for (index = 0; index <= TRV_PIXEL_MAX; index++)
    {
      g_pixl2rgb_lut[index].red
        = g_pixl2rgb_lut[index].green
        = g_pixl2rgb_lut[index].blue = 0;
    }

  /* Calculate the cube to trv_pixel_t scale factor.  This factor will
   * convert an RGB component in the range {0..RGB_CUBE_SIZE-1} to
   * a value in the range {0..TRV_PIXEL_MAX}.
   */

  g_trv_cube2pixel = (float)TRV_PIXEL_MAX / (float)(RGB_CUBE_SIZE-1);

  /* Allocate each color in the RGB Cube */

  pinfo->ncolors = index = 0;
  for (rgb.red = 0;   rgb.red   < RGB_CUBE_SIZE; rgb.red++)
    for (rgb.green = 0; rgb.green < RGB_CUBE_SIZE; rgb.green++)
      for (rgb.blue = 0;  rgb.blue  < RGB_CUBE_SIZE; rgb.blue++)
        {
          struct trv_color_rgb_s color;

          color.red   = (short) (rgb.red   * 65535 / (RGB_CUBE_SIZE - 1));
          color.green = (short) (rgb.green * 65535 / (RGB_CUBE_SIZE - 1));
          color.blue  = (short) (rgb.blue  * 65535 / (RGB_CUBE_SIZE - 1));

          /* Save the RGB to pixel lookup data */

          lut[index] = TRV_MKRGB(color.red, color.green, color.blue);
          pinfo->ncolors = ++index;

          /* Save the pixel to RGB lookup data */

          if (color.pixel <= TRV_PIXEL_MAX)
             {
               g_pixl2rgb_lut[color.pixel] = rgb;
             }
        }
#endif
}

/****************************************************************************
 * Name: trv_color_endmapping
 *
 * Description:
 *   When all color mapping has been performed, this function should be
 *   called to release all resources dedicated to color mapping.
 *
 ****************************************************************************/

void trv_color_endmapping(void)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  if (g_pixel2um_lut)
    {
      trv_free(g_pixel2um_lut);
      g_pixel2um_lut = NULL;
    }
#else
  if (g_pixl2rgb_lut)
    {
      trv_free(g_pixl2rgb_lut);
      g_pixl2rgb_lut = NULL;
    }
#endif
}

/****************************************************************************
 * Name: trv_color_free
 *
 * Description:
 *   Free the color lookup table
 *
 ****************************************************************************/

void trv_color_free(struct trv_palette_s *pinfo)
{
  if (pinfo->lut)
    {
      trv_free(pinfo->lut);
      pinfo->lut = NULL;
    }
}

/****************************************************************************
 * Name: trv_color_rgb2pixel
 * Description: Map a RGB triplet into the corresponding pixel.  The
 *   value range of ech RGB value is assume to lie within 0 through
 *   TRV_PIXEL_MAX.
 ****************************************************************************/

trv_pixel_t trv_color_rgb2pixel(struct trv_color_rgb_s *pixel)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
   struct trv_color_lum_s lum;

   /* Convert the RGB Value into a luminance value.
    * Get the luminance associated with the RGB value.
    */

   lum.luminance = sqrt(pixel->red   * pixel->red
                      + pixel->green * pixel->green
                      + pixel->blue  * pixel->blue);

   /* Convert the RGB Component into unit vector + luminance */

   if (lum.luminance > 0.0)
     {
       lum.red   = (float)pixel->red   / lum.luminance;
       lum.green = (float)pixel->green / lum.luminance;
       lum.blue  = (float)pixel->blue  / lum.luminance;
     }
   else
     {
       lum.red = lum.green = lum.blue = g_unit_vector[GREY_NDX].red;
     }

   return trv_color_lum2pixel(&lum);
#else
   trv_pixel_t returnValue;
   int red, green, blue;

   red   = MIN((pixel->red   * RGB_CUBE_SIZE) / (TRV_PIXEL_MAX+1),
               RGB_CUBE_SIZE - 1);
   green = MIN((pixel->green * RGB_CUBE_SIZE) / (TRV_PIXEL_MAX+1),
               RGB_CUBE_SIZE - 1);
   blue  = MIN((pixel->blue  * RGB_CUBE_SIZE) / (TRV_PIXEL_MAX+1),
               RGB_CUBE_SIZE - 1);

   returnValue = (red * RGB_CUBE_SIZE + green) * RGB_CUBE_SIZE + blue;

   return returnValue;
#endif
}

/****************************************************************************
 * Name: trv_color_lum2pixel
 * Description: Convert an RGB-Luminance value into a pixel
 ****************************************************************************/

trv_pixel_t trv_color_lum2pixel(struct trv_color_lum_s *lum)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
   enum unit_vector_index_e uvndx;
   uint8_t lumndx;

   /* Get the g_unit_vector array index associated with this lum */

   uvndx = trv_lum2colorform(lum);

   /* Get the luminance number associated with this lum at this index
    * Make sure that the requested luminance does not exceed the maximum
    * allowed for this unit vector.
    */

   if (lum->luminance >= g_unit_vector[uvndx].luminance)
     {
       lumndx = (NLUMINANCES-1);
     }
   else
     {
       lumndx = (uint8_t)((float)NLUMINANCES
                           * lum->luminance / g_unit_vector[uvndx].luminance);
       if (lumndx) lumndx--;
     }

   /* Now get the pixel value from the unit vector index and the luminance
    * number.  We will probably have to expand this later from the 8-bit
    * index to a wider representation.
    */

   return TRV_UVLUM2NDX(uvndx, lumndx);

#else
   struct trv_color_rgb_s rgb;

   /* Convert the luminance value to its RGB components */
   rgb.red   = lum->red   * lum->luminance;
   rgb.green = lum->green * lum->luminance;
   rgb.blue  = lum->blue  * lum->luminance;

   return trv_color_rgb2pixel(&rgb);
#endif
}

/****************************************************************************
 * Name: trv_color_pixel2lum
 * Description: Convert a pixel value into RGB-Luminance value.
 ****************************************************************************/

void trv_color_pixel2lum(trv_pixel_t pixval,
                         struct trv_color_lum_s *lum)
{
#if RGB_CUBE_SIZE < MIN_LUM_LEVELS
  *lum = g_pixel2um_lut[pixval];
#else
   /* Convert the pixel to its RGB components */

   lum->red   = g_trv_cube2pixel * (float)g_pixl2rgb_lut[pixval].red;
   lum->green = g_trv_cube2pixel * (float)g_pixl2rgb_lut[pixval].green;
   lum->blue  = g_trv_cube2pixel * (float)g_pixl2rgb_lut[pixval].blue;

   /* Get the luminance associated with the RGB value */

   lum->luminance = sqrt(lum->red   * lum->red
                       + lum->green * lum->green
                       + lum->blue  * lum->blue);

   /* Convert the RGB Component into unit vector + luminance */

   if (lum->luminance > 0.0)
     {
       lum->red /= lum->luminance;
       lum->blue /= lum->luminance;
       lum->green /= lum->luminance;
     }
#endif
}
