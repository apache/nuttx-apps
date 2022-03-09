/****************************************************************************
 * apps/graphics/nxglyphs/include/mkcursor.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
