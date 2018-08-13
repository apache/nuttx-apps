/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_readtexturefile.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "trv_types.h"
#include "wld_bitmaps.h"
#include "wld_color.h"
#include "wld_mem.h"
#include "wld_graphicfile.h"
#include "wld_utils.h"

/*************************************************************************
 * Pre-procssor Definitions
 *************************************************************************/

#define COLOR_LEVELS 6

#undef MIN
#undef MAX
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_log2
 *
 * Description:
 *   Return the log base 2 of the argument, or -1 if the argument is not
 *   an integer power of 2.
 *
 ************************************************************************/

static int wld_log2(int x)
{
  int i;
  unsigned int n;

  if (x <= 0)
    {
      return -1;
    }
  else
    {
      n = (unsigned int)x;
    }

  for (i = 0; (n & 0x1) == 0; i++, n >>= 1);

  if (n == 1)
    {
      return i;
    }
  else
    {
      return -1;
    }
}

/*************************************************************************
 * Name: wld_new_texture
 * Description:
 ************************************************************************/

static wld_bitmap_t *wld_new_texture(uint16_t width, uint16_t height)
{
  wld_bitmap_t *t;

  if (height <= 0 || width <= 0)
    {
      wld_fatal_error("wld_new_texture:  bad texture dimensions");
    }

  t = (wld_bitmap_t *) wld_malloc(sizeof(wld_bitmap_t));
  t->bm = (wld_pixel_t *) wld_malloc(height * width * sizeof(wld_pixel_t));

  t->w = width;
  t->h = height;
  t->log2h = wld_log2(height);

  return t;
}

/*************************************************************************
 * Name: wld_quantize_texture
 * Description:
 ************************************************************************/

static void wld_quantize_texture(graphic_file_t * gFile, wld_bitmap_t * t)
{
  color_rgb_t pixel;
  wld_pixel_t *dest = t->bm;
  int x;
  int y;

  for (x = gFile->width - 1; x >= 0; x--)
    {
      for (y = gFile->height - 1; y >= 0; y--)
        {
          pixel = wld_graphicfile_pixel(gFile, x, y);
          *dest++ = wld_rgb2pixel(&pixel);
        }
    }
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_read_texturefile
 * Description:
 ************************************************************************/

wld_bitmap_t *wld_read_texturefile(char *filename)
{
  graphic_file_t *gFile;
  wld_bitmap_t *t;

  gFile = wld_readgraphic_file(filename);
  if (gFile == NULL)
    {
      wld_fatal_error("ERROR: Reading texture %s.", filename);
    }

  /* The height and width should be powers of two for efficient texture
   * mapping.  Here, we enforce this. */

  if (wld_log2(gFile->width) == -1 || wld_log2(gFile->height) == -1)
    {
      wld_fatal_error("Dimensions texture %s are not powers of two.", filename);
    }

  t = wld_new_texture(gFile->width, gFile->height);
  wld_quantize_texture(gFile, t);

  wld_free_graphicfile(gFile);

  return t;
}
