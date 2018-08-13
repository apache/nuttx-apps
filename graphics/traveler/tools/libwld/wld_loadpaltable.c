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

/* If the following switch is false, then the whole g_pal_table is loaded from
 * the file.  Otherwise, the g_pal_table will be calculated from a range
 * table
 */

#define USE_PAL_RANGES true

#include "trv_types.h"
#include "wld_world.h"
#include "wld_paltable.h"
#if (!MSWINDOWS)
#  include "wld_bitmaps.h"
#  include "wld_color.h"
#endif
#include "wld_mem.h"
#include "wld_utils.h"

/*************************************************************************
 * Pre-processor Definitions
 *************************************************************************/

/* This indicates the maximum number of palette ranges which may be
 * specified in the file
 */

#if USE_PAL_RANGES
#  define MAX_PAL_RANGES 64
#endif

/*************************************************************************
 * Private Type Definitions
 ************************************************************************/

#if USE_PAL_RANGES
typedef struct
  {
    uint8_t firstColor;
    int8_t colorRange;
    uint8_t clipColor;
  } pal_range_t;
#endif

/*************************************************************************
 * Public Data
 *************************************************************************/

/* This is the palette table which is used to adjust the texture values
 * with distance
 */

wld_pixel_t *g_pal_table[NUM_ZONES];

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_allocate_paltable
 * Description:
 ************************************************************************/

static void wld_allocate_paltable(uint32_t palTabEntrySize)
{
  int i;

  for (i = 0; i < NUM_ZONES; i++)
    {
      g_pal_table[i] =
        (wld_pixel_t *) wld_malloc(palTabEntrySize * sizeof(wld_pixel_t));
    }
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_load_paltable
 * Description:
 * This function loads the g_pal_table from the specified file
 ************************************************************************/

uint8_t wld_load_paltable(char *file)
{
#if (!MSWINDOWS)
  wld_pixel_t *palptr;
  color_lum_t lum;
  int16_t zone;
  float scale;
  int pixel;

  /* Allocate memory to hold the palette range mapping data */

  wld_allocate_paltable(TRV_PIXEL_MAX + 1);

  /* The first zone is just the identity transformation */

  palptr = g_pal_table[0];
  for (pixel = 0; pixel <= TRV_PIXEL_MAX; pixel++)
    {
      *palptr++ = pixel;
    }

  /* Process each remaining distance zone */

  for (zone = 1; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palptr = g_pal_table[zone];

      /* Calculate the luminance scaling factor to use for this zone */

      scale = (float)(NUM_ZONES - zone) / (float)NUM_ZONES;

      /* Scale each element of the palette */

      for (pixel = 0; pixel <= TRV_PIXEL_MAX; pixel++)
        {
          /* Get the unit vector + luminance representation of the pixel */

          wld_pixel2lum(pixel, &lum);

          /* Re-scale the luminance component by range scale factor */

          lum.luminance *= scale;

          /* Then save the pixel associated with this scaled value in the range
           * palette table */

          *palptr++ = wld_lum2pixel(&lum);
        }
    }

  return WORLD_SUCCESS;

#else
#  if USE_PAL_RANGES
  FILE *fp, *fopen();
  int16_t i;
  int16_t nranges;
  int16_t zone;
  int16_t palndx;
  wld_pixel_t plotcolor;
  wld_pixel_t *palptr;
  pal_range_t ranges[MAX_PAL_RANGES];

  /* Open the file which contains the palette table */

  fp = fopen(file, "r");
  if (!fp)
    {
      return PALR_FILE_OPEN_ERROR;
    }

  /* Read the number of ranges from the file */

  nranges = wld_read_decimal(fp);
  if (nranges > MAX_PAL_RANGES)
    {
      fclose(fp);
      return PALR_TOO_MANY_RANGES;
    }

  /* Then read all of the palette range data from the file */

  for (i = 0; i < nranges; i++)
    {
      ranges[i].firstColor = wld_read_decimal(fp);
      ranges[i].colorRange = wld_read_decimal(fp);
      ranges[i].clipColor = wld_read_decimal(fp);
    }

  /* We are now done with the input file */

  fclose(fp);

  /* Allocate memory to hold the palette range mapping data */

  wld_allocate_paltable(PALETTE_SIZE);

  /* Process each distance zone */

  for (zone = 0; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palptr = g_pal_table[zone];

      /* Process each possible palette index within the zone */

      for (palndx = 0; palndx < PALETTE_SIZE; palndx++)
        {
          /* Assume that the range will not be found.  In this case, we will
           * perform the 1-to-1 mapping */

          plotcolor = palndx;

          /* Find the range the color is in. */

          for (i = 0; i < nranges; i++)
            {
              /* Check if the color range is ascending or descending */

              if (ranges[i].colorRange < 0)
                {
                  /* The colors are descending */
                  if ((palndx <= ranges[i].firstColor) &&
                      (palndx > (ranges[i].firstColor + ranges[i].colorRange)))
                    {
                      /* Found it, set the new plotcolor */

                      if (plotcolor > zone)
                        {
                          /* Offset the color by the zone we are processing */

                          plotcolor -= zone;

                          /* Check if we have exceeded the range of this color.
                           * If so, then set the color to the clipColor */

                          if (plotcolor <=
                              ranges[i].firstColor + ranges[i].colorRange)
                            {
                              plotcolor = ranges[i].clipColor;
                            }
                        }
                      else
                        {
                          plotcolor = ranges[i].clipColor;
                        }

                      /* Now break out of the loop */

                      break;
                    }
                }

              /* The colors are ascending */

              else if ((palndx >= ranges[i].firstColor) &&
                       (palndx < (ranges[i].firstColor + ranges[i].colorRange)))
                {
                  /* Found it, set the new plotcolor */

                  plotcolor += zone;

                  /* Check if we have exceeded the range of this color.  If so,
                   * then set the color to black */

                  if (plotcolor >= ranges[i].firstColor + ranges[i].colorRange)
                    {
                      plotcolor = ranges[i].clipColor;
                    }

                  /* Now break out of the loop */

                  break;
                }
            }

          /* Save the plotcolor in the g_pal_table */

          palptr[palndx] = plotcolor;

        }
    }

  return WORLD_SUCCESS;
#  else
  FILE *fp;
  int16_t zone;
  int16_t palndx;
  uint8_t plotcolor;
  uint8_t *palptr;

  /* Allocate memory to hold the palette range mapping data */

  wld_allocate_paltable();

  /* Open the file which contains the palette table */

  fp = fopen(file, "r");
  if (!fp)
    {
      return PALR_FILE_OPEN_ERROR;
    }

  /* Process each distance zone */

  for (zone = 0; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palptr = GET_PALPTR(zone);

      /* Process each possible palette index within the zone */

      for (palndx = 0; palndx < PALETTE_SIZE; palndx++)
        {
          /* Read the data into g_pal_table */

          palptr[palndx] = wld_read_decimal(fp);
        }
    }

  fclose(fp);
  return WORLD_SUCESS;
#  endif                               /* USE_PAL_RANGES */
#endif                                 /* !MSWINDOWS */
}
