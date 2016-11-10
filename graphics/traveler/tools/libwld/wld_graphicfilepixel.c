/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_graphicfilepixel.c
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

#include "trv_types.h"
#include "wld_bitmaps.h"
#include "wld_graphicfile.h"

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_graphicfile_pixel
 * Description:
 ************************************************************************/

color_rgb_t wld_graphicfile_pixel(graphic_file_t *gfile, int x, int y)
{
  if (gfile->type == GFILE_PALETTED)
    {
      int i;

      i = gfile->bitmap[y * gfile->width + x];
      return gfile->palette[i];
    }
  else
    {
      unsigned char *pixel = gfile->bitmap + (y * gfile->width + x) * 3;
      color_rgb_t rgb;

      rgb.red = *pixel++;
      rgb.green = *pixel++;
      rgb.blue = *pixel;

      return rgb;
    }
}
