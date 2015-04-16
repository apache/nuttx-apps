/****************************************************************************
 * examples/ltdc/dma2d.c
 *
 *   Copyright (C) 2008, 2011-2012, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Marco Krahl <ocram.lhark@gmail.com>
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

#include "ltdc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
****************************************************************************/

/****************************************************************************
 * Private Function
****************************************************************************/

/****************************************************************************
 * Name: ltdc_remove_dma2d_surface
 *
 * Description:
 *   Remove the surface of the dma2dlayer
 *
 ***************************************************************************/

static void ltdc_remove_dma2d_surface(FAR struct dma2d_surface *sur)
{
  if (sur)
    {
      up_dma2dremovelayer(sur->dma2d);
      free(sur);
    }
}


/****************************************************************************
 * Name: ltdc_create_dma2d_surface
 *
 * Description:
 *   Create a surface for the dma2dlayer
 *
 ***************************************************************************/

static FAR struct dma2d_surface *ltdc_create_dma2d_surface(uint16_t xres,
                                                           uint16_t yres,
                                                           uint8_t fmt)
{
  int ret;
  FAR struct dma2d_surface *sur = (FAR struct dma2d_surface *)
                                     malloc(sizeof(struct dma2d_surface));

  if (sur)
    {
      sur->dma2d = up_dma2dcreatelayer(xres, yres, fmt);

      if (!sur->dma2d)
        {
          dbg("up_dma2dcreatelayer failed\n");
          free(sur);
          sur = NULL;
        }
      else
        {
          ret = sur->dma2d->getvideoinfo(sur->dma2d, &sur->vinfo);

          if (ret != OK)
            {
              dbg("getvideoinfo() failed\n");
            }
          else
            {
              ret = sur->dma2d->getplaneinfo(sur->dma2d, 0, &sur->pinfo);

              if (ret != OK)
                {
                  dbg("getplaneinfo() failed\n");
                }
            }

          if(ret != OK)
            {
              ltdc_remove_dma2d_surface(sur);
              sur = NULL;
            }
          else
            {
              int lid;
              sur->dma2d->getlid(sur->dma2d, &lid);
              dbg("dma2d layer %d is created with: "
                  "layer = %p, xres = %d, yres = %d, fb start address = %p, "
                  "fb size = %d, fmt = %d, bpp = %d\n",
                  lid, sur->dma2d, sur->vinfo.xres, sur->vinfo.yres,
                  sur->pinfo.fbmem, sur->pinfo.fblen, sur->vinfo.fmt,
                  sur->pinfo.bpp);
            }
        }
    }

  return sur;
}


/****************************************************************************
 * Name: ltdc_clearlayer
 *
 * Description:
 *   Clear the whole ltdc layer with a specific color
 *
 ***************************************************************************/

static void ltdc_clearlayer(FAR struct surface *sur, uint8_t color)
{
  uint32_t   argb;
  struct ltdc_area_s area;

  argb = ltdc_color(&sur->vinfo, color);
  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  sur->layer->fillarea(sur->layer, &area, argb);
}

/****************************************************************************
 * Name: dma2d_clearlayer
 *
 * Description:
 *   Clear the whole dma2d layer with a specific color
 *
 ***************************************************************************/

static void dma2d_clearlayer(FAR struct dma2d_surface *sur, uint8_t color)
{
  uint32_t   argb;
  struct ltdc_area_s area;

  argb = ltdc_color(&sur->vinfo, color);
  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  sur->dma2d->fillarea(sur->dma2d, &area, argb);
}

/****************************************************************************
 * Name: ltdc_blendshadow
 *
 * Description:
 *  Helper: Blend a rectangle with an existing layer.
 *          This also blends a subjacent mirror image of the rectangle position
 *          dependent.
 *
 * Note! layer back and fore must have the same size.
 *       layer dest must be larger or equal to the size of the layer back and
 *       fore.
 *
 ***************************************************************************/

static void ltdc_blendshadow(FAR struct surface *dest,
                             FAR struct dma2d_surface *fore,
                             FAR struct dma2d_surface *back,
                             fb_coord_t xpos, fb_coord_t ypos,
                             fb_coord_t shadowlen,
                             uint8_t alpha)
{
  FAR struct ltdc_area_s area;
  int32_t    shadowx;
  int32_t    shadowy;

  area.xpos = xpos;
  area.ypos = ypos;
  area.xres = back->vinfo.xres;
  area.yres = back->vinfo.yres;

  /* Calculate the position of the mirror image */

  shadowx = ((xpos - (dest->vinfo.xres - back->vinfo.xres) / 2) * shadowlen * 2) /
                ((dest->vinfo.xres - back->vinfo.xres) / 2);
  if (xpos + shadowx < 0)
    {
      shadowx = -xpos;
    }
  else if (xpos + back->vinfo.xres + shadowx > dest->vinfo.xres)
    {
      shadowx = dest->vinfo.xres - (xpos +  back->vinfo.xres);
    }

  shadowy = ((ypos - (dest->vinfo.yres - back->vinfo.yres) / 2) * shadowlen * 2) /
                ((dest->vinfo.yres - back->vinfo.yres) / 2);

  if (ypos + shadowy < 0)
    {
      shadowy = -ypos;
    }
  else if (ypos + back->vinfo.yres + shadowy > dest->vinfo.yres)
    {
      shadowy = dest->vinfo.yres - (ypos +  back->vinfo.yres);
    }

  /* We do not really need the scratch layer, but we want to test blit and blend
   * operation with 3 dma2d layer
   */

  fore->dma2d->blit(fore->dma2d, 0, 0, dest->dma2d, &area);

  area.xpos = 0;
  area.ypos = 0;

  /* Blend the shadow surface semitransparency */

  back->dma2d->setalpha(back->dma2d, 86 * alpha / 255);
  fore->dma2d->setalpha(fore->dma2d, 169 * alpha / 255);
  dest->layer->blend(dest->layer, xpos + shadowx, ypos + shadowy, fore->dma2d,
                      0, 0, back->dma2d, &area);

  /* Blit to the origin surface */

  back->dma2d->setalpha(back->dma2d, alpha);
  fore->dma2d->setalpha(fore->dma2d, 255 - alpha);
  dest->layer->blend(dest->layer, xpos, ypos, fore->dma2d,
                      0, 0, back->dma2d, &area);
}

/****************************************************************************
 * Name: ltdc_blendrect
 *
 * Description:
 *   Helper: Blend a rectangle to the a specific pixel position.
 *   Note! This is only useful for the blitflipositioning test.
 *
 ***************************************************************************/

static void ltdc_blendrect(FAR struct dma2d_surface *fore,
                           FAR struct dma2d_surface *back,
                           fb_coord_t xpos, fb_coord_t ypos,
                           fb_coord_t shadowlen)
{
  FAR struct surface *sur = ltdc_get_surface(LTDC_LAYER_INACTIVE);

  /* Clear the invisible flip layer */

  ltdc_clearlayer(sur, LTDC_BLACK);

  /* Blend the rectangle */

  ltdc_blendshadow(sur, fore, back, xpos, ypos, 16, 255);

  /* Flip the layer to make the changes visible */

  sur->layer->update(sur->layer, LTDC_UPDATE_FLIP|
                                  LTDC_SYNC_VBLANK|
                                    LTDC_SYNC_WAIT);

  usleep(10);
}

