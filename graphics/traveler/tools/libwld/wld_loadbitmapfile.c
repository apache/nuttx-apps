/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_loadbitmapfile.c
 * This file contains the logic which loads texture bitmaps
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

#include <stdbool.h>
#include <stdio.h>

#include "trv_types.h"
#include "wld_mem.h"
#include "wld_world.h"
#include "wld_bitmaps.h"
#if (MSWINDOWS)
#  include "wld_pcx.h"
#endif
#include "wld_utils.h"

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_read_filename
 * Description:
 * Read a file name from the input stream
 ************************************************************************/

static bool wld_read_filename(FILE * fp, char *fileName)
{
  int16_t nbytes;
  int ch;

  /* Skip over any leading spaces, new lines, or carriage returns (for
   * MSDOS compatibility)
   */

  do
    {
      ch = getc(fp);
      if (ch == EOF)
        return false;
    }
  while ((ch == ' ') || (ch == '\n') || (ch == '\r'));

  /* Get the file name from the file */

  nbytes = 0;
  for (;;)
    {
      /* Everything up to the next newline or space must be the filename */

      if ((ch != '\n') && (ch != ' ') && (ch != '\r'))
        {
          /* Make sure that the file name is not too large */

          if (nbytes >= FILE_NAME_SIZE)
            {
              return false;
            }

          /* Add the new character to the file name */

          fileName[nbytes] = (char)ch;
          nbytes++;
        }
      else
        {
          /* End of the file name -- Don't forget the ASCIIZ terminator */

          fileName[nbytes] = '\0';
          break;
        }

      /* Get the character for the next time through the loop.  Every file name
       * should be terminated with a space or a new line.  EOF is unexpected in
       * this context. */

      ch = getc(fp);
      if (ch == EOF)
        {
          return false;
        }
    }

  return true;
}

/*************************************************************************
 * Name: wld_load_bitmaps
 * Description:
 * This function loads the world data from the input file
 ************************************************************************/

static uint8_t wld_load_bitmaps(FILE * fp)
{
#if MSWINDOWS
  volatile pcx_picture_t pcx;
  color_rgb_t *palette;
#endif
  uint16_t mapndx;
  uint8_t result = BMAP_SUCCESS;
  char filename[FILE_NAME_SIZE];

  /* Discard any bitmaps that we may currently have buffered */

  wld_discard_bitmaps();

  /* Get the number of bitmaps in the bitmap file */

  g_nbitmaps = wld_read_decimal(fp);
  if (g_nbitmaps >= MAX_BITMAPS)
    return BMAP_TOO_MANY;

  /* Read the colors used to rend the sky and ground */

  g_sky_color = wld_read_decimal(fp);
  g_ground_color = wld_read_decimal(fp);

#if MSWINDOWS
  /* Load the textures -- Note that the first texture will be used to define
   * the worldPalette */

  palette = worldPalette;
#endif

  /* Load each bitmap file */

  for (mapndx = 0; (mapndx < g_nbitmaps && result == BMAP_SUCCESS); mapndx++)
    {
      /* Load the even bitmap */
      /* Get the name of the file which contains the even bitmap */

      if (!wld_read_filename(fp, filename))
        {
          return BMAP_BML_READ_ERROR;
        }

#if MSWINDOWS
      /* Setup to load the PCX bitmap from the file for the event bitmap */

      result = wld_pcx_init(&pcx, BITMAP_HEIGHT, BITMAP_WIDTH,
                            palette, NULL);
      if (result)
        {
          return result;
        }

      /* Put the bitmap buffer into the evenBitmap list */

      g_even_bitmaps[mapndx].w = BITMAP_WIDTH;
      g_even_bitmaps[mapndx].h = BITMAP_HEIGHT;
      g_even_bitmaps[mapndx].log2h = BITMAP_LOG2H;
      g_even_bitmaps[mapndx].bm = pcx.buffer;

      /* Load the PCX bitmap from the file for the event bitmap */

      result = wld_loadpcx(filename, &pcx);
      if (result)
        {
          return result;
        }

      /* Don't bother to load the palette on the rest of the textures -- we
       * will assume the same palette for all textures. */

      palette = NULL;
#else
      g_even_bitmaps[mapndx] = wld_read_texturefile(filename);
      if (!g_even_bitmaps[mapndx])
        {
          return BMAP_BML_READ_ERROR;
        }
#endif

      /* Load the odd bitmap */
      /* Get the name of the file which contains the odd bitmap */

      if (!wld_read_filename(fp, filename))
        {
          return BMAP_BML_READ_ERROR;
        }

#ifndef WEDIT
#  if MSWINDOWS
      /* Setup to load the PCX bitmap from the file for the odd bitmap */

      result = wld_pcx_init(&pcx, BITMAP_HEIGHT, BITMAP_WIDTH,
                            palette, NULL);
      if (result)
        {
          return result;
        }

      /* Put the bitmap buffer into the oddBitmap list */

      g_odd_bitmaps[mapndx].w = BITMAP_WIDTH;
      g_odd_bitmaps[mapndx].h = BITMAP_HEIGHT;
      g_odd_bitmaps[mapndx].log2h = BITMAP_LOG2H;
      g_odd_bitmaps[mapndx].bm = pcx.buffer;

      /* Load the PCX bitmap from the file for the odd bitmap */

      result = wld_loadpcx(filename, &pcx);
#  else
      g_odd_bitmaps[mapndx] = wld_read_texturefile(filename);
      if (!g_odd_bitmaps[mapndx])
        {
          return BMAP_BML_READ_ERROR;
        }
#  endif
#endif
    }

  return result;
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_load_bitmapfile
 * Description:
 * This function opens the input file and loads the world data from it
 ************************************************************************/

uint8_t wld_load_bitmapfile(char *bmlFile)
{
  FILE *fp;
  uint8_t result;

  /* Open the file which contains the names of all of the bitmap files */

  fp = fopen(bmlFile, "r");
  if (!fp)
    return BMAP_BML_OPEN_ERROR;

  /* Load all of the bitmaps */

  result = wld_load_bitmaps(fp);
  if (result)
    wld_discard_bitmaps();

  /* We are all done with the file, so close it */

  fclose(fp);
  return result;
}
