/****************************************************************************
 * examples/nxtext/nxtext_putc.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#include <nuttx/config.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/nx.h>
#include <nuttx/nxtk.h>
#include <nuttx/nxfonts.h>

#include "nxtext_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Select renderer -- Some additional logic would be required to support
 * pixel depths that are not directly addressable (1,2,4, and 24).
 */

#if CONFIG_EXAMPLES_NXTEXT_BPP == 1
#  define RENDERER nxf_convert_1bpp
#elif CONFIG_EXAMPLES_NXTEXT_BPP == 2
#  define RENDERER nxf_convert_2bpp
#elif CONFIG_EXAMPLES_NXTEXT_BPP == 4
#  define RENDERER nxf_convert_4bpp
#elif CONFIG_EXAMPLES_NXTEXT_BPP == 8
#  define RENDERER nxf_convert_8bpp
#elif CONFIG_EXAMPLES_NXTEXT_BPP == 16
#  define RENDERER nxf_convert_16bpp
#elif CONFIG_EXAMPLES_NXTEXT_BPP == 24
#  define RENDERER nxf_convert_24bpp
#elif  CONFIG_EXAMPLES_NXTEXT_BPP == 32
#  define RENDERER nxf_convert_32bpp
#else
#  error "Unsupported CONFIG_EXAMPLES_NXTEXT_BPP"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxtext_renderglyph
 ****************************************************************************/

static inline FAR const struct nxtext_glyph_s *
nxtext_renderglyph(FAR struct nxtext_state_s *st,
                   FAR const struct nx_fontbitmap_s *bm, uint8_t ch)
{
  FAR struct nxtext_glyph_s *glyph = NULL;
  FAR nxgl_mxpixel_t *ptr;
#if CONFIG_EXAMPLES_NXTEXT_BPP < 8
  nxgl_mxpixel_t pixel;
#endif
  int bmsize;
  int row;
  int col;
  int ret;

  /* Make sure that there is room for another glyph */

  message("nxtext_renderglyph: ch=%02x\n", ch);
  if (st->nglyphs < st->maxglyphs)
    {
      /* Allocate the glyph */

      glyph         = &st->glyph[st->nglyphs];
      glyph->code   = ch;

      /* Get the dimensions of the glyph */

      glyph->width  = bm->metric.width + bm->metric.xoffset;
      glyph->height = bm->metric.height + bm->metric.yoffset;

      /* Allocate memory to hold the glyph with its offsets */

      glyph->stride = (glyph->width * CONFIG_EXAMPLES_NXTEXT_BPP + 7) / 8;
      bmsize        =  glyph->stride * glyph->height;
      glyph->bitmap = (FAR uint8_t *)malloc(bmsize);

      if (glyph->bitmap)
        {
          /* Initialize the glyph memory to the background color */

#if CONFIG_EXAMPLES_NXTEXT_BPP < 8
          pixel  = st->wcolor[0];
#  if CONFIG_EXAMPLES_NXTEXT_BPP == 1
          /* Pack 1-bit pixels into a 2-bits */

          pixel &= 0x01;
          pixel  = (pixel) << 1 |pixel;
#  endif
#  if CONFIG_EXAMPLES_NXTEXT_BPP < 4
          /* Pack 2-bit pixels into a nibble */

          pixel &= 0x03;
          pixel  = (pixel) << 2 |pixel;
#  endif

          /* Pack 4-bit nibbles into a byte */

          pixel &= 0x0f;
          pixel  = (pixel) << 4 | pixel;

          ptr    = (FAR nxgl_mxpixel_t *)glyph->bitmap;
          for (row = 0; row < glyph->height; row++)
            {
              for (col = 0; col < glyph->stride; col++)
                {
                  /* Transfer the packed bytes into the buffer */

                  *ptr++ = pixel;
                }
            }

#elif CONFIG_EXAMPLES_NXTEXT_BPP == 24
# error "Additional logic is needed here for 24bpp support"

#else /* CONFIG_EXAMPLES_NXTEXT_BPP = {8,16,32} */

          ptr = (FAR nxgl_mxpixel_t *)glyph->bitmap;
          for (row = 0; row < glyph->height; row++)
            {
              /* Just copy the color value into the glyph memory */

              for (col = 0; col < glyph->width; col++)
                {
                  *ptr++ = st->wcolor[0];
                }
            }
#endif

          /* Then render the glyph into the allocated memory */

          ret = RENDERER((FAR nxgl_mxpixel_t*)glyph->bitmap,
                          glyph->height, glyph->width, glyph->stride,
                          bm, st->fcolor[0]);
          if (ret < 0)
            {
              /* Actually, the RENDERER never returns a failure */

              message("nxtext_renderglyph: RENDERER failed\n");
              free(glyph->bitmap);
              glyph->bitmap = NULL;
              glyph         = NULL;
            }
          else
            {
               /* Make it permanent */

               st->nglyphs++;
            }
        }
    }

  return glyph;
}

/****************************************************************************
 * Name: nxtext_addspace
 ****************************************************************************/

static inline FAR const struct nxtext_glyph_s *
nxtext_addspace(FAR struct nxtext_state_s *st, uint8_t ch)
{
  FAR struct nxtext_glyph_s *glyph = NULL;

  /* Make sure that there is room for another glyph */

  if (st->nglyphs < st->maxglyphs)
    {
      /* Allocate the NULL glyph */

      glyph        = &st->glyph[st->nglyphs];
      memset(glyph, 0, sizeof(struct nxtext_glyph_s));

      glyph->code  = ' ';
      glyph->width = st->spwidth;

      st->nglyphs++;
    }
  return glyph;
}

