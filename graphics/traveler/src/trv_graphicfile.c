/****************************************************************************
 * apps/graphics/traveler/src/trv_graphicfile.c
 * Load image from a graphic file
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
#include "trv_pcx.h"
#include "trv_graphicfile.h"

#include <stdio.h>
#include <errno.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tvr_graphicfile_read
 *
 * Description:
 *   Read a bitmap texture from a graphic file
 *
 *   NOTE:  This function exists only as extension pointer where additional
 *   graphic file formats can be supported.  Currently only the ancient
 *   PCX format is supported.
 *
 ****************************************************************************/

FAR struct trv_graphicfile_s *tvr_graphicfile_read(char *filename)
{
  FAR struct trv_graphicfile_s *gfile = NULL;
  FAR FILE *fp;

  /* Open the graphic file for reading */

  fp = fopen(filename, "rb");
  if (fp == NULL)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open graphic file %s: %d\n",
              filename, errcode);
      return NULL;
    }

  /* Load the graphic file into memory.
   *
   * REVISIT: Here would be the place where we would decide on the format of
   * the graphic file.  Here we just assume that the file is PCX.
   */

  gfile = trv_load_pcx(fp, filename);
  if (gfile == NULL)
    {
      fprintf(stderr, "ERROR: Failed to load graphic file %s\n", filename);
    }

  fclose(fp);
  return gfile;
}

/****************************************************************************
 * Name: trv_graphicfile_new
 *
 * Description:
 *   Allocate a new graphic file structure
 *
 ****************************************************************************/

FAR struct trv_graphicfile_s *trv_graphicfile_new(void)
{
  FAR struct trv_graphicfile_s *gfile;


  gfile = (FAR struct trv_graphicfile_s*)
    trv_malloc(sizeof(struct trv_graphicfile_s));
  if (gfile)
    {
      gfile->transparent_entry = -1;
    }

  return gfile;
}

/****************************************************************************
 * Name: trv_graphicfile_free
 *
 * Description:
 *   Free the graphic file structure after also freeing in additional
 *   resources attached to the structure.

 ****************************************************************************/

void trv_graphicfile_free(FAR struct trv_graphicfile_s *gfile)
{
  if (gfile != NULL)
    {
      if (gfile->palette != NULL)
        {
          trv_free(gfile->palette);
        }

      if (gfile->bitmap != NULL)
        {
          trv_free(gfile->bitmap);
        }

      trv_free(gfile);
    }
}

/****************************************************************************
 * Name: trv_graphicfile_pixel
 *
 * Description:
 *   Return the RGB color value for a pixel at location (x,y) in the
 *   texture bitmap.
 *
 ****************************************************************************/

struct trv_color_rgb_s
trv_graphicfile_pixel(FAR struct trv_graphicfile_s *gfile, int x, int y)
{
  /* REVISIT:  This only works for paletted graphics files like PCX.
   * For the paletted color, the color lookup values lies at the (x,y)
   * position in the image.  For a true color image, the RGB value would
   * lie at the (x,y) position,
   */

  int ndx = gfile->bitmap[y * gfile->width + x];
  return gfile->palette[ndx];
}
