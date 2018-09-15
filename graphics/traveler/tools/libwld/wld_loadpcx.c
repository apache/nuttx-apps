/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_loadpcx.c
 * This file contains the logic which loads a PCX file.
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

#include <stdio.h>
#include <malloc.h>
#include "trv_types.h"
#include "wld_mem.h"
#if (!MSWINDOWS)
#  include "wld_bitmaps.h"
#  include "wld_graphicfile.h"
#endif
#include "wld_pcx.h"

/*************************************************************************
 * Private Function Prototypes
 *************************************************************************/

static void wld_load_pcxheader(FILE * fp, pcx_header_t * header);
static void wld_load_pcxdata(FILE * fp, int32_t imagSize,
                             uint8_t * imageBuffer);
static void wld_load_pcxpalette(FILE * fp, color_rgb_t * palette);

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_load_pcxheader
 * Description:
 ************************************************************************/

static void wld_load_pcxheader(FILE * fp, pcx_header_t * header)
{
  uint8_t *tempBuffer;
  int i;

  /* Load the header */

  tempBuffer = (uint8_t *) header;

  for (i = 0; i < SIZEOF_PCX_HEADER; i++)
    {
      tempBuffer[i] = getc(fp);
    }
}

/*************************************************************************
 * Name: wld_load_pcxdata
 * Description:
 ************************************************************************/

static void wld_load_pcxdata(FILE * fp, int32_t imageSize, uint8_t * imageBuffer)
{
  uint32_t count;
  int16_t numBytes;
  uint8_t data;

  count = 0;
  while (count <= imageSize)
    {
      /* Get the first piece of data */

      data = getc(fp);

      /* Is this a run length encoding? */

      if ((data >= 192) /* && (data <= 255) */ )
        {
          /* How many bytes in run? */

          numBytes = data - 192;

          /* Get the actual data for the run */

          data = getc(fp);

          /* Replicate data in buffer numBytes times */

          while (numBytes-- > 0)
            {
              imageBuffer[count++] = data;
            }
        }
      else
        {
          /* Actual data, just copy it into buffer at next location */

          imageBuffer[count++] = data;
        }
    }
}

/*************************************************************************
 * Name: wld_load_pcxpalette
 * Description:
 ************************************************************************/

static void wld_load_pcxpalette(FILE * fp, color_rgb_t * palette)
{
  int i;

  /* Move to end of file then back up 768 bytes i.e. to begining of palette */

  fseek(fp, -768L, SEEK_END);

  /* Load the PCX palette into the palette structure */

  for (i = 0; i < 256; i++)
    {
      /* Get the RGB components */

#if MSWINDOWS
      palette[i].red = (getc(fp) >> 2);
      palette[i].green = (getc(fp) >> 2);
      palette[i].blue = (getc(fp) >> 2);
#else
      palette[i].red = getc(fp);
      palette[i].green = getc(fp);
      palette[i].blue = getc(fp);
#endif
    }
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_loadpcx
 * Description:
 * This function loads a pcx file into a picture structure, the actual image
 * data for the pcx file is decompressed and expanded into a secondary buffer
 * within the picture structure, the separate images can be grabbed from this
 * buffer later.  also the header and palette are loaded
 ************************************************************************/

#if MSWINDOWS
uint8_t wld_loadpcx(char *filename, pcxPicturePtr image)
{
  FILE *fp, *fopen();
  uint16_t imageWidth, imageHeight;
  uint32_t imageSize;

  /* Open the file */

  fp = fopen(filename, "rb");
  if (!fp)
    {
      return PCX_OPEN_ERROR;
    }

  /* Load the PCX Header */

  wld_load_pcxheader(fp, &image->header);

  /* Load the PCX data */

  if (image->buffer)
    {
      imageWidth = image->header.width - image->header.x + 1;
      imageHeight = image->header.height - image->header.y + 1;
      imageSize = imageHeight * imageWidth;
      wld_load_pcxdata(fp, imageSize, image->buffer);
    }

  /* Load the PCX palette */

  if (image->palette)
    {
      wld_load_pcxpalette(fp, &image->palette);
    }

  fclose(fp);
  return PCX_SUCCESS;
}
#else
graphic_file_t *wld_loadpcx(FILE * fp, char *filename)
{
  pcx_header_t header;
  wld_pixel_t *buffer;
  graphic_file_t *gFile;
  color_rgb_t *palette;
  uint16_t imageWidth, imageHeight;
  uint32_t imageSize;

  /* Load the PCX Header */

  wld_load_pcxheader(fp, &header);

  /* Allocate Space to hold the image data */

  imageWidth = header.width - header.x + 1;
  imageHeight = header.height - header.y + 1;
  imageSize = imageHeight * imageWidth * sizeof(wld_pixel_t);
  buffer = (wld_pixel_t *) wld_malloc(imageSize + 1);

  /* Load the PCX data into the buffer */

  wld_load_pcxdata(fp, imageSize, (uint8_t *) buffer);

  /* Allocate space to hold the PCX palette */

  palette = (color_rgb_t *) wld_malloc(sizeof(color_rgb_t) * 256);

  /* Load the PCX palette */

  wld_load_pcxpalette(fp, palette);

  /* Now save the resulting data in a graphic_file_t structure */

  gFile = wld_new_graphicfile();
  gFile->type = GFILE_PALETTED;
  gFile->palette = palette;
  gFile->width = imageWidth;
  gFile->height = imageHeight;
  gFile->bitmap = buffer;

  return gFile;
}
#endif
