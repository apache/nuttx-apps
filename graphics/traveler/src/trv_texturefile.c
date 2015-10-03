/****************************************************************************
 * apps/graphics/traveler/src/trv_texturefile.c
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
#include "trv_bitmaps.h"
#include "trv_mem.h"
#include "trv_color.h"
#include "trv_graphicfile.h"
#include "trv_fsutils.h"

#include <stdio.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define COLOR_LEVELS 6

#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_log2
 *
 * Description:
 *   Return the log base 2 of the argument, or -1 if the argument is not
 *   an integer power of 2.
 *
 ****************************************************************************/

static int trv_log2(uint16_t x)
{
  int i;

  if (x != 0)
    {
      for (i = 0; i < 16 && (x & 1) == 0; i++, x >>= 1);
    }

  return (x == 1) ? i : -EINVAL;
}

/****************************************************************************
 * Name: trv_new_texture
 *
 * Description:
 ****************************************************************************/

static FAR struct trv_bitmap_s *
trv_new_texture(uint16_t width, uint16_t height)
{
  struct trv_bitmap_s *bitmap;
  int log2h;
  int log2w;

  /* The height and width should be powers of two for efficient texture
   * mapping.  Here, we enforce this.
   */

  log2w = trv_log2(width);
  if (log2w < 0)
    {
      fprintf(stderr, "ERROR: Width is not a power of 2: %u\n", width);
      return NULL;
    }

  log2h = trv_log2(height);
  if (log2h < 0)
    {
      fprintf(stderr, "ERROR: Height is not a power of 2: %u\n", height);
      return NULL;
    }

  /* Allocate the bitmap container */

  bitmap = (FAR struct trv_bitmap_s*)trv_malloc(sizeof(struct trv_bitmap_s));
  if (!bitmap)
    {
      fprintf(stderr, "ERROR: Failed to allocate bitmap structure\n",
              height, width);
      return NULL;
    }

  /* Allocate the bitmap texture memory */

  bitmap->bm = (trv_pixel_t*)trv_malloc(height * width * sizeof(trv_pixel_t));
  if (!bitmap->bm)
    {
      fprintf(stderr, "ERROR: Failed to allocate bitmap: %u x %u x %u\n",
              height, width, sizeof(trv_pixel_t));
      trv_free(bitmap);
      return NULL;
    }

  /* Initialize the bitmap container */

  bitmap->w     = width;
  bitmap->h     = height;
  bitmap->log2h = log2h;

  return bitmap;
}

/****************************************************************************
 * Name: trv_quantize_texture
 * Description:
 ****************************************************************************/

static void trv_quantize_texture(FAR struct trv_graphicfile_s *gfile,
                                 FAR struct trv_bitmap_s *bitmap)
{
  FAR trv_pixel_t *destpixel = bitmap->bm;
  struct trv_color_rgb_s pixel;
  int x;
  int y;

  for (x = gfile->width - 1; x >= 0; x--)
    {
      for (y = gfile->height - 1; y >= 0; y--)
        {
          pixel        = trv_graphicfile_pixel(gfile, x, y);
          *destpixel++ = trv_color_rgb2pixel(&pixel);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_read_texture
 *
 * Description:
 *
 ****************************************************************************/

FAR struct trv_bitmap_s *trv_read_texture(FAR const char *filename)
{
  FAR struct trv_graphicfile_s *gfile;
  FAR struct trv_bitmap_s *bitmap;

  gfile = tvr_graphicfile_read(filename);
  if (gfile == NULL)
    {
      fprintf(stderr, "ERROR: Read failed for texture %s\n", filename);
      return NULL;
    }

  bitmap = trv_new_texture(gfile->width, gfile->height);
  if (bitmap)
    {
      trv_quantize_texture(gfile, bitmap);
    }

  trv_graphicfile_free(gfile);
  return bitmap;
}