/****************************************************************************
 * Name: nxtext_findglyph
 ****************************************************************************/

static FAR const struct nxtext_glyph_s *
nxtext_findglyph(FAR struct nxtext_state_s *st, uint8_t ch)
{
  int i;

  /* First, try to find the glyph in the cache of pre-rendered glyphs */

   for (i = 0; i < st->nglyphs; i++)
    {
      if (st->glyph[i].code == ch)
        {
          return &st->glyph[i];
        }
    }
  return NULL;
}

/****************************************************************************
 * Name: nxtext_getglyph
 ****************************************************************************/

static FAR const struct nxtext_glyph_s *
nxtext_getglyph(FAR struct nxtext_state_s *st, uint8_t ch)
{
  FAR const struct nxtext_glyph_s *glyph;
  FAR const struct nx_fontbitmap_s *bm;

  /* First, try to find the glyph in the cache of pre-rendered glyphs */

  glyph = nxtext_findglyph(st, ch);
  if (!glyph)
    {
      /* No, it is not cached... Does the code map to a glyph? */

      bm = nxf_getbitmap(ch);
      if (!bm)
        {
          /* No, there is no glyph for this code.  Use space */

          glyph = nxtext_findglyph(st, ' ');
          if (!glyph)
            {
              /* There isn't fake glyph for ' ' yet... create one */

              glyph = nxtext_addspace(st, ' ');
            }
        }
      else
        {
          glyph = nxtext_renderglyph(st, bm, ch);
        }
    }

  return glyph;
}

/****************************************************************************
 * Name: nxtext_addchar
 ****************************************************************************/

static FAR const struct nxtext_bitmap_s *
nxtext_addchar(FAR struct nxtext_state_s *st, uint8_t ch)
{
  FAR struct nxtext_bitmap_s *bm = NULL;
  FAR struct nxtext_bitmap_s *bmleft;

  /* Is there space for another character on the display? */

  if (st->nchars < st->maxchars)
    {
       /* Yes, setup the bitmap */

       bm = &st->bm[st->nchars];

       /* Find the matching glyph */

       bm->glyph = nxtext_getglyph(st, ch);
       if (!bm->glyph)
         {
           return NULL;
         }

       /* Set up the bounds for the bitmap */

       if (st->nchars <= 0)
         {
            /* The first character is one space from the left */

            st->fpos.x  = st->spwidth;
         }
       else
         {
            /* Otherwise, it is to the left of the preceding char */

            bmleft = &st->bm[st->nchars-1];
            st->fpos.x  = bmleft->bounds.pt2.x + 1;
         }

       bm->bounds.pt1.x = st->fpos.x;
       bm->bounds.pt1.y = st->fpos.y;
       bm->bounds.pt2.x = st->fpos.x + bm->glyph->width - 1;
       bm->bounds.pt2.y = st->fpos.y + bm->glyph->height - 1;

       st->nchars++;
    }

  return bm;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxtext_home
 *
 * Description:
 *   Set the next character position to the top-left corner of the display.
 *
 ****************************************************************************/

void nxtext_home(FAR struct nxtext_state_s *st)
{
  /* The first character is one space from the left */

  st->fpos.x = st->spwidth;

  /* And two lines from the top */

  st->fpos.y = 2;
}

/****************************************************************************
 * Name: nxtext_newline
 *
 * Description:
 *   Set the next character position to the beginning of the next line.
 *
 ****************************************************************************/

void nxtext_newline(FAR struct nxtext_state_s *st)
{
  /* Carriage return: The first character is one space from the left */

  st->fpos.x = st->spwidth;

  /* Linefeed: Done the max font height + 2 */

  st->fpos.y += (st->fheight + 2);
}

/****************************************************************************
 * Name: nxtext_putc
 *
 * Description:
 *   Render the specified character at the current display position.
 *
 ****************************************************************************/

void nxtext_putc(NXWINDOW hwnd, FAR struct nxtext_state_s *st, uint8_t ch)
{
  FAR const struct nxtext_bitmap_s *bm;

  /* If it is a newline character, then just perform the logical newline
   * operation.
   */

  if (ch == '\n')
    {
      nxtext_newline(st);
    }

  /* Otherwise, find the glyph associated with the character and render it
   * onto the display.
   */

  else
    {
      bm = nxtext_addchar(st, ch);
      if (bm)
        {
          nxtext_fillchar(hwnd, &bm->bounds, bm);
        }
    }
}

/****************************************************************************
 * Name: nxtext_fillchar
 *
 * Description:
 *   This implements the character display.  It is part of the nxtext_putc
 *   operation but may also be used when redrawing an existing display.
 *
 ****************************************************************************/

void nxtext_fillchar(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                     FAR const struct nxtext_bitmap_s *bm)
{
  FAR const void *src = (FAR const void *)bm->glyph->bitmap;
  struct nxgl_rect_s intersection;
  int ret;

  /* Handle the special case of spaces which have no glyph bitmap */

  if (src)
    {
      /* Get the intersection of the redraw region and the character bitmap */

      nxgl_rectintersect(&intersection, rect, &bm->bounds);
      if (!nxgl_nullrect(&intersection))
        {
          ret = nx_bitmap((NXWINDOW)hwnd, &intersection, &src,
                          &bm->bounds.pt1,
                          (unsigned int)bm->glyph->stride);
          if (ret < 0)
            {
              message("nxtext_fillchar: nx_bitmapwindow failed: %d\n", errno);
            }
        }
    }
}

