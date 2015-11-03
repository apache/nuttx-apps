/****************************************************************************
 * apps/graphics/traveler/src/trv_paltbl.c
 * This file contains the logic that creates the range palette table that is
 * used to modify the palette with range to hit
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
 * Included files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_mem.h"
#include "trv_color.h"
#include "trv_fsutils.h"
#include "trv_world.h"
#include "trv_bitmaps.h"
#include "trv_paltable.h"

#include <stdio.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This indicates the maximum number of palette ranges which may be
 * specified in the file
 */

#ifdef CONFIG_GRAPHICS_TRAVELER_PALRANGES
# define MAX_PAL_RANGES 64
#endif

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_PALRANGES
struct trv_palrange_s
{
  uint8_t firstcolor;
  int8_t  colorrange;
  uint8_t clipColor;
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This is the palette table which is used to adjust the texture values
 * with distance
 */

trv_pixel_t *g_paltable[NUM_ZONES];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_allocate_paltbl
 *
 * Description:
 *
 ****************************************************************************/

static int trv_allocate_paltbl(uint32_t entrysize)
{
  int i;

  for (i = 0; i < NUM_ZONES; i++)
    {
      g_paltable[i] = (FAR trv_pixel_t*)
        trv_malloc(entrysize * sizeof(trv_pixel_t));

      if (g_paltable[i] == NULL)
        {
          return -ENOMEM;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_load_paltable
 *
 * Description:
 *   This function loads the g_paltable from the specified file
 *
 ****************************************************************************/

int trv_load_paltable(FAR const char *file)
{
#ifdef CONFIG_GRAPHICS_TRAVELER_PALRANGES
  struct trv_palrange_s ranges[MAX_PAL_RANGES];
  trv_pixel_t plotcolor;
  FAR trv_pixel_t *palptr;
  FAR FILE  *fp;
  int16_t nranges;
  int16_t zone;
  int16_t palndx;
  int ret;
  int i;

  /* Open the file which contains the palette table */

  fp = fopen(file, "r");
  if (!fp)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", file, errcode);
      return -errcode;
    }

  /* Read the number of ranges from the file */

  nranges = trv_read_decimal(fp);
  if (nranges > MAX_PAL_RANGES)
    {
      fprintf(stderr, "ERROR: Too many ranges: %d\n", nranges);
      fclose(fp);
      return -E2BIG;
    }

  /* Then read all of the palette range data from the file */

  for (i = 0; i < nranges; i++)
    {
      ranges[i].firstcolor = trv_read_decimal(fp);
      ranges[i].colorrange = trv_read_decimal(fp);
      ranges[i].clipColor  = trv_read_decimal(fp);
    }

  /* We are now done with the input file */

  fclose(fp);

  /* Allocate memory to hold the palette range mapping data */

  ret = trv_allocate_paltbl(PALETTE_SIZE);
  if (ret < 0)
    {
      fprintf(stderr,
              "ERROR: Failed to allocate range mapping table: %d\n",
              ret);
      return ret;
    }

  /* Process each distance zone */

  for (zone = 0; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palptr = g_paltable[zone];

      /* Process each possible palette index within the zone */

      for (palndx = 0; palndx < PALETTE_SIZE; palndx++)
        {
          /* Assume that the range will not be found.  In this case, we
           * will perform the 1-to-1 mapping
           */

          plotcolor = palndx;

          /* Find the range the color is in. */

          for (i = 0; i < nranges; i++)
            {
              /* Check if the color range is ascending or descending */

              if (ranges[i].colorrange < 0)
                {
                  /* The colors are descending */

                  if ((palndx <= ranges[i].firstcolor) &&
                      (palndx > (ranges[i].firstcolor + ranges[i].colorrange)))
                    {
                      /* Found it, set the new plotcolor */

                      if (plotcolor > zone)
                        {
                          /* Offset the color by the zone we are processing */

                          plotcolor -= zone;

                          /* Check if we have exceeded the range of this
                           * color.  If so, then set the color to the
                           * clipColor
                           */

                          if (plotcolor <= ranges[i].firstcolor + ranges[i].colorrange)
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

              else if ((palndx >= ranges[i].firstcolor) &&
                       (palndx < (ranges[i].firstcolor + ranges[i].colorrange)))
                {
                  /* Found it, set the new plotcolor */

                  plotcolor += zone;

                  /* Check if we have exceeded the range of this color.  If so,
                   * then set the color to black
                   */

                  if (plotcolor >= ranges[i].firstcolor + ranges[i].colorrange)
                    {
                      plotcolor = ranges[i].clipColor;
                    }

                  /* Now break out of the loop */

                  break;
                }
            }

          /* Save the plotcolor in the g_paltable */

          palptr[palndx] = plotcolor;

        }
    }

  return OK;
#else
  FILE  *fp;
  int16_t zone;
  int16_t palndx;
  FAR uint8_t *palptr;
  int ret;

  /* Allocate memory to hold the palette range mapping data */

  ret = trv_allocate_paltbl(PALETTE_SIZE);
  if (ret < 0)
    {
      fprintf(stderr,
              "ERROR: Failed to allocate range mapping table: %d\n",
              ret);
      return ret;
    }

  /* Open the file which contains the palette table */

  fp = fopen(file, "r");
  if (!fp)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", file, errcode);
      return -errcode;
    }

  /* Process each distance zone */

  for (zone = 0; zone < NUM_ZONES; zone++)
    {
      /* Point to the palette mapping data for this zone */

      palptr = GET_PALPTR(zone);

      /* Process each possible palette index within the zone */

      for (palndx = 0; palndx < PALETTE_SIZE; palndx++)
        {
          /* Read the data into g_paltable */

          palptr[palndx] = trv_read_decimal(fp);
        }
    }

  fclose(fp);
  return OK;
#endif /* CONFIG_GRAPHICS_TRAVELER_PALRANGES */
}

/****************************************************************************
 * Function: trv_release_paltable
 *
 * Description:
 *
 ****************************************************************************/

void trv_release_paltable(void)
{
  int i;

  for (i = 0; i < NUM_ZONES; i++)
    {
      if (g_paltable[i])
        {
          trv_free(g_paltable[i]);
          g_paltable[i] = NULL;
        }
    }
}