/****************************************************************************
 * Name: ltdc_blendoutline
 *
 * Description:
 *   Draw the outline of a rectangle.
 *   Note! This is done by performing sequential blend operations.
 *         It does not claim to have a good speed performance.
 *
 ***************************************************************************/

static void ltdc_blendoutline(FAR struct dma2d_surface *fore,
                              FAR struct dma2d_surface *back)
{
  int   n;
  struct ltdc_area_s area;

  /* Use the inactive layer as second scratch layer */

  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

  /* Draw the outline */

  for (n = 0; n < 5 ; n++)
    {
      area.xpos = n;
      area.ypos = n;
      area.yres = fore->vinfo.yres - n * 2;
      area.xres = fore->vinfo.xres - n * 2;

      if (n == 0 || n == 2)
        {
          dma2d_clearlayer(fore, LTDC_BLACK);
          back->dma2d->setalpha(back->dma2d, 1);
          back->dma2d->setalpha(fore->dma2d, 2);
        }
      else if (n == 1)
        {
          dma2d_clearlayer(fore, LTDC_WHITE);
          back->dma2d->setalpha(back->dma2d, 2);
          back->dma2d->setalpha(fore->dma2d, 1);
        }
      else if (n == 3)
        {
          dma2d_clearlayer(fore, LTDC_BLACK);
          back->dma2d->setalpha(back->dma2d, 3);
          back->dma2d->setalpha(fore->dma2d, 1);
        }
      else
        {
          dma2d_clearlayer(fore, LTDC_BLACK);
          back->dma2d->setalpha(back->dma2d, 3);
          back->dma2d->setalpha(fore->dma2d, 0);
        }

      inactive->dma2d->blend(inactive->dma2d, n, n, back->dma2d,
                              n, n, fore->dma2d, &area);
    }

  /* Copy the result back to the background layer */

  area.xpos = 0;
  area.ypos = 0;
  area.yres = back->vinfo.yres;
  area.xres = back->vinfo.xres;

  back->dma2d->blit(back->dma2d, 0, 0, inactive->dma2d, &area);
}

/****************************************************************************
 * Name: ltdc_calcpixel
 *
 * Description:
 *   Calculates the next pixel position.
 *   This is based on the Breseham algorithmus.
 *
 ***************************************************************************/

static void ltdc_calcpos(int32_t *x0, int32_t *y0, int32_t x1, int32_t y1)
{
  if (*x0 != x1 && *y0 != y1)
    {
      int32_t sx = *x0<x1 ? 1 : -1;
      int32_t sy = *y0<y1 ? 1 : -1;
      int32_t dx = x1 - *x0 < 0 ? *x0 - x1 : x1 - *x0;
      int32_t dy = y1 - *y0 < 0 ? y1 - *y0 : *y0 - y1;
      int32_t e = 2 * (dx + dy);

      if (e > dy)
        {
          *x0 += sx;
        }
      if (e < dx)
        {
          *y0 += sy;
        }
    }
}

/****************************************************************************
 * Name: ltdc_dma2d_interface
 *
 * Description:
 *   Test: Error handling of dma2d interface
 *
 ***************************************************************************/

static void ltdc_dma2d_interface(void)
{
  int ret;
  uint8_t alpha = 0;
  uint32_t blendmode = 0;
#ifdef CONFIG_STM32_LTDC_L2
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_INACTIVE);
#else
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
#endif
  FAR struct dma2d_layer_s *dma2d = active->dma2d;

  dbg("Perform simple dma2d interface test\n");

  /* test setalpha */

  ret = dma2d->setalpha(dma2d, 127);

  if (ret != OK)
    {
      dbg("setalpha() failed\n");
      _exit(1);
    }

  /* test getalpha */

  ret = dma2d->getalpha(dma2d, &alpha);

  if (ret != OK  || alpha != 127)
    {
      dbg("getalpha() failed\n");
      _exit(1);
    }

  /* test setblendmode */

  ret = dma2d->setblendmode(dma2d, DMA2D_BLEND_ALPHA);

  if (ret != OK)
    {
      dbg("setblendmode() failed\n");
      _exit(1);
    }

  /* test getblendmode */

  ret = dma2d->getblendmode(dma2d, &blendmode);

  if (ret != OK  || blendmode != DMA2D_BLEND_ALPHA)
    {
      dbg("getblendmode() failed\n");
      _exit(1);
    }

#ifdef CONFIG_STM32_DMA2D_L8
  /* test setclut */

  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      int i;
      FAR struct fb_cmap_s *cmapltdc = ltdc_createcmap(LTDC_EXAMPLE_NCOLORS);
      FAR struct fb_cmap_s *cmap = ltdc_createcmap(256);

      if (!cmap || !cmapltdc)
        {
          ltdc_deletecmap(cmap);
          ltdc_deletecmap(cmapltdc);
          _exit(1);
        }

      /* store the clut table of the ltdc layer */

      ret = active->layer->getclut(active->layer, cmapltdc);

      if (ret != OK)
        {
          dbg("ltdc getclut() failed\n");
          _exit(1);
        }

      for (i = 0; i < LTDC_EXAMPLE_NCOLORS; i++)
        {
#ifdef CONFIG_FB_TRANSPARENCY
          dbg("ltdc color %d, %02x:%02x:%02x:%02x\n", i,
                cmapltdc->transp[i],
                cmapltdc->red[i],
                cmapltdc->green[i],
                cmapltdc->blue[i]);
#else
          dbg("ltdc color %d, %02x:%02x:%02x\n", i,
                cmapltdc->red[i],
                cmapltdc->green[i],
                cmapltdc->blue[i]);
#endif
        }

      ret = dma2d->getclut(dma2d, cmap);

      if (ret != OK)
        {
          dbg("getclut() failed\n");
          _exit(1);
        }

      memset(cmap->red, 0, 256);
      memset(cmap->green, 0, 256);
      memset(cmap->blue, 0, 256);
#ifdef CONFIG_FB_TRANSPARENCY
      memset(cmap->transp, 0, 256);
#endif
      dbg("set color lookup table to black\n");

      ret = dma2d->setclut(dma2d, cmap);

      if (ret != OK)
        {
          dbg("setclut() failed\n");
          _exit(1);
        }

      ret = dma2d->getclut(dma2d, cmap);

      if (ret != OK)
        {
          dbg("getclut() failed\n");
          _exit(1);
        }

      /* Check if the clut is black */

#ifdef CONFIG_FB_TRANSPARENCY
      if(memcmp(cmap->red, cmap->blue, 256) ||
           memcmp(cmap->red, cmap->green, 256) ||
           memcmp(cmap->transp, cmap->blue, 256))
#else
      if(memcmp(cmap->red, cmap->blue, 256) ||
           memcmp(cmap->red, cmap->green, 256))
