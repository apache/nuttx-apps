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

/* If the following switch is false, then the whole palTable is loaded from
 * the file.  Otherwise, the palTable will be calculated from a range
 * table
 */

#define USE_PAL_RANGES true

#include "trv_types.h"
#include "wld_world.h"
#include "wld_paltable.h"
#if (!MSWINDOWS)
#include "wld_bitmaps.h"
#include "x11color.h"
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
# define MAX_PAL_RANGES 64
#endif

/*************************************************************************
 * Private Type Declarations
 ************************************************************************/

#if USE_PAL_RANGES
typedef struct
{
  uint8_t firstColor;
  sbyte colorRange;
  uint8_t clipColor;
} palRangeType;
#endif

/*************************************************************************
 * Public Data
 *************************************************************************/

/* This is the palette table which is used to adjust the texture values
 * with distance
 */

trv_pixel_t *palTable[NUM_ZONES];

/*************************************************************************
 * Private Functions
 *************************************************************************/


/*************************************************************************
 * Name: wld_AllocatePalTable
 * Description:
 ************************************************************************/

static void wld_AllocatePalTable(uint32_t palTabEntrySize)
{
  int i;

  for (i = 0; i < NUM_ZONES; i++)
    {
      palTable[i] = (trv_pixel_t*)wld_malloc(palTabEntrySize*sizeof(trv_pixel_t));
    }
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_load_paltable
 * Description:
 * This function loads the palTable from the specified file
 ************************************************************************/

uint8_t wld_load_paltable(char *file)
{
#if (!MSWINDOWS)
  trv_pixel_t *palPtr;
  ColorLumType lum;
  int16_t  zone;
  float scale;
  int pixel;

  /* Allocate memory to hold the palette range mapping data */

  wld_AllocatePalTable(MAX_PIXEL_TYPE+1);

  /* The first zone is just the identity transformation */

  palPtr = palTable[0];
  for (pixel = 0; pixel <= MAX_PIXEL_TYPE; pixel++)
    {
      *palPtr++ = pixel;
    }

  /* Process each remaining distance zone */

  for (zone = 1; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palPtr = palTable[zone];

      /* Calculate the luminance scaling factor to use for this zone */

      scale = (float)(NUM_ZONES - zone) / (float)NUM_ZONES;

      /* Scale each element of the palette */

      for (pixel = 0; pixel <= MAX_PIXEL_TYPE; pixel++)
        {
          /* Get the unit vector + luminance representation of the pixel */

          wld_Pixel2Lum(pixel, &lum);

          /* Re-scale the luminance component by range scale factor */

          lum.luminance *= scale;

          /* Then save the pixel associated with this scaled value in
           * the range palette table
           */

          *palPtr++ = wld_Lum2Pixel(&lum);

        }
    }
  return WORLD_SUCCESS;

#else
#if USE_PAL_RANGES
  FILE  *fp, *fopen();
  int16_t  i;
  int16_t  numRanges;
  int16_t  zone;
  int16_t  palIndex;
  trv_pixel_t plotColor;
  trv_pixel_t *palPtr;
  palRangeType ranges[MAX_PAL_RANGES];

  /* Open the file which contains the palette table */

  fp = fopen(file, "r");
  if (!fp) return PALR_FILE_OPEN_ERROR;

  /* Read the number of ranges from the file */

  numRanges = wld_read_decimal(fp);
  if (numRanges > MAX_PAL_RANGES)
    {
      fclose(fp);
      return PALR_TOO_MANY_RANGES;
    }

  /* Then read all of the palette range data from the file */

  for (i = 0; i < numRanges; i++)
    {
      ranges[i].firstColor = wld_read_decimal(fp);
      ranges[i].colorRange = wld_read_decimal(fp);
      ranges[i].clipColor  = wld_read_decimal(fp);
    }

  /* We are now done with the input file */

  fclose(fp);

  /* Allocate memory to hold the palette range mapping data */

  wld_AllocatePalTable(PALETTE_SIZE);

  /* Process each distance zone */

  for (zone = 0; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palPtr = palTable[zone];

      /* Process each possible palette index within the zone */

      for (palIndex = 0; palIndex < PALETTE_SIZE; palIndex++)
        {
          /* Assume that the range will not be found.  In this case, we
           * will perform the 1-to-1 mapping
           */

          plotColor = palIndex;

          /* Find the range the color is in. */

          for (i = 0; i < numRanges; i++)
            {
              /* Check if the color range is ascending or descending */

              if (ranges[i].colorRange < 0)
                {
                  /* The colors are descending */
                  if ((palIndex <= ranges[i].firstColor) &&
                      (palIndex > (ranges[i].firstColor + ranges[i].colorRange)))
                    {
                      /* Found it, set the new plotColor */

                      if (plotColor > zone)
                        {
                          /* Offset the color by the zone we are processing */

                          plotColor -= zone;

                          /* Check if we have exceeded the range of this
                           * color.  If so, then set the color to the
                           * clipColor
                           */

                          if (plotColor <= ranges[i].firstColor + ranges[i].colorRange)
                            {
                              plotColor = ranges[i].clipColor;
                            }
                        }
                      else
                        {
                          plotColor = ranges[i].clipColor;
                        }

                      /* Now break out of the loop */

                      break;
                    }
                }

              /* The colors are ascending */

              else if ((palIndex >= ranges[i].firstColor) &&
                       (palIndex < (ranges[i].firstColor + ranges[i].colorRange)))
                {
                  /* Found it, set the new plotColor */

                  plotColor += zone;

                  /* Check if we have exceeded the range of this color.  If so,
                   * then set the color to black
                   */

                  if (plotColor >= ranges[i].firstColor + ranges[i].colorRange)
                    {
                      plotColor = ranges[i].clipColor;
                    }

                  /* Now break out of the loop */

                  break;
                }
            }

          /* Save the plotColor in the palTable */

          palPtr[palIndex] = plotColor;

        }
    }

  return WORLD_SUCCESS;
#else
  FILE  *fp, *fopen();
  int16_t  zone;
  int16_t  palIndex;
  uint8_t plotColor;
  uint8_t *palPtr;

  /* Allocate memory to hold the palette range mapping data */

  wld_AllocatePalTable();

  /* Open the file which contains the palette table */

  fp = fopen(file, "r");
  if (!fp) return PALR_FILE_OPEN_ERROR;

  /* Process each distance zone */

  for (zone = 0; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palPtr = GET_PALPTR(zone);

      /* Process each possible palette index within the zone */

      for (palIndex = 0; palIndex < PALETTE_SIZE; palIndex++)
        {
          /* Read the data into palTable */

          palPtr[palIndex] = wld_read_decimal(fp);
        }
    }

  fclose(fp);
  return WORLD_SUCESS;
#endif /* USE_PAL_RANGES */
#endif /* !MSWINDOWS */
}
