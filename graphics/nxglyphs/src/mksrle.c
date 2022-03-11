/****************************************************************************
 * apps/graphics/nxglyphs/src/mksrle.c
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

static unsigned char getmono(unsigned char r, unsigned char g, unsigned char b)
{
  float mono;
  unsigned int imono;

  //mono = 0.2125 * (float)r +(0.7154 * (float)g + 0.0721 * (float)b;
  mono = 0.299 * (float)r + 0.587 * (float)g + 0.114 * (float)b;

  imono = (unsigned int)mono;
  if (mono > 255)
    {
      mono = 255;
    }

  return mono;
}

static int findpixel(unsigned char r, unsigned char g, unsigned char b)
{
  int i;

  /* Throw away some accuracy */

  r &= 0xfc;
  g &= 0xfc;
  b &= 0xfc;

  for (i = 0; i < npixels; i++)
    {
      if (pixels[i].r == r && pixels[i].g == g && pixels[i].b == b)
        {
          return i;
        }
    }

  fprintf(stderr, "Pixel not found\n");
  exit(1);
}

static void addpixel(unsigned char r, unsigned char g, unsigned char b)
{
  int i;

  /* Throw away some accuracy */

  r &= 0xfc;
  g &= 0xfc;
  b &= 0xfc;

  for (i = 0; i < npixels; i++)
    {
      if (pixels[i].r == r && pixels[i].g == g && pixels[i].b == b)
        {
          return;
        }
    }

  pixels[npixels].r = r;
  pixels[npixels].g = g;
  pixels[npixels].b = b;
  npixels++;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  const unsigned char *ptr;
  unsigned char r;
  unsigned char g;
  unsigned char b;
  int ncode;
  int code;
  int last;
  int nruns;
  int first;
  int len;
  int i;
  int j;

  printf("/********************************************************************************************\n");
  printf(" * apps/graphics/nxglyphs/src/glyph_xxxxxx.cxx\n");
  printf(" *\n");
  printf(" * Licensed to the Apache Software Foundation (ASF) under one or more");
  printf(" * contributor license agreements.  See the NOTICE file distributed with");
  printf(" * this work for additional information regarding copyright ownership.  The");
  printf(" * ASF licenses this file to you under the Apache License, Version 2.0 (the");
  printf(" * "License"); you may not use this file except in compliance with the");
  printf(" * License.  You may obtain a copy of the License at");
  printf(" *");
  printf(" *   http://www.apache.org/licenses/LICENSE-2.0");
  printf(" *");
  printf(" * Unless required by applicable law or agreed to in writing, software");
  printf(" * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT");
  printf(" * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the");
  printf(" * License for the specific language governing permissions and limitations");
  printf(" * under the License.");
  printf(" *\n");
  printf(" ********************************************************************************************/\n\n");

  printf("/********************************************************************************************\n");
  printf(" * Included Files\n");
  printf(" ********************************************************************************************/\n\n");
  printf("#include <nuttx/config.h>\n");
  printf("#include <sys/types.h>\n");
  printf("#include <stdint.h>\n\n");
  printf("#include \"graphics/nxwidgets/crlepalettebitmap.hxx\"\n");
  printf("#include \"graphics/nxglyphs.hxx\"\n\n");

  /* Count the number of colors (npixels) */

  ptr = gimp_image.pixel_data;

  for (i = 0; i < gimp_image.height; i++)
    {
      for (j = 0; j < gimp_image.width; j++)
        {
          r = *ptr++;
          g = *ptr++;
          b = *ptr++;

          addpixel(r, g, b);
        }
    }

  printf("/********************************************************************************************\n");
  printf(" * Pre-Processor Definitions\n");
  printf(" ********************************************************************************************/\n\n");
  printf("#define BITMAP_NROWS     %u\n", gimp_image.height);
  printf("#define BITMAP_NCOLUMNS  %u\n", gimp_image.width);
  printf("#define BITMAP_NLUTCODES %u\n\n", npixels);

  printf("/********************************************************************************************\n");
  printf(" * Private Bitmap Data\n");
  printf(" ********************************************************************************************/\n\n");
  printf("using namespace NXWidgets;\n\n");

  printf("#if CONFIG_NXWIDGETS_BPP == 24 ||  CONFIG_NXWIDGETS_BPP == 32\n");
  printf("// RGB24 (8-8-8) Colors\n\n");
  printf("static const uint32_t g_xxxNormalLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 8)
    {
      printf(" ");
      for (j = 0; j < 8 && i + j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r; /* 8->8 bits */
          unsigned int g = pixels[i+j].g; /* 8->8 bits */
          unsigned int b = pixels[i+j].b; /* 8->8 bits */
          unsigned int pix24 = r << 16 | g << 8 | b;
          printf(" 0x%06x,", pix24);
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");
  printf("static const uint32_t g_xxxBrightLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 8)
    {
      printf(" ");
      for (j = 0; j < 8 && i + j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r; /* 8->8 bits */
          unsigned int g = pixels[i+j].g; /* 8->8 bits */
          unsigned int b = pixels[i+j].b; /* 8->8 bits */

          r = (3*r + 0xff) >> 2;
          g = (3*g + 0xff) >> 2;
          b = (3*b + 0xff) >> 2;

          unsigned int pix24 = r << 16 | g << 8 | b;
          printf(" 0x%06x,", pix24);
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");
  printf("#elif CONFIG_NXWIDGETS_BPP == 16\n");
  printf("// RGB16 (565) Colors (four of the colors in this map are duplicates)\n\n");
  printf("static const uint16_t g_xxxNormalLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 10)
    {
      printf(" ");
      for (j = 0; j < 10 && i+j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r >> 3;   /* 8->5 bits */
          unsigned int g = pixels[i+j].g >> 2;   /* 8->6 bits */
          unsigned int b = pixels[i+j].b >> 3;   /* 8->5 bits */

          if (r > 0x1f)
            {
              r = 0x1f;
            }

          if (g > 0x3f)
            {
              g = 0x3f;
            }

          if (b > 0x1f)
            {
              b = 0x1f;
            }

          unsigned int pix16 = r << 11 | g << 5 | b;
          printf(" 0x%04x,", pix16);
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");
  printf("static const uint16_t g_xxxBrightLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 10)
    {
      printf(" ");
      for (j = 0; j < 10 && i+j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r;
          unsigned int g = pixels[i+j].g;
          unsigned int b = pixels[i+j].b;

          r = (3*r + 0xff) >> 2;
          g = (3*g + 0xff) >> 2;
          b = (3*b + 0xff) >> 2;

          r >>= 3;   /* 8->5 bits */
          g >>= 2;   /* 8->6 bits */
          b >>= 3;   /* 8->5 bits */

          if (r > 0x1f)
            {
              r = 0x1f;
            }

          if (g > 0x3f)
            {
              g = 0x3f;
            }

          if (b > 0x1f)
            {
              b = 0x1f;
            }

          unsigned int pix16 = r << 11 | g << 5 | b;
          printf(" 0x%04x,", pix16);
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");

  printf("#elif CONFIG_NXWIDGETS_BPP == 8\n");
  printf("// 8-bit color lookups.  NOTE:  This is really dumb!  The lookup index is 8-bits and it used\n");
  printf("// to lookup an 8-bit value.  There is no savings in that!  It would be better to just put\n");
  printf("// the 8-bit color/greyscale value in the run-length encoded image and save the cost of these\n");
  printf("// pointless lookups.  But these p;ointless lookups do make the logic compatible with the\n");
  printf("// 16- and 24-bit types.\n");
  printf("///\n\n");

  printf("#  ifdef CONFIG_NXWIDGETS_GREYSCALE\n");
  printf("// 8-bit Greyscale\n\n");
  printf("static const uint8_t g_xxxNormalLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 16)
    {
      printf(" ");
      for (j = 0; j < 16 && i+j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r; /* 8->8 bits */
          unsigned int g = pixels[i+j].g; /* 8->8 bits */
          unsigned int b = pixels[i+j].b; /* 8->8 bits */

          printf(" 0x%02x,", getmono(r, g, b));
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("}\n\n");
  printf("static const uint8_t g_xxxBrightLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 16)
    {
      printf(" ");
      for (j = 0; j < 16 && i+j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r; /* 8->8 bits */
          unsigned int g = pixels[i+j].g; /* 8->8 bits */
          unsigned int b = pixels[i+j].b; /* 8->8 bits */

          r = (3*r + 0xff) >> 2;
          g = (3*g + 0xff) >> 2;
          b = (3*b + 0xff) >> 2;

          printf(" 0x%02x,", getmono(r, g, b));
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");
  printf("#  else /* CONFIG_NXWIDGETS_GREYSCALE */\n");
  printf("// RGB8 (332) Colors\n\n");
  printf("static const nxgl_mxpixel_t g_xxxNormalLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 16)
    {
      printf(" ");
      for (j = 0; j < 16 && i+j < npixels; j++)
        {
          unsigned int r = (pixels[i+j].r + 2) >> 5; /* 8->3 bits */
          unsigned int g = (pixels[i+j].g + 2) >> 5; /* 8->3 bits */
          unsigned int b = (pixels[i+j].b + 4) >> 6; /* 8->2 bits */

          if (r > 0x07)
            {
              r = 0x07;
            }
          if (g > 0x07)
            {
              g = 0x07;
            }
          if (b > 0x03)
            {
              b = 0x03;
            }

          unsigned int pix8 = r << 5 | g << 2 | b;
          printf(" 0x%02x,", pix8);
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");
  printf("static const uint8_t g_xxxBrightLut[BITMAP_NLUTCODES] =\n");
  printf("{\n");

  for (i = 0; i < npixels; i += 16)
    {
      printf(" ");
      for (j = 0; j < 16 && i+j < npixels; j++)
        {
          unsigned int r = pixels[i+j].r;
          unsigned int g = pixels[i+j].g;
          unsigned int b = pixels[i+j].b;

          r = (3*r + 0xff) >> 2;
          g = (3*g + 0xff) >> 2;
          b = (3*b + 0xff) >> 2;

          r = (r + 2) >> 5; /* 8->3 bits */
          g = (g + 2) >> 5; /* 8->3 bits */
          b = (b + 4) >> 6; /* 8->2 bits */

          if (r > 0x07)
            {
              r = 0x07;
            }
          if (g > 0x07)
            {
              g = 0x07;
            }
          if (b > 0x03)
            {
              b = 0x03;
            }

          unsigned int pix8 = r << 5 | g << 2 | b;
          printf(" 0x%02x,", pix8);
        }

      printf("  /* Codes %d-%d */\n", i, i+j-1);
    }

  printf("};\n\n");
  printf("#  endif\n");
  printf("#else\n");
  printf("#  error Unsupported pixel format\n");
  printf("#endif\n\n");

  printf("static const struct SRlePaletteBitmapEntry g_xxxRleEntries[] =\n");
  printf("{\n ");

  ptr = gimp_image.pixel_data;

  for (i = 0; i < gimp_image.height; i++)
    {
      ncode = 0;
      last  = -1;
      nruns = 0;
      first = 1;
      len   = 0;

      for (j = 0; j < gimp_image.width; j++)
        {
          r = *ptr++;
          g = *ptr++;
          b = *ptr++;

          code = findpixel(r, g, b);
          if (code == last && ncode < 256)
            {
              ncode++;
            }
          else
            {
              if (ncode > 0)
                {
                  len += printf(" {%3d, %3d},", ncode, last);
                  if (++nruns > 7)
                    {
                      if (first)
                        {
                          while (len < 100)
                            {
                              putchar(' ');
                              len++;
                            }

                          printf("// Row %d\n ", i);
                          first = 0;
                          len = 0;
                        }
                      else
                        {
                          printf("\n ");
                          len = 0;
                        }

                      nruns = 0;
                    }
                }

              ncode = 1;
              last = code;
            }
        }

      if (ncode > 0)
        {
          len += printf(" {%3d, %3d},", ncode, last);
        }

      if (first)
        {
          while (len < 100)
           {
              putchar(' ');
              len++;
           }

          printf("// Row %d\n ", i);
        }
      else
        {
          printf("\n ");
        }
    }

  printf("};\n\n");
  printf("/********************************************************************************************\n");
  printf(" * Public Bitmap Structure Definitions\n");
  printf(" ********************************************************************************************/\n\n");
  printf("const struct SRlePaletteBitmap NXWidgets::g_xxxBitmap =\n");
  printf("{\n");
  printf("  CONFIG_NXWIDGETS_BPP,  // bpp    - Bits per pixel\n");
  printf("  CONFIG_NXWIDGETS_FMT,  // fmt    - Color format\n");
  printf("  BITMAP_NLUTCODES,      // nlut   - Number of colors in the lLook-Up Table (LUT)\n");
  printf("  BITMAP_NCOLUMNS,       // width  - Width in pixels\n");
  printf("  BITMAP_NROWS,          // height - Height in rows\n");
  printf("  {                      // lut    - Pointer to the beginning of the Look-Up Table (LUT)\n");
  printf("    g_xxxNormalLut,      //          Index 0: Unselected LUT\n");
  printf("    g_xxxBrightLut,      //          Index 1: Selected LUT\n");
  printf("  },\n");
  printf("  g_xxxRleEntries        // data   - Pointer to the beginning of the RLE data\n");
  printf("};\n\n");

  return 0;
}