#endif
        {
          dbg("unexpected clut content\n");
          _exit(1);
        }

      /* Check if the ltdc clut is set by the dma2d interface */

      ret = active->layer->getclut(active->layer, cmap);

      if (ret != OK)
        {
          dbg("ltdc getclut() failed\n");
          _exit(1);
        }

      /* Check if the ltdc clut is black */

#ifdef CONFIG_FB_TRANSPARENCY
      if(memcmp(cmap->red, cmap->blue, 256) ||
           memcmp(cmap->red, cmap->green, 256) ||
           memcmp(cmap->transp, cmap->blue, 256))
#else
      if(memcmp(cmap->red, cmap->blue, 256) ||
           memcmp(cmap->red, cmap->green, 256))
#endif
        {
          dbg("unexpected clut content\n");
          _exit(1);
        }

      /* Check if the clut is set by the ltdc interface */

      /* Restore the clut table of the ltdc layer */

      ret = active->layer->setclut(active->layer, cmapltdc);

      if (ret != OK)
        {
          dbg("ltdc setclut() failed\n");
          _exit(1);
        }

      /* Compare with the related clut table of the dma2d layer */

      ret = dma2d->getclut(dma2d, cmap);

      if (ret != OK)
        {
          dbg("getclut() failed\n");
          _exit(1);
        }

#ifdef CONFIG_FB_TRANSPARENCY
      if(memcmp(cmap->transp, cmapltdc->transp, LTDC_EXAMPLE_NCOLORS) ||
           memcmp(cmap->red, cmapltdc->red, LTDC_EXAMPLE_NCOLORS) ||
           memcmp(cmap->green, cmapltdc->green, LTDC_EXAMPLE_NCOLORS) ||
           memcmp(cmap->blue, cmapltdc->blue, LTDC_EXAMPLE_NCOLORS))
#else
      if(memcmp(cmap->red, cmapltdc->red, LTDC_EXAMPLE_NCOLORS) ||
           memcmp(cmap->green, cmapltdc->green, LTDC_EXAMPLE_NCOLORS) ||
           memcmp(cmap->blue, cmapltdc->blue, LTDC_EXAMPLE_NCOLORS))
#endif
        {
          dbg("clut of ltdc layer and related dma2d layer are different\n");
          _exit(1);
        }
      else
        {
          dbg("ok, changing the clut by the ltdc layer also changed the clut of "
                "the dma2d layer as expected.\n");
        }

      /* Check expected setclut error */

      cmap->first = 256;

      ret = dma2d->setclut(dma2d, cmap);

      if (ret == OK)
        {
          dbg("setclut() failed, expected error if first color exceeds 256\n");
        }
      else
        {
          dbg("setclut() Ok, unsupported cmap detected\n");
        }

      /* Check expected getclut error */

      ret = dma2d->getclut(dma2d, cmap);

      if (ret == OK)
        {
          dbg("getclut() failed, expected error if first color exceeds 256\n");
        }
      else
        {
          dbg("getclut() Ok, unsupported cmap detected\n");
        }

      cmap->first = 0;

      /* Restore the clut table of the ltdc layer if something goes wrong */

      ret = active->layer->setclut(active->layer, cmapltdc);

      if (ret != OK)
        {
          dbg("ltdc setclut() failed\n");
          _exit(1);
        }

      ltdc_deletecmap(cmap);
      ltdc_deletecmap(cmapltdc);
    }
#endif
}

/****************************************************************************
 * Name: ltdc_dma2d_fillarea
 *
 * Description:
 *   Test: Drawing color to specific area.
 *
 ***************************************************************************/

static void ltdc_dma2d_fillarea(void)
{
  int        ret = !OK;
  FAR struct ltdc_area_s area;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif

  /* Wrong positioning detection */

  area.xpos = 1;
  area.ypos = 0;
  area.xres = active->vinfo.xres;
  area.yres = active->vinfo.yres;

  dbg("check if the ltdc driver recognized when positioning overflows the whole"
      " layer buffer\n");

  if (active->layer->fillarea(active->layer, &area, 0) != OK)
    {
      ret = OK;
    }

  area.xpos = 0;
  area.ypos = 1;

  if (active->layer->fillarea(active->layer, &area, 0) != OK)
    {
      ret = OK == OK ? OK : ret;
    }

  if (ret == OK)
    {
      dbg("ok, driver detects wrong positioning\n");
    }
  else
    {
      dbg("fail, wrong positioning can overflow layer buffer\n");
    }

  dbg("check if the dma2d driver recognized when positioning overflows the"
      " whole layer buffer\n");

  if (active->dma2d->fillarea(active->dma2d, &area, 0) != OK)
    {
      ret = OK == OK ? OK : ret;
    }

  area.xpos = 0;
  area.ypos = 1;

  if (active->dma2d->fillarea(active->dma2d, &area, 0) != OK)
    {
      ret = OK == OK ? OK : ret;
    }

  if (ret == OK)
    {
      dbg("ok, driver detects wrong positioning\n");
    }
  else
    {
      dbg("fail, wrong positioning can overflow layer buffer\n");
    }

  /* Flip with non blend */

  dbg("Perform fillarea test\n");

  dbg("Ensure that the active layer is opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  dbg("Disable blend mode for the active layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));

  dbg("Flip the top layer to the active visible layer\n");
  active->layer->update(active->layer, LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

  dbg("Draw a red rectangle in top left quarter of the screen\n");

  area.xpos = 0;
  area.ypos = 0;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  active->layer->fillarea(active->layer, &area,
                          ltdc_color(&active->vinfo, LTDC_RED));

  usleep(1000000);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));

  dbg("Draw a green rectangle in top right quarter of the screen\n");

  area.xpos = active->vinfo.xres/2;
  area.ypos = 0;

  active->layer->fillarea(active->layer, &area,
                          ltdc_color(&active->vinfo, LTDC_GREEN));

  usleep(1000000);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));

  dbg("Draw a white rectangle in bottom left quarter of the screen\n");

  area.xpos = 0;
  area.ypos = active->vinfo.yres/2;

  active->layer->fillarea(active->layer, &area,
                          ltdc_color(&active->vinfo, LTDC_WHITE));

  dbg("Draw a blue rectangle in bottom right quarter of the screen\n");

  usleep(1000000);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));

  area.xpos = active->vinfo.xres/2;;
  area.ypos = active->vinfo.yres/2;

  active->layer->fillarea(active->layer, &area,
                          ltdc_color(&active->vinfo, LTDC_BLUE));

  usleep(1000000);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));
}

#ifdef CONFIG_STM32_LTDC_L2
/****************************************************************************
 * Name: ltdc_dma2d_blitsimple
 *
 * Description:
 *   Test: Perform simple blit operation to check source area positioning
 *
 ***************************************************************************/

