/****************************************************************************
 * apps/graphics/traveler/src/trv_pcx.c
 * PCX graphic file support
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
#include "trv_bitmaps.h"
#include "trv_graphicfile.h"
#include "trv_pcx.h"

#include <stdio.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_load_pcxheader
 *
 * Description:
 *
 ****************************************************************************/

static void trv_load_pcxheader(FAR FILE *fp, struct pcx_header_s *header)
{
  FAR uint8_t *tmpbuffer;
  int i;

  /* Load the header */

  tmpbuffer = (FAR uint8_t*)header;

  for (i = 0; i < SIZEOF_PCX_HEADER; i++)
    {
      tmpbuffer[i] = getc(fp);
    }
}

/****************************************************************************
 * Name: trv_load_pcxdata
 *
 * Description:
 *
 ****************************************************************************/

static void trv_load_pcxdata(FAR FILE *fp, int32_t imagesize,
                             FAR uint8_t *imagebuffer)
{
  uint32_t count;
  int16_t nbytes;
  uint8_t data;

  count = 0;
  while(count <= imagesize)
    {
      /* Get the first piece of data */

      data = getc(fp);

      /* Is this a run length encoding? */

      if ((data >= 192) /* && (data <= 255) */)
        {
          /* How many bytes in run? */

          nbytes = data - 192;

          /* Get the actual data for the run */

          data  = getc(fp);

          /* Replicate data in buffer nbytes times */

          while(nbytes-- > 0)
            {
              imagebuffer[count++] = data;
            }
        }
      else
        {
          /* Actual data, just copy it into buffer at next location */

          imagebuffer[count++] = data;
        }
    }
}

/****************************************************************************
 * Name: trv_load_pcxpalette
 *
 * Description:
 *
 ****************************************************************************/

static void trv_load_pcxpalette(FAR FILE *fp,
                                FAR struct trv_color_rgb_s *palette)
{
  int i;

  /* Move to end of file then back up 768 bytes i.e. to beginning of palette */

  (void)fseek(fp, -768L, SEEK_END);

  /* Load the PCX palette into the palette structure */

  for (i = 0; i < 256; i++)
    {
      /* Get the RGB components */

      palette[i].red   = getc(fp);
      palette[i].green = getc(fp);
      palette[i].blue  = getc(fp);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_load_pcx
 *
 * Description:
 *   This function loads a PCX file into a memory.
 *
 ****************************************************************************/

FAR struct trv_graphicfile_s *trv_load_pcx(FAR FILE *fp,
                                           FAR const char *filename)
{
  struct pcx_header_s header;
  FAR trv_pixel_t *buffer;
  FAR struct trv_graphicfile_s *gfile;
  FAR struct trv_color_rgb_s *palette;
  uint16_t imagewidth;
  uint16_t imageheight;
  uint32_t imagesize;

  /* Load the PCX Header */

  trv_load_pcxheader(fp, &header);

  /* Allocate Space to hold the image data */

  imagewidth  = header.width - header.x + 1;
  imageheight = header.height - header.y + 1;
  imagesize   = imageheight * imagewidth * sizeof(trv_pixel_t);
  buffer      = (FAR trv_pixel_t*)trv_malloc(imagesize + 1);

  /* Load the PCX data into the buffer */

  trv_load_pcxdata(fp, imagesize, (FAR uint8_t*)buffer);

  /* Allocate space to hold the PCX palette */

  palette = (struct trv_color_rgb_s*)
    trv_malloc(sizeof(struct trv_color_rgb_s) * 256);

  gfile = NULL;
  if (palette)
    {
      /* Load the PCX palette */

      trv_load_pcxpalette(fp, palette);

      /* Now save the resulting data in a struct trv_graphicfile_s structure */

      gfile = trv_graphicfile_new();
      if (gfile)
        {
          gfile->palette = palette;
          gfile->width = imagewidth;
          gfile->height = imageheight;
          gfile->bitmap = buffer;
        }
    }

  return gfile;
}

