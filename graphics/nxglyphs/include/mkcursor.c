/****************************************************************************
 * apps/graphics/nxglyphs/include/mkcursor.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct rgb_s
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct rgb_s pixels[1024];
static int npixels = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int rgb2gray2(unsigned char r, unsigned char g, unsigned char b)
{
  uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;

  if (rgb == BGCOLOR)
    {
      return 0;
    }
  else if (rgb == FGCOLOR1)
    {
      return 1;
    }
  else if (rgb == FGCOLOR2)
    {
      return 2;
    }
  else if (rgb == FGCOLOR3)
    {
      return 3;
    }
  else
    {
      fprintf(stderr, "ERROR: Unrecognized color (%u,%u,%u), 0x%06lx\n",
              r, g, b, (unsigned long)rgb);
      fprintf(stderr, "       Using BGCOLOR\n");
      return 0;
    }
}

int main(int argc, char **argv, char **envp)
{
  const unsigned char *ptr;
  unsigned char r;
  unsigned char g;
  unsigned char b;
  uint32_t code;
  int i;
  int j;

  printf("#include <nuttx/config.h>\n\n");
  printf("#include <stdint.h>\n\n");
  printf("#include <nuttx/video/rgbcolors.h>\n");
  printf("#include <nuttx/nx/nxcursor.h>\n\n");

  printf("#if CONFIG_NXWIDGETS_BPP == 8\n");
  printf("#  define FGCOLOR1             RGB8_WHITE\n");
  printf("#  define FGCOLOR2             RGB8_BLACK\n");
  printf("#  define FGCOLOR3             RGB8_GRAY\n");
  printf("#elif CONFIG_NXWIDGETS_BPP == 16\n");
  printf("#  define FGCOLOR1             RGB16_WHITE\n");
  printf("#  define FGCOLOR2             RGB16_BLACK\n");
  printf("#  define FGCOLOR3             RGB16_GRAY\n");
  printf("#elif CONFIG_NXWIDGETS_BPP == 24 || CONFIG_NXWIDGETS_BPP == 32\n");
  printf("#  define FGCOLOR1             RGB24_WHITE\n");
  printf("#  define FGCOLOR2             RGB24_BLACK\n");
  printf("#  define FGCOLOR3             RGB24_GRAY\n");
  printf("#else\n");
  printf("#  error \"Pixel depth not supported (CONFIG_NXWIDGETS_BPP)\"\n");
  printf("#endif\n\n");

  printf("static const uint8_t g_cursorImage[] =\n");
  printf("{\n");

  /* Convert 24-bit RGB to 2 bit encoded cursor colors */

  ptr = gimp_image.pixel_data;

  for (i = 0; i < gimp_image.height; i++)
    {
      putchar(' ');
      for (j = 0; j < gimp_image.width; )
        {
          r = *ptr++;
          g = *ptr++;
          b = *ptr++;
          j++;

          code = (uint32_t)rgb2gray2(r, g, b) << 6;
          if (j < gimp_image.width)
            {
              r = *ptr++;
              g = *ptr++;
              b = *ptr++;
              j++;

              code |= (uint32_t)rgb2gray2(r, g, b) << 4;
            }

          if (j < gimp_image.width)
            {
              r = *ptr++;
              g = *ptr++;
              b = *ptr++;
              j++;

              code |= (uint32_t)rgb2gray2(r, g, b) << 2;
            }

          if (j < gimp_image.width)
            {
              r = *ptr++;
              g = *ptr++;
              b = *ptr++;
              j++;

              code |= (uint32_t)rgb2gray2(r, g, b);
            }

          printf(" 0x%02x,", code);
        }

      printf("    /* Row %d */\n", i);
    }

  printf("};\n\n");
  printf("const struct nx_cursorimage_s g_cursor =\n");
  printf("{\n");
  printf("  .size =\n");
  printf("  {\n");
  printf("    .w = %u,\n", gimp_image.width);
  printf("    .h = %u\n", gimp_image.height);
  printf("  },\n");
  printf("  .color1 =\n");
  printf("  {\n");
  printf("    FGCOLOR1\n");
  printf("  },\n");
  printf("  .color2 =\n");
  printf("  {\n");
  printf("    FGCOLOR1\n");
  printf("  },\n");
  printf("  .color3 =\n");
  printf("  {\n");
  printf("    FGCOLOR3\n");
  printf("  },\n");
  printf("  .image  = g_cursorImage\n");
  printf("};\n\n");

  return 0;
}