static void ltdc_dma2d_blitsimple(void)
{
  FAR struct ltdc_area_s area;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif

  /* Flip with non blend */

  dbg("Perform simple blit operation\n");

  dbg("active->pinfo.fbmem = %p\n", active->pinfo.fbmem);
  dbg("inactive->pinfo.fbmem = %p\n", inactive->pinfo.fbmem);

  dbg("Ensure that both ltdc layer are opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  dbg("Disable blend mode for ltdc both layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_NONE);

  /* Fullscreen blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  dbg("Flip the top layer to the active visible layer\n");
  inactive->layer->update(active->layer,
                            LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

  ltdc_simple_draw(&inactive->vinfo, &inactive->pinfo);

  dbg("Blit the whole bottom layer to the top layer\n");
  area.xpos = 0;
  area.ypos = 0;
  area.xres = inactive->vinfo.xres;
  area.yres = inactive->vinfo.yres;
  active->layer->blit(active->layer, 0, 0, inactive->dma2d, &area);

  usleep(1000000);

  /* Top left to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = 0;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blit the top left red rectangle from the bottom layer with the"
      " top layer\n");

  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, inactive->dma2d, &area);

  usleep(1000000);

  /* Top right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = inactive->vinfo.xres/2;
  area.ypos = 0;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blit the top right green rectangle from the bottom layer with the"
      " top layer\n");

  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, inactive->dma2d, &area);

  usleep(1000000);

  /* Bottom left to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = inactive->vinfo.yres/2;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blit the bottom left white rectangle from the bottom layer with the"
      " top layer\n");

  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, inactive->dma2d, &area);

  usleep(1000000);

  /* Bottom right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = inactive->vinfo.xres/2;
  area.ypos = inactive->vinfo.yres/2;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blit the bottom right blue rectangle from the bottom layer with the"
      " top layer\n");

  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, inactive->dma2d, &area);

  usleep(1000000);
}

/****************************************************************************
 * Name: ltdc_dma2d_blitpositioning
 *
 * Description:
 *   Test: Perform simple blit operation to check source and destination
 *         positioning
 *
 ***************************************************************************/

static void ltdc_dma2d_blitpositioning(void)
{
  uint32_t   x, y;
  FAR struct ltdc_area_s area;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif

  /* Set layer in the middle of the screen */

  dbg("active->pinfo.fbmem = %p\n", active->pinfo.fbmem);
  dbg("inactive->pinfo.fbmem = %p\n", inactive->pinfo.fbmem);

  dbg("Ensure that both ltdc layer opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  dbg("Disable blend mode for both ltdc layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_NONE);

  /* Fullscreen blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  dbg("Flip the top layer to the active visible layer\n");
  inactive->layer->update(active->layer,
                            LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

  /* Create colored background buffer */
  ltdc_simple_draw(&inactive->vinfo, &inactive->pinfo);


  dbg("Perform positioning test\n");

  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;
  area.xpos = inactive->vinfo.xres/4;
  area.ypos = inactive->vinfo.yres/4;

  x = active->vinfo.xres/4;
  y = active->vinfo.yres/4;

  /* Move right */

  for (; x < active->vinfo.xres/4 + active->vinfo.xres/8; x++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       area.xpos = x;

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);

       usleep(5);
    }

  /* Move down */

  for (; y < active->vinfo.yres/4 + active->vinfo.yres/8; y++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       area.ypos = y;

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);

       usleep(5);
     }

  /* Move left */

  for (; x >= active->vinfo.xres/4 - active->vinfo.xres/8; x--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       area.xpos = x;

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);

       usleep(5);
    }

  /* Move up */

  for (; y >= active->vinfo.yres/4; y--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       area.ypos = y;

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);

       usleep(5);
     }

  /* Move right to the start position */

  for (; x <= active->vinfo.xres/4; x++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       area.xpos = x;

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);

       usleep(5);
    }

  dbg("Perform move test\n");

  area.xpos = inactive->vinfo.xres/4;
  area.ypos = inactive->vinfo.yres/4;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  /* Move right */

  for (x = area.xpos; x < area.xpos + area.xres/8; x++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, x, area.ypos, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move down */

  for (y = area.ypos; y < area.ypos + area.yres/8; y++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move left */

  for (; x >= area.xpos - area.xres/8; x--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move up */

  for (; y >= area.ypos; y--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move right to the start position */

  for (; x <= area.xpos; x++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, x, y, inactive->dma2d, &area);
       usleep(5);
    }

  dbg("Perform reference positioning test\n");

  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;
  area.xpos = inactive->vinfo.xres/4;
  area.ypos = inactive->vinfo.yres/4;

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  /* Move right */

  for (; area.xpos >= active->vinfo.xres/4 - active->vinfo.xres/8; area.xpos--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, active->vinfo.xres/4,
                            active->vinfo.yres/4, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move down */

  for (; area.ypos >= active->vinfo.yres/4 - active->vinfo.yres/8; area.ypos--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, active->vinfo.xres/4,
                            active->vinfo.yres/4, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move left */

  for (; area.xpos < active->vinfo.xres/4 + active->vinfo.xres/8; area.xpos++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, active->vinfo.xres/4,
                            active->vinfo.yres/4, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move up */

  for (; area.ypos < active->vinfo.yres/4; area.ypos++)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, active->vinfo.xres/4,
                            active->vinfo.yres/4, inactive->dma2d, &area);
       usleep(5);
    }

  /* Move right to the start position */

  for (; area.xpos >= active->vinfo.xres/4; area.xpos--)
    {
       /* Cleanup current screen */
       ltdc_clearlayer(active, LTDC_BLACK);

       active->layer->blit(active->layer, active->vinfo.xres/4,
                            active->vinfo.yres/4, inactive->dma2d, &area);
       usleep(5);
    }
}

/****************************************************************************
 * Name: ltdc_dma2d_blendsimple
 *
 * Description:
 *   Test: Perform simple blend operation to check source area positioning
 *
 ***************************************************************************/

static void ltdc_dma2d_blendsimple(void)
{
  uint8_t    alpha;
  FAR struct ltdc_area_s area;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif

  dbg("Perform simple blend operation\n");

  dbg("active->pinfo.fbmem = %p\n", active->pinfo.fbmem);
  dbg("inactive->pinfo.fbmem = %p\n", inactive->pinfo.fbmem);

  dbg("Ensure that both ltdc layer are opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  dbg("Enable alpha blend mode for both dma2d layer\n");
  active->dma2d->setblendmode(active->dma2d, DMA2D_BLEND_ALPHA);
  inactive->dma2d->setblendmode(inactive->dma2d, DMA2D_BLEND_ALPHA);

  /* Fullscreen blend */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  dbg("Flip the top layer to the active visible layer\n");
  active->layer->update(active->layer,
                        LTDC_UPDATE_ACTIVATE|LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);

  ltdc_simple_draw(&inactive->vinfo, &inactive->pinfo);

  dbg("Blend the whole bottom layer to the top layer\n");

  area.xpos = 0;
  area.ypos = 0;
  area.xres = inactive->vinfo.xres;
  area.yres = inactive->vinfo.yres;

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      active->dma2d->setalpha(active->dma2d, 255 - alpha/4);
      inactive->dma2d->setalpha(inactive->dma2d, alpha);
      active->layer->blend(active->layer, area.xpos, area.ypos,
                            active->dma2d, area.xpos, area.ypos,
                            inactive->dma2d, &area);
      usleep(5);
    }

  /* Blend top left to the middle */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = 0;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blend the top left red rectangle from the bottom layer with the middle"
      " of the top layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      active->dma2d->setalpha(active->dma2d, 255 - alpha/4);
      inactive->dma2d->setalpha(inactive->dma2d, alpha);
      active->layer->blend(active->layer, area.xpos, area.ypos,
                            active->dma2d, area.xpos, area.ypos,
                            inactive->dma2d, &area);
      usleep(5);
    }

  /* Top right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = inactive->vinfo.xres/2;
  area.ypos = 0;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blend the top right green rectangle from the bottom layer with the"
      " middle of the top layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      active->dma2d->setalpha(active->dma2d, 255 - alpha/4);
      inactive->dma2d->setalpha(inactive->dma2d, alpha);
      active->layer->blend(active->layer, area.xpos, area.ypos,
                            active->dma2d, area.xpos, area.ypos,
                            inactive->dma2d, &area);
      usleep(5);
    }

  /* Bottom left to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = inactive->vinfo.yres/2;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blit the bottom left white rectangle from the bottom layer with the"
      " middle of the top layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      active->dma2d->setalpha(active->dma2d, 255 - alpha/4);
      inactive->dma2d->setalpha(inactive->dma2d, alpha);
      active->layer->blend(active->layer, area.xpos, area.ypos,
                            active->dma2d, area.xpos, area.ypos,
                            inactive->dma2d, &area);
      usleep(5);
    }

  /* Bottom right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = inactive->vinfo.xres/2;
  area.ypos = inactive->vinfo.yres/2;
  area.xres = inactive->vinfo.xres/2;
  area.yres = inactive->vinfo.yres/2;

  dbg("Blit the bottom right blue rectangle from the bottom layer with the"
      " middle of the top layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      active->dma2d->setalpha(active->dma2d, 255 - alpha/4);
      inactive->dma2d->setalpha(inactive->dma2d, alpha);
      active->layer->blend(active->layer, area.xpos, area.ypos,
                            active->dma2d, area.xpos, area.ypos,
                            inactive->dma2d, &area);
      usleep(5);
    }

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);
}
#endif /* CONFIG_STM32_LTDC_L2 */

/****************************************************************************
 * Name: ltdc_dma2d_blitdynamiclayer
 *
 * Description:
 *   Test: Perform simple blit operation with allocated dma2d layer using the
 *         dma2d interface.
 *
 ***************************************************************************/

static void ltdc_dma2d_blitdynamiclayer(void)
{
  int        ret = !OK;
  FAR struct ltdc_area_s area;
  FAR struct ltdc_area_s forearea;
  FAR struct dma2d_surface *fore;
  FAR struct dma2d_surface *back;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif
  /* Create two new dma2d layer */

  back = ltdc_create_dma2d_surface(active->vinfo.xres, active->vinfo.yres,
                                    active->vinfo.fmt);

  if (!back)
    {
      _exit(1);
    }

  dbg("create background dma2d surface: %p\n", back);

  fore = ltdc_create_dma2d_surface(active->vinfo.xres/2, active->vinfo.yres/2,
                                    active->vinfo.fmt);

  if (!fore)
    {
      ltdc_remove_dma2d_surface(back);
      _exit(1);
    }

  dbg("create foreground dma2d surface: %p\n", fore);

  /* Wrong positioning detection */

  forearea.xpos = 0;
  forearea.ypos = 0;
  forearea.xres = fore->vinfo.xres;
  forearea.yres = fore->vinfo.yres;

  dbg("check if the ltdc driver recognized when positioning overflows the whole"
      " layer buffer\n");

  if (active->layer->blit(active->layer,
        active->vinfo.xres - forearea.xres + 1,
            active->vinfo.yres - forearea.yres, fore->dma2d, &forearea) != OK)
    {
      ret = OK;
    }

  if (active->layer->blit(active->layer,
        active->vinfo.xres - forearea.xres,
          active->vinfo.yres - forearea.yres + 1, fore->dma2d, &forearea) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  forearea.xpos = 1;

  if (active->layer->blit(active->layer, 0, 0, fore->dma2d, &forearea) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  forearea.xpos = 0;
  forearea.ypos = 1;

  if (active->layer->blit(active->layer, 0, 0, fore->dma2d, &forearea) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (ret == OK)
    {
      dbg("ok, driver detects wrong positioning\n");
    }
  else
    {
      dbg("fail, wrong positioning can overflow layer buffer\n");
    }

  dbg("check if the dma2d driver recognized when positioning overflows the"
        " whole layer buffer\n");

  forearea.xpos = 0;
  forearea.ypos = 0;

  if (active->dma2d->blit(active->dma2d,
        active->vinfo.xres - forearea.xres + 1,
          active->vinfo.yres - forearea.yres, fore->dma2d, &forearea) != OK)
    {
      ret = OK;
    }

  if (active->dma2d->blit(active->dma2d,
        active->vinfo.xres - forearea.xres,
          active->vinfo.yres - forearea.yres + 1, fore->dma2d, &forearea) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  forearea.xpos = 1;

  if (active->dma2d->blit(active->dma2d, 0, 0, fore->dma2d, &forearea) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  forearea.xpos = 0;
  forearea.ypos = 1;

  if (active->dma2d->blit(active->dma2d, 0, 0, fore->dma2d, &forearea) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (ret == OK)
    {
      dbg("ok, driver detects wrong positioning\n");
    }
  else
    {
      dbg("fail, wrong positioning can overflow layer buffer\n");
    }

  /* Initialize the dma2d fullscreen background layer */

  ltdc_simple_draw(&back->vinfo, &back->pinfo);

  /* Initialize foreground area for blitting to the screen */

  forearea.xpos = 0;
  forearea.ypos = 0;
  forearea.xres = fore->vinfo.xres;
  forearea.yres = fore->vinfo.yres;

  /* Blit test */

  dbg("Perform simple dma2d blit operation\n");

  dbg("Ensure that the ltdc layer is opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  dbg("Disable blend mode for the ltdc layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  dbg("Flip the top layer to the active visible layer\n");
  active->layer->update(active->layer,
                            LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

  /* Top left to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = 0;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blit the top left red rectangle from the background layer to the"
      " foreground layer\n");
  fore->dma2d->blit(fore->dma2d, 0, 0, back->dma2d, &area);

  dbg("Blit the resulting dma2d layer to the middle of the screen\n");
  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, back->dma2d, &forearea);

  usleep(1000000);

  /* Top right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = active->vinfo.xres/2;
  area.ypos = 0;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blit the top right green rectangle from the background layer to the"
      " foreground layer\n");
  fore->dma2d->blit(fore->dma2d, 0, 0, back->dma2d, &area);

  dbg("Blit the resulting dma2d layer to the middle of the screen\n");
  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, fore->dma2d, &forearea);

  usleep(1000000);

  /* Bottom left to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = active->vinfo.yres/2;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blit the bottom left white rectangle from the background layer to the"
      " foreground layer\n");
  fore->dma2d->blit(fore->dma2d, 0, 0, back->dma2d, &area);

  dbg("Blit the resulting dma2d layer to the middle of the screen\n");
  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, fore->dma2d, &forearea);

  usleep(1000000);

  /* Bottom right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = active->vinfo.xres/2;
  area.ypos = active->vinfo.yres/2;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blit the bottom right blue rectangle from the background layer to the"
      " foreground layer\n");
  fore->dma2d->blit(fore->dma2d, 0, 0, back->dma2d, &area);

  dbg("Blit the resulting dma2d layer to the middle of the screen\n");
  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, fore->dma2d, &forearea);

  usleep(1000000);

  /* Middle to the middle blit */

  dbg("Set the active layer to fullscreen black\n");

  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = active->vinfo.xres/4;
  area.ypos = active->vinfo.yres/4;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blit the bottom half rectangle from the background layers to the middle"
      " of the foreground layer\n");
  fore->dma2d->blit(fore->dma2d, 0, 0, back->dma2d, &area);

  dbg("Blit the resulting dma2d layer to the middle of the screen\n");
  active->layer->blit(active->layer, active->vinfo.xres/4,
                        active->vinfo.yres/4, fore->dma2d, &forearea);

  usleep(1000000);

  ltdc_remove_dma2d_surface(fore);
  ltdc_remove_dma2d_surface(back);
}

/****************************************************************************
 * Name: ltdc_dma2d_blenddynamiclayer
 *
 * Description:
 *   Test: Perform simple blend operation with allocated dma2d layer using the
 *         dma2d interface.
 *
 ***************************************************************************/

static void ltdc_dma2d_blenddynamiclayer(void)
{
  int        ret = !OK;
  uint8_t    alpha;
  FAR struct ltdc_area_s area;
  FAR struct dma2d_surface *fore;
  FAR struct dma2d_surface *back;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif
  /* Create two new dma2d layer */

  back = ltdc_create_dma2d_surface(active->vinfo.xres, active->vinfo.yres,
                                    active->vinfo.fmt);

  if (!back)
    {
      _exit(1);
    }

  dbg("create background dma2d surface: %p\n", back);

  fore = ltdc_create_dma2d_surface(active->vinfo.xres/2, active->vinfo.yres/2,
                                    active->vinfo.fmt);

  if (!fore)
    {
      ltdc_remove_dma2d_surface(back);
      _exit(1);
    }

  dbg("create foreground dma2d surface: %p\n", fore);

  /* Wrong positioning detection */

  dbg("check if the ltdc driver recognized when positioning overflows the whole"
      " layer buffer\n");

  area.xpos = 0;
  area.ypos = 0;
  area.xres = back->vinfo.xres;
  area.xres = back->vinfo.yres;

  if (active->layer->blend(active->layer,
            active->vinfo.xres - area.xres + 1,
                active->vinfo.yres - area.yres, fore->dma2d,
                    0, 0, back->dma2d, &area) != OK)
    {
      ret = OK;
    }

  if (active->layer->blend(active->layer,
            active->vinfo.xres - area.xres + 1,
                active->vinfo.yres - area.yres, fore->dma2d,
                    0, 0, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (active->layer->blend(active->layer, 0, 0, fore->dma2d,
            fore->vinfo.xres - area.xres + 1,
                fore->vinfo.yres - area.yres, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (active->layer->blend(active->layer, 0, 0, fore->dma2d,
            fore->vinfo.xres - area.xres,
                fore->vinfo.yres - area.yres + 1, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  area.xpos = 1;
  if (active->layer->blend(active->layer, 0, 0, fore->dma2d,
                            0, 0, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  area.xpos = 0;
  area.ypos = 1;
  if (active->layer->blend(active->layer, 0, 0, fore->dma2d,
                            0, 0, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (ret == OK)
    {
      dbg("ok, driver detects wrong positioning\n");
    }
  else
    {
      dbg("fail, wrong positioning can overflow layer buffer\n");
    }

  dbg("check if the dma2d driver recognized when positioning overflows the"
      " whole layer buffer\n");

  area.xpos = 0;
  area.ypos = 0;

  if (active->dma2d->blend(active->dma2d,
            active->vinfo.xres - area.xres + 1,
                active->vinfo.yres - area.yres, fore->dma2d,
                    0, 0, back->dma2d, &area) != OK)
    {
      ret = OK;
    }

  if (active->dma2d->blend(active->dma2d,
            active->vinfo.xres - area.xres + 1,
                active->vinfo.yres - area.yres, fore->dma2d,
                    0, 0, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (active->dma2d->blend(active->dma2d, 0, 0, fore->dma2d,
            fore->vinfo.xres - area.xres + 1,
                fore->vinfo.yres - area.yres, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (active->dma2d->blend(active->dma2d, 0, 0, fore->dma2d,
            fore->vinfo.xres - area.xres,
                fore->vinfo.yres - area.yres + 1, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  area.xpos = 1;
  if (active->dma2d->blend(active->dma2d, 0, 0, fore->dma2d,
                            0, 0, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  area.xpos = 0;
  area.ypos = 1;
  if (active->dma2d->blend(active->dma2d, 0, 0, fore->dma2d,
                            0, 0, back->dma2d, &area) != OK)
    {
      ret = ret == OK ? OK : ret;
    }

  if (ret == OK)
    {
      dbg("ok, driver detects wrong positioning\n");
    }
  else
    {
      dbg("fail, wrong positioning can overflow layer buffer\n");
    }

  /* Initialize the dma2d fullscreen background layer */

  ltdc_simple_draw(&back->vinfo, &back->pinfo);

  /* Blit test */

  dbg("Perform simple dma2d blend operation\n");

  dbg("Ensure that the ltdc layer is opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  dbg("Disable blend mode for the ltdc layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);
  dbg("Enable alpha blend mode for both dma2d layer\n");
  fore->dma2d->setblendmode(fore->dma2d, DMA2D_BLEND_ALPHA);
  back->dma2d->setblendmode(back->dma2d, DMA2D_BLEND_ALPHA);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  dbg("Flip the top layer to the active visible layer\n");
  active->layer->update(active->layer,
                            LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

  /* Top left to the middle */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = 0;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blend the top left red rectangle from the background layer with the"
      " middle of the foreground layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      fore->dma2d->setalpha(fore->dma2d, 255 - alpha);
      back->dma2d->setalpha(back->dma2d, alpha);
      active->dma2d->blend(active->dma2d, active->vinfo.xres/4,
                            active->vinfo.yres/4, fore->dma2d, 0, 0,
                            back->dma2d, &area);
      usleep(5);
    }

  usleep(1000000);

  /* Top right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = active->vinfo.xres/2;
  area.ypos = 0;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blend the top right green rectangle from the background layer with the"
      " middle of the foreground layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      fore->dma2d->setalpha(fore->dma2d, 255 - alpha);
      back->dma2d->setalpha(back->dma2d, alpha);
      active->dma2d->blend(active->dma2d, active->vinfo.xres/4,
                            active->vinfo.yres/4, fore->dma2d, 0, 0,
                            back->dma2d, &area);
      usleep(5);
    }

  usleep(1000000);

  /* Bottom left to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = 0;
  area.ypos = active->vinfo.yres/2;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blend the bottom left white rectangle from the background layer with the"
      " middle of foreground layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      fore->dma2d->setalpha(fore->dma2d, 255 - alpha);
      back->dma2d->setalpha(back->dma2d, alpha);
      active->dma2d->blend(active->dma2d, active->vinfo.xres/4,
                            active->vinfo.yres/4, fore->dma2d, 0, 0,
                            back->dma2d, &area);
      usleep(5);
    }

  usleep(1000000);

  /* Bottom right to the middle blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = active->vinfo.xres/2;
  area.ypos = active->vinfo.yres/2;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blend the bottom right blue rectangle from the background layer with the"
      " middle of the foreground layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      fore->dma2d->setalpha(fore->dma2d, 255 - alpha);
      back->dma2d->setalpha(back->dma2d, alpha);
      active->dma2d->blend(active->dma2d, active->vinfo.xres/4,
                            active->vinfo.yres/4, fore->dma2d, 0, 0,
                            back->dma2d, &area);
      usleep(5);
    }

  usleep(1000000);

  /* Middle to the middle blit */

  dbg("Set the active layer to fullscreen black\n");

  ltdc_clearlayer(active, LTDC_BLACK);

  area.xpos = active->vinfo.xres/4;
  area.ypos = active->vinfo.yres/4;
  area.xres = active->vinfo.xres/2;
  area.yres = active->vinfo.yres/2;

  dbg("Blend the bottom half screen rectangle from the background layers middle"
      " with the middle of the foreground layer\n");

  for (alpha = 0; alpha < 255 ; alpha++)
    {
      fore->dma2d->setalpha(fore->dma2d, 255 - alpha);
      back->dma2d->setalpha(back->dma2d, alpha);
      active->dma2d->blend(active->dma2d, active->vinfo.xres/4,
                            active->vinfo.yres/4, fore->dma2d, 0, 0,
                            back->dma2d, &area);
      usleep(5);
    }

  usleep(1000000);

  ltdc_remove_dma2d_surface(fore);
  ltdc_remove_dma2d_surface(back);
}

/****************************************************************************
 * Name: ltdc_dma2d_blitflippositioning
 *
 * Description:
 *   Perform simple blit and flip operation with both interfaces
 *
 ***************************************************************************/

static void ltdc_dma2d_blitflippositioning(void)
{
  uint32_t   x, y;
  FAR struct ltdc_area_s area;
  FAR struct dma2d_surface *fore;
  FAR struct dma2d_surface *back;
  FAR struct dma2d_surface *image;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8 || inactive->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif

  /* Create three new dma2d layer */

  fore = ltdc_create_dma2d_surface(active->vinfo.xres/2, active->vinfo.yres/2,
                                    active->vinfo.fmt);

  if (!fore)
    {
      _exit(1);
    }

  dbg("create foreground dma2d surface: %p\n", fore);

  back = ltdc_create_dma2d_surface(active->vinfo.xres/2, active->vinfo.yres/2,
                                    active->vinfo.fmt);

  if (!back)
    {
      ltdc_remove_dma2d_surface(fore);
      _exit(1);
    }

  dbg("create background dma2d surface: %p\n", back);

  image = ltdc_create_dma2d_surface(active->vinfo.xres, active->vinfo.yres,
                                      active->vinfo.fmt);

  if (!image)
    {
      ltdc_remove_dma2d_surface(fore);
      ltdc_remove_dma2d_surface(back);
      _exit(1);
    }

  dbg("create the dma2d surface to store the image: %p\n", image);

  dbg("Enable alpha blending for both dma2d layer\n");
  fore->dma2d->setblendmode(fore->dma2d, DMA2D_BLEND_ALPHA);
  back->dma2d->setblendmode(back->dma2d, DMA2D_BLEND_ALPHA);


  dbg("Ensure that both ltdc layer opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  dbg("Disable blend mode for both ltdc layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_NONE);

  /* Fullscreen blit */

  dbg("Set the active layer to fullscreen black\n");
  ltdc_clearlayer(active, LTDC_BLACK);

  dbg("Flip the top layer to the active visible layer\n");
  inactive->layer->update(active->layer,
                            LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

  /* Draw the four colored rectangles for blend operations */

  ltdc_simple_draw(&image->vinfo, &image->pinfo);

  dbg("Perform positioning test\n");

  area.xpos = inactive->vinfo.xres/4;
  area.ypos = inactive->vinfo.yres/4;
  area.yres = back->vinfo.yres;
  area.xres = back->vinfo.xres;

  /* Move right */

  y = active->vinfo.yres/4;

  for (x = active->vinfo.xres/4; x < active->vinfo.xres/4 + active->vinfo.xres/8; x++)
    {
      area.xpos = x;

      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move down */

  for (; y < active->vinfo.yres/4 + active->vinfo.yres/8; y++)
    {
      area.ypos = y;

      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move left */

  for (; x >= active->vinfo.xres/4 - active->vinfo.xres/8; x--)
    {
      area.xpos = x;

      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move up */

  for (; y >= active->vinfo.yres/4; y--)
    {
      area.ypos = y;

      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move right to the start position */

  for (; x <= active->vinfo.xres/4; x++)
    {
      area.xpos = x;

      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }


  ltdc_simple_draw(&back->vinfo, &back->pinfo);

  ltdc_blendoutline(fore, back);

  dbg("Perform move test\n");

  area.xpos = inactive->vinfo.xres/4;
  area.ypos = inactive->vinfo.yres/4;
  area.yres = back->vinfo.yres;
  area.xres = back->vinfo.xres;

  back->dma2d->blit(back->dma2d, 0, 0, inactive->dma2d, &area);

  /* Move right */

  y = area.ypos;

  for (x = area.xpos; x < area.xpos + active->vinfo.xres/8; x++)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move down */

  for (; y < area.ypos + active->vinfo.yres/8; y++)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move left */

  for (; x >= area.xpos - active->vinfo.xres/8; x--)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move up */

  for (; y >= area.ypos; y--)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }

  /* Move right to the start position */

  for (; x <= area.xpos; x++)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, x, y, 16);
    }


  dbg("Perform reference positioning test\n");

  area.xpos = inactive->vinfo.xres/4;
  area.ypos = inactive->vinfo.yres/4;
  area.yres = back->vinfo.yres;
  area.xres = back->vinfo.xres;


  /* Move right */

  for (; area.xpos >= active->vinfo.xres/4 - active->vinfo.xres/8; area.xpos--)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, active->vinfo.xres/4, active->vinfo.yres/4, 16);
    }

  /* Move down */

  for (; area.ypos >= active->vinfo.yres/4 - active->vinfo.yres/8; area.ypos--)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, active->vinfo.xres/4, active->vinfo.yres/4, 16);
    }

  /* Move left */

  for (; area.xpos < active->vinfo.xres/4 + active->vinfo.xres/8; area.xpos++)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, active->vinfo.xres/4, active->vinfo.yres/4, 16);
    }

  /* Move up */

  for (; area.ypos < active->vinfo.yres/4; area.ypos++)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, active->vinfo.xres/4, active->vinfo.yres/4, 16);
    }

  /* Move right to the start position */

  for (; area.xpos >= active->vinfo.xres/4; area.xpos--)
    {
      back->dma2d->blit(back->dma2d, 0, 0, image->dma2d, &area);

      ltdc_blendoutline(fore, back);

      ltdc_blendrect(fore, back, active->vinfo.xres/4, active->vinfo.yres/4, 16);
    }

  ltdc_remove_dma2d_surface(fore);
  ltdc_remove_dma2d_surface(back);
  ltdc_remove_dma2d_surface(image);
}

/****************************************************************************
 * Name: ltdc_screensaver
 *
 * Description:
 *  Perform the screensaver test.
 *  Note! This test runs in an endless loop.
 *
 ***************************************************************************/

static void ltdc_screensaver(void)
{
  int32_t   x1, y1, x2, y2;
  FAR struct ltdc_area_s area;
  FAR struct dma2d_surface *scratch;
  FAR struct dma2d_surface *rect1;
  FAR struct dma2d_surface *rect2;
  FAR struct dma2d_surface *image;
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

#ifdef CONFIG_STM32_DMA2D_L8
  if (active->vinfo.fmt == FB_FMT_RGB8 || inactive->vinfo.fmt == FB_FMT_RGB8)
    {
      dbg("skipped, output to layer with CLUT pixel format not supported\n");
      return;
    }
#endif

  /* Create three new dma2d layer */

  scratch = ltdc_create_dma2d_surface(active->vinfo.xres/4,
                                      active->vinfo.yres/4,
                                      active->vinfo.fmt);

  if (!scratch)
    {
      _exit(1);
    }

  dbg("create a scratch dma2d layer: %p\n", scratch);

  rect1 = ltdc_create_dma2d_surface(active->vinfo.xres/4,
                                    active->vinfo.yres/4,
                                    active->vinfo.fmt);

  if (!rect1)
    {
      ltdc_remove_dma2d_surface(scratch);
      _exit(1);
    }

  dbg("create a dma2d layer for the rectangle 1: %p\n", rect1);

  rect2 = ltdc_create_dma2d_surface(active->vinfo.xres/4,
                                    active->vinfo.yres/4,
                                    active->vinfo.fmt);

  if (!rect2)
    {
      ltdc_remove_dma2d_surface(scratch);
      ltdc_remove_dma2d_surface(rect1);
      _exit(1);
    }

  dbg("create a dma2d layer for rectangle 2: %p\n", rect2);

  image = ltdc_create_dma2d_surface(active->vinfo.xres,
                                    active->vinfo.yres,
                                    active->vinfo.fmt);

  if (!image)
    {
      ltdc_remove_dma2d_surface(scratch);
      ltdc_remove_dma2d_surface(rect1);
      ltdc_remove_dma2d_surface(rect2);
      _exit(1);
    }

  dbg("create a dma2d layer to store the background image: %p\n", image);

  dbg("Enable alpha blending for the dma2d layer\n");
  scratch->dma2d->setblendmode(scratch->dma2d, DMA2D_BLEND_ALPHA);
  rect1->dma2d->setblendmode(rect1->dma2d, DMA2D_BLEND_ALPHA);
  rect2->dma2d->setblendmode(rect2->dma2d, DMA2D_BLEND_ALPHA);

  /* ltdc layer settings */

  dbg("Ensure that both ltdc layer opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  dbg("Disable blend mode for both ltdc layer\n");
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_NONE);

  /* Draw the four colored rectangles for blend operations */

  ltdc_simple_draw(&image->vinfo, &image->pinfo);

  dbg("Perform screensaver\n");

  area.xpos = image->vinfo.xres/4 + image->vinfo.xres / 8;
  area.ypos = image->vinfo.yres/4 + image->vinfo.yres / 8;
  area.xres = rect1->vinfo.xres;
  area.yres = rect1->vinfo.yres;

  /* Create a rectangle with fix content */

  rect1->dma2d->blit(rect1->dma2d, 0, 0, image->dma2d, &area);

  ltdc_blendoutline(scratch, rect1);

  /* Randomize the start position */

  x1 = rand() % (inactive->vinfo.xres - rect1->vinfo.xres);
  y1 = rand() % (inactive->vinfo.yres - rect1->vinfo.yres);
  x2 = rand() % (inactive->vinfo.xres - rect2->vinfo.xres);
  y2 = rand() % (inactive->vinfo.yres - rect2->vinfo.yres);

  /* Change the blend area to the rect2 resolution */

  area.yres = rect2->vinfo.yres;
  area.xres = rect2->vinfo.xres;

  for(;;)
    {
      int32_t  xend1;
      int32_t  yend1;
      int32_t  xend2;
      int32_t  yend2;

      /* Randomize next end position */

      xend1 = rand() % (inactive->vinfo.xres - rect1->vinfo.xres);
      yend1 = rand() % (inactive->vinfo.yres - rect1->vinfo.yres);
      xend2 = rand() % (inactive->vinfo.xres - rect2->vinfo.xres);
      yend2 = rand() % (inactive->vinfo.yres - rect2->vinfo.yres);

      while (x1 != xend1 && y1 != yend1 && x2 != xend2 && y2 != yend2)
        {
          FAR struct surface *sur = ltdc_get_surface(LTDC_LAYER_INACTIVE);

          /* Calculate the next pixel start positions */

          ltdc_calcpos(&x1, &y1, xend1, yend1);
          ltdc_calcpos(&x2, &y2, xend2, yend2);

          area.xpos = x2;
          area.ypos = y2;

          /* Create the rectangle with the dynamic content */

          rect2->dma2d->blit(rect2->dma2d, 0, 0, image->dma2d, &area);

          ltdc_blendoutline(scratch, rect2);

          ltdc_clearlayer(sur, LTDC_BLACK);

          /* Blend rect2 as underlying rectangle */

          ltdc_blendshadow(sur, scratch, rect2, x2, y2, 10, 255);

          /* Blend rect1 as overlying rectangle ans semitransparency */

          ltdc_blendshadow(sur, scratch, rect1, x1, y1, 10, 127);

          /* Flip the layer to make the changes visible */

          sur->layer->update(sur->layer, LTDC_UPDATE_FLIP|
                                          LTDC_SYNC_VBLANK|
                                            LTDC_SYNC_WAIT);

          usleep(500);
        }
    }
}

/****************************************************************************
 * Name: ltdc_dma2d_blitmain
 *
 * Description:
 *   Triggers the dma2d tests
 *
 ***************************************************************************/

void ltdc_dma2d_main(void)
{

  ltdc_dma2d_interface();
  ltdc_dma2d_fillarea();
#ifdef CONFIG_STM32_LTDC_L2
  ltdc_dma2d_blitsimple();
  ltdc_dma2d_blitpositioning();
  ltdc_dma2d_blendsimple();
#endif
  ltdc_dma2d_blitdynamiclayer();
  ltdc_dma2d_blenddynamiclayer();
#ifdef CONFIG_STM32_LTDC_L2
  ltdc_dma2d_blitflippositioning();
  ltdc_screensaver();
#endif
}
