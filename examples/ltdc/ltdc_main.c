/****************************************************************************
 * examples/ltdc/ltdc_main.c
 *
 *   Copyright (C) 2008, 2011-2012, 2015 Gregory Nutt. All rights reserved.
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

#include "ltdc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
****************************************************************************/

#ifdef CONFIG_STM32_LTDC_INTERFACE
#ifdef CONFIG_STM32_LTDC_L2
static struct surface g_surface[2];
#else
static struct surface g_surface[1];
#endif
#endif

#ifdef CONFIG_FB_CMAP
#ifdef CONFIG_FB_TRANSPARENCY
static uint8_t g_color[4*LTDC_EXAMPLE_NCOLORS];
#else
static uint8_t g_color[3*LTDC_EXAMPLE_NCOLORS];
#endif

static struct fb_cmap_s g_cmap =
{
  .first  = 0,
  .len    = LTDC_EXAMPLE_NCOLORS,
  .red    = &g_color[0],
  .green  = &g_color[LTDC_EXAMPLE_NCOLORS],
  .blue   = &g_color[2*LTDC_EXAMPLE_NCOLORS]
#ifdef CONFIG_FB_TRANSPARENCY
 ,.transp = &g_color[3*LTDC_EXAMPLE_NCOLORS]
#endif
};
#endif

/****************************************************************************
 * Private Function
****************************************************************************/

#ifdef CONFIG_STM32_LTDC_INTERFACE
/******************************************************************************
 * Name: ltdc_init_surface
 *
 * Description:
 *   Initialize layer and the layers videoinfo and planeinfo
 *
 *****************************************************************************/

static int ltdc_init_surface(int lid, uint32_t mode)
{
  FAR struct surface *sur = &g_surface[lid];

  sur->layer = up_ltdcgetlayer(lid);

  if (!sur->layer)
    {
      dbg("up_ltdcgetlayer() failed\n");
      return -1;
    }

  if (sur->layer->getvideoinfo(sur->layer, &sur->vinfo) != OK)
    {
      dbg("getvideoinfo() failed\n");
      return -1;
    }

  if (sur->layer->getplaneinfo(sur->layer, 0, &sur->pinfo) != OK)
    {
      dbg("getplaneinfo() failed\n");
      return -1;
    }

  dbg("layer %d is configured with: xres = %d, yres = %d,"
      "fb start address = %p, fb size = %d, fmt = %d, bpp = %d\n",
          lid, sur->vinfo.xres, sur->vinfo.yres, sur->pinfo.fbmem, sur->pinfo.fblen,
          sur->vinfo.fmt, sur->pinfo.bpp);

#ifdef CONFIG_FB_CMAP

  /* Initialize the clut table */

  if (sur->vinfo.fmt == FB_FMT_RGB8)
    {
      sur->layer->setclut(sur->layer, &g_cmap);
      sur->layer->update(sur->layer, LTDC_UPDATE_NONE);
    }
#endif

#ifdef CONFIG_STM32_DMA2D
  /* Initialize dma2d layer */

  if (sur->layer->getlid(sur->layer, &lid, LTDC_LAYER_DMA2D) != OK)
    {
      dbg("getlid() failed\n");
      return -1;
    }

  sur->dma2d = up_dma2dgetlayer(lid);

  if (sur->dma2d == NULL)
    {
      dbg("up_dma2dgetlayer() failed\n");
      return -1;
    }
#endif

  return OK;
}

/******************************************************************************
 * Name: ltdc_setget_test
 *
 * Description:
 *   Perform layer area positioning test
 *
 *****************************************************************************/

static void ltdc_setget_test(void)
{
  uint8_t alpha;
  uint16_t xpos;
  uint16_t ypos;
  uint32_t color;
  uint32_t mode;
  int ret;
  FAR struct ltdc_area_s area;
  FAR struct surface *sur = ltdc_get_surface(LTDC_LAYER_ACTIVE);

  dbg("Perform set and get test\n");

  /* setalpha */

  ret = sur->layer->setalpha(sur->layer, 0x7f);

  if (ret != OK)
    {
      dbg("setalpha() failed\n");
    }

  ret = sur->layer->getalpha(sur->layer, &alpha);

  if (ret != OK || alpha != 0x7f)
    {
      dbg("getalpha() failed\n");
    }

  /* setcolor */

  ret = sur->layer->setcolor(sur->layer, 0x11223344);

  if (ret != OK)
    {
      dbg("setcolor() failed\n");
    }

  ret = sur->layer->getcolor(sur->layer, &color);

  if (ret != OK || color != 0x11223344)
    {
      dbg("getcolor() failed\n");
    }

  /* setcolorkey */

  ret = sur->layer->setcolorkey(sur->layer, 0x55667788);

  if (ret != OK)
    {
      dbg("setcolorkey() failed\n");
    }

  ret = sur->layer->getcolorkey(sur->layer, &color);

  if (ret != OK || color != 0x55667788)
    {
      dbg("getcolorkey() failed\n");
    }

  /* setblendmode */

  ret = sur->layer->setblendmode(sur->layer, LTDC_BLEND_NONE);

  if (ret != OK)
    {
      dbg("setblendmode() failed\n");
    }

  ret = sur->layer->getblendmode(sur->layer, &mode);

  if (ret != OK || mode != LTDC_BLEND_NONE)
    {
      dbg("getblendmode() failed\n");
    }

  /* setarea */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  ret = sur->layer->setarea(sur->layer, &area,
            sur->vinfo.xres/8, sur->vinfo.yres/8);

  if (ret != OK)
    {
      dbg("setarea() failed\n");
    }

  ret = sur->layer->getarea(sur->layer, &area, &xpos, &ypos);

  if (ret != OK || xpos != sur->vinfo.xres/8 || ypos != sur->vinfo.yres/8 ||
      area.xpos != sur->vinfo.xres/4 || area.ypos != sur->vinfo.yres/4 ||
      area.xres != sur->vinfo.xres/2 || area.yres != sur->vinfo.yres/2)
    {
      dbg("getarea() failed\n");
    }

#ifdef CONFIG_FB_CMAP
  if (sur->vinfo.fmt == FB_FMT_RGB8)
    {

      FAR struct fb_cmap_s *cmap = ltdc_createcmap(LTDC_EXAMPLE_NCOLORS);

      ltdc_clrcolor(g_cmap.red, 0x11, LTDC_EXAMPLE_NCOLORS);
      ltdc_clrcolor(g_cmap.blue, 0x22, LTDC_EXAMPLE_NCOLORS);
      ltdc_clrcolor(g_cmap.green, 0x33, LTDC_EXAMPLE_NCOLORS);
#ifdef CONFIG_FB_TRANSPARENCY
      ltdc_clrcolor(g_cmap.transp, 0x44, LTDC_EXAMPLE_NCOLORS);
#endif

      ret = sur->layer->setclut(sur->layer, &g_cmap);

      if (ret != OK)
        {
          dbg("setclut() failed\n");
        }

      /* Clear all colors to black */

      ltdc_clrcolor(cmap->red, 0, LTDC_EXAMPLE_NCOLORS);
      ltdc_clrcolor(cmap->blue, 0, LTDC_EXAMPLE_NCOLORS);
      ltdc_clrcolor(cmap->green, 0, LTDC_EXAMPLE_NCOLORS);
#ifdef CONFIG_FB_TRANSPARENCY
      ltdc_clrcolor(cmap->transp, 0, LTDC_EXAMPLE_NCOLORS);
#endif

      ret = sur->layer->getclut(sur->layer, cmap);

      if (ret != OK)
        {
          dbg("getclut() failed\n");
        }

#ifdef CONFIG_FB_TRANSPARENCY
      if (ltdc_cmpcolor(g_cmap.red, cmap->red, LTDC_EXAMPLE_NCOLORS) ||
            ltdc_cmpcolor(g_cmap.blue, cmap->blue, LTDC_EXAMPLE_NCOLORS) ||
             ltdc_cmpcolor(g_cmap.green, cmap->green, LTDC_EXAMPLE_NCOLORS) ||
                ltdc_cmpcolor(g_cmap.transp, cmap->transp, LTDC_EXAMPLE_NCOLORS))
#else
      if (ltdc_cmpcolor(g_cmap.red, cmap->red, LTDC_EXAMPLE_NCOLORS) ||
            ltdc_cmpcolor(g_cmap.blue, cmap->blue, LTDC_EXAMPLE_NCOLORS) ||
             ltdc_cmpcolor(g_cmap.green, cmap->green, LTDC_EXAMPLE_NCOLORS))
#endif
        {
          dbg("getclut() failed, unexpected cmap content\n");
        }

      ltdc_deletecmap(cmap);

      /* Restore the origin cmap */

      ltdc_init_cmap();

      ret = sur->layer->setclut(sur->layer, &g_cmap);

      if (ret != OK)
        {
          dbg("setclut() failed\n");
        }
    }
#endif

  /* Restore to default state */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  sur->layer->setalpha(sur->layer, 0xff);
  sur->layer->setcolor(sur->layer, 0);
  sur->layer->setcolorkey(sur->layer, 0);
  sur->layer->setarea(sur->layer, &area, 0, 0);
  sur->layer->setblendmode(sur->layer, LTDC_BLEND_NONE);
  sur->layer->update(sur->layer, 0);
}

/******************************************************************************
 * Name: ltdc_color_test
 *
 * Description:
 *   Perform layer color test
 *
 *****************************************************************************/

static void ltdc_color_test(void)
{
  struct ltdc_area_s area;

  FAR struct surface *sur = ltdc_get_surface(LTDC_LAYER_ACTIVE);

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  ltdc_simple_draw(&sur->vinfo, &sur->pinfo);

  usleep(1000000);

  /* Default Color black */

  dbg("Set default color to black\n");

  sur->layer->setcolor(sur->layer, 0xff000000);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  /* Set active layer to the upper half of the screen */

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  dbg("Update the layer, should be black outside the colorful rectangle\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Default Color red */

  dbg("Update the layer, should be red outside the colorful rectangle\n");

  sur->layer->setcolor(sur->layer, 0xffff0000);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Default Color green */

  dbg("Update the layer, should be green outside the colorful rectangle\n");

  sur->layer->setcolor(sur->layer, 0xff00ff00);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Default Color blue */

  dbg("Update the layer, should be blue outside the colorful rectangle\n");

  sur->layer->setcolor(sur->layer, 0xff0000ff);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Restore original size */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  /* Default Color black */

  dbg("Set default color to black\n");

  sur->layer->setcolor(sur->layer, 0);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);
}

/******************************************************************************
 * Name: ltdc_colorkey_test
 *
 * Description:
 *   Perform layer colorkey test
 *
 *****************************************************************************/

static void ltdc_colorkey_test(void)
{
  struct ltdc_area_s area;

  FAR struct surface *sur = ltdc_get_surface(LTDC_LAYER_ACTIVE);

  ltdc_simple_draw(&sur->vinfo, &sur->pinfo);

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  /* Resize active layer */

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  /* Enable colorkey */

  sur->layer->setblendmode(sur->layer, LTDC_BLEND_COLORKEY);

  /* Color key white */

  dbg("Set colorkey to white\n");

  sur->layer->setcolorkey(sur->layer, 0xffffff);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Color key red */

  dbg("Set colorkey to red\n");

  sur->layer->setcolorkey(sur->layer, 0xff0000);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Color key green */

  dbg("Set colorkey to green\n");

  sur->layer->setcolorkey(sur->layer, 0xff00);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);

  /* Color key red */

  dbg("Set colorkey to blue\n");

  sur->layer->setcolorkey(sur->layer, 0xff);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  dbg("Disable colorkey\n");

  usleep(1000000);

  sur->layer->setcolorkey(sur->layer, 0);

  /* Restore original size */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, 0, 0);

  /* Disable colorkeying */

  sur->layer->setblendmode(sur->layer, LTDC_BLEND_NONE);

  dbg("Update the layer\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK|LTDC_SYNC_WAIT);

  usleep(1000000);
}

/******************************************************************************
 * Name: ltdc_area_test
 *
 * Description:
 *   Perform layer area positioning test
 *
 *****************************************************************************/

static void ltdc_area_test(void)
{
  int n;
  int x;
  int y;
  struct ltdc_area_s area;
  FAR struct surface *sur = ltdc_get_surface(LTDC_LAYER_ACTIVE);

  dbg("Perform area test\n");

  ltdc_simple_draw(&sur->vinfo, &sur->pinfo);

  usleep(1000000);

  /* Set active layer to the upper left rectangle of the screen */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  dbg("Update the layer, to show the upper left rectangle of the screen\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Set active layer to the upper rigth rectangle of the screen */

  area.xpos = sur->vinfo.xres/2;
  area.ypos = 0;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  dbg("Update the layer, to show the upper right rectangle of the screen\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Set active layer to the upper left rectangle of the screen */

  area.xpos = 0;
  area.ypos = sur->vinfo.yres/2;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  dbg("Update the layer, to show the lower left rectangle of the screen\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Set active layer to the upper right rectangle of the screen */

  area.xpos = sur->vinfo.xres/2;
  area.ypos = sur->vinfo.yres/2;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  dbg("Update the layer, to show the lower right rectangle of the screen\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Perform layer positioning */

  dbg("Perform positioning test\n");

  /* Set layer in the middle of the screen */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  /* Move right */

  for (n = 0; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move down */

  for (n = 0; n < sur->vinfo.yres/8; n++)
    {
      area.ypos++;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move left */

  for (n = 0; n < sur->vinfo.xres/4; n++)
    {
      area.xpos--;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move up */

  for (n = 0; n < sur->vinfo.yres/8; n++)
    {
      area.ypos--;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move back to the middle */

  for (n = 0; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Perform move */

  dbg("Perform move test\n");

  /* Set layer in the middle of the screen */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  /* Move right */

  for (n = 0; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4,
                            sur->vinfo.yres/4);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move down */

  for (n = 0; n < sur->vinfo.yres/8; n++)
    {
      area.ypos++;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4,
                            sur->vinfo.yres/4);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move left */

  for (n = 0; n < sur->vinfo.xres/4; n++)
    {
      area.xpos--;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4,
                            sur->vinfo.yres/4);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move up */

  for (n = 0; n < sur->vinfo.yres/8; n++)
    {
      area.ypos--;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4,
                            sur->vinfo.yres/4);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move back to the middle */

  for (n = 0; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4,
                            sur->vinfo.yres/4);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Perform Reference position */

  dbg("Perform reference positioning test\n");

  /* Set layer in the middle of the screen */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  /* Move right */

  for (x = 0; x < sur->vinfo.xres/8; x++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - x,
                            sur->vinfo.yres/4);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move down */

  for (y = 0; y < sur->vinfo.yres/8; y++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - x,
                            sur->vinfo.yres/4 - y);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move left */

  for (x = 0; x < sur->vinfo.xres/4; x++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - sur->vinfo.xres/8 + x,
                            sur->vinfo.yres/4 - y);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move up */

  for (y = 0; y < sur->vinfo.yres/8; y++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - sur->vinfo.xres/8 + x,
                            sur->vinfo.yres/4 - sur->vinfo.yres/8 + y);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  /* Move back to the middle */

  for (x = 0; x < sur->vinfo.xres/8; x++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 + sur->vinfo.xres/8 - x,
                            sur->vinfo.yres/4 - sur->vinfo.yres/8 + y);
      sur->layer->update(sur->layer, LTDC_SYNC_NONE);
      usleep(5);
    }

  usleep(1000000);

  /* Restore original size */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  dbg("Update the layer to fullscreen\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  usleep(1000000);
}

/******************************************************************************
 * Name: ltdc_common_test
 *
 * Description:
 *   Perform test with all layer operations at once
 *   Todo: add alpha blending and default color
 *
 *****************************************************************************/

static void ltdc_common_test(void)
{
  int n;
  int c;
  int x;
  int y;
  uint32_t colorkey;
  struct ltdc_area_s area;
  FAR struct surface *sur;

  dbg("Set layer 2 to the active layer, blend with subjacent layer 1\n");

  sur = ltdc_get_surface(LTDC_LAYER_TOP);

  ltdc_simple_draw(&sur->vinfo, &sur->pinfo);

  usleep(1000000);

  colorkey = LTDC_EXAMPLE_NCOLORS;

  /* Perform area test */

  dbg("Perform area test\n");

  /* Set layer in the middle of the screen */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  /* Enable colorkeying */

  sur->layer->setblendmode(sur->layer, LTDC_BLEND_COLORKEY);

  /* Update the layer */

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  /* Move right */

  for (n = 0, c = 4; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move down */

  for (n = 0, c = 4; n < sur->vinfo.yres/8; n++)
    {
      area.ypos++;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move left */

  for (n = 0, c = 4; n < sur->vinfo.xres/4; n++)
    {
      area.xpos--;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move up */

  for (n = 0, c = 4; n < sur->vinfo.yres/8; n++)
    {
      area.ypos--;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move back to the middle */

  for (n = 0, c = 4; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Perform positioning test */

  dbg("Perform positioning test\n");

  /* Set layer in the middle of the screen */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  /* Move right */

  for (n = 0, c = 4; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4, sur->vinfo.yres/4);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move down */

  for (n = 0, c = 4; n < sur->vinfo.yres/8; n++)
    {
      area.ypos++;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4, sur->vinfo.yres/4);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move left */

  for (n = 0, c = 4; n < sur->vinfo.xres/4; n++)
    {
      area.xpos--;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4, sur->vinfo.yres/4);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move up */

  for (n = 0, c = 4; n < sur->vinfo.yres/8; n++)
    {
      area.ypos--;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4, sur->vinfo.yres/4);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move back to the middle */

  for (n = 0, c = 4; n < sur->vinfo.xres/8; n++)
    {
      area.xpos++;
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4, sur->vinfo.yres/4);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Perform Reference position */

  dbg("Perform reference positioning test\n");

  /* Set layer in the middle of the screen */

  area.xpos = sur->vinfo.xres/4;
  area.ypos = sur->vinfo.yres/4;
  area.xres = sur->vinfo.xres/2;
  area.yres = sur->vinfo.yres/2;

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);
  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  /* Move right */

  for (x = 0, c = 4; x < sur->vinfo.xres/8; x++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - x,
                            sur->vinfo.yres/4);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move down */

  for (y = 0, c = 4; y < sur->vinfo.yres/8; y++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - x,
                            sur->vinfo.yres/4 - y);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move left */

  for (x = 0, c = 4; x < sur->vinfo.xres/4; x++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - sur->vinfo.xres/8 + x,
                            sur->vinfo.yres/4 - y);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move up */

  for (y = 0, c = 4; y < sur->vinfo.yres/8; y++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 - sur->vinfo.xres/8 + x,
                            sur->vinfo.yres/4 - sur->vinfo.yres/8 + y);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  /* Move back to the middle */

  for (x = 0, c = 4; x < sur->vinfo.xres/8; x++)
    {
      sur->layer->setarea(sur->layer, &area,
                            sur->vinfo.xres/4 + sur->vinfo.xres/8 - x,
                            sur->vinfo.yres/4 - sur->vinfo.yres/8 + y);
      if (c++ == 4)
        {
          c = 0;
          if (colorkey == LTDC_EXAMPLE_NCOLORS)
            colorkey = LTDC_RED;
          else
            colorkey++;

          sur->layer->setcolorkey(sur->layer, 0xff000000 | g_rgb24[colorkey]);
        }

      sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);
    }

  usleep(1000000);

  /* Restore original size */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = sur->vinfo.xres;
  area.yres = sur->vinfo.yres;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  sur->layer->setarea(sur->layer, &area, area.xpos, area.ypos);

  /* Disable colorkeying */

  sur->layer->setcolorkey(sur->layer, 0);
  sur->layer->setblendmode(sur->layer, LTDC_BLEND_NONE);

  dbg("Update the layer to fullscreen\n");

  sur->layer->update(sur->layer, LTDC_SYNC_VBLANK);

  usleep(1000000);
}

#ifdef CONFIG_STM32_LTDC_L2
/******************************************************************************
 * Name: ltdc_alpha_blend_test
 *
 * Description:
 *   Perform layer blend test
 *
 *****************************************************************************/

static void ltdc_alpha_blend_test(void)
{
  int i;
  FAR struct surface *top;
  FAR struct surface *bottom;
  struct ltdc_area_s area;

  /* Ensure operation on layer 2 */

  dbg("Set layer 2 to the active layer, blend with subjacent layer 1\n");

  top = ltdc_get_surface(LTDC_LAYER_TOP);
  bottom = ltdc_get_surface(LTDC_LAYER_BOTTOM);

  dbg("top = %p, bottom = %p\n", top->pinfo.fbmem, bottom->pinfo.fbmem);

  ltdc_simple_draw(&top->vinfo, &top->pinfo);

  dbg("Fill layer1 with color black\n");

  ltdc_drawcolor(&bottom->vinfo, bottom->pinfo.fbmem,
                  bottom->vinfo.xres, bottom->vinfo.yres,
                  ltdc_color(&bottom->vinfo, LTDC_BLACK));

  area.xpos = top->vinfo.xres/4;
  area.ypos = top->vinfo.yres/4;
  area.xres = top->vinfo.xres/2;
  area.yres = top->vinfo.yres/2;

  dbg("Set area to xpos = %d, ypos = %d, xres = %d, yres = %d\n",
        area.xpos, area.ypos, area.xres, area.yres);

  top->layer->setarea(top->layer, &area, area.xpos, area.ypos);

  dbg("Set alpha blending with bottom layer1\n");

  top->layer->setblendmode(top->layer, LTDC_BLEND_ALPHA);
  dbg("Disable blending for bottom layer1 to make the layer color visible\n");

  bottom->layer->setblendmode(bottom->layer, LTDC_BLEND_NONE);
  bottom->layer->setalpha(bottom->layer, 0xff);

  dbg("Fill bottom layer1 with color black\n");

  ltdc_drawcolor(&bottom->vinfo, bottom->pinfo.fbmem,
                  bottom->vinfo.xres, bottom->vinfo.yres,
                  ltdc_color(&bottom->vinfo, LTDC_BLACK));

  dbg("Blend in black subjacent layer\n");

  for (i = 255; i >= 0; i--)
    {
      top->layer->setalpha(top->layer, i);
      top->layer->update(top->layer, LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);
    }

  dbg("Fill bottom layer1 with color red\n");

  ltdc_drawcolor(&bottom->vinfo, bottom->pinfo.fbmem,
                  bottom->vinfo.xres, bottom->vinfo.yres,
                  ltdc_color(&bottom->vinfo, LTDC_RED));

  dbg("Blend in red subjacent layer\n");

  for (i = 255; i >= 0; i--)
    {
      top->layer->setalpha(top->layer, i);
      top->layer->update(top->layer, LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);
    }

  dbg("Fill bottom layer1 with color green\n");

  ltdc_drawcolor(&bottom->vinfo, bottom->pinfo.fbmem,
                  bottom->vinfo.xres, bottom->vinfo.yres,
                  ltdc_color(&bottom->vinfo, LTDC_GREEN));

  dbg("Blend in green subjacent layer\n");

  for (i = 255; i >= 0; i--)
    {
      top->layer->setalpha(top->layer, i);
      top->layer->update(top->layer, LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);
    }

  dbg("Fill bottom layer1 with color blue\n");

  ltdc_drawcolor(&bottom->vinfo, bottom->pinfo.fbmem,
                  bottom->vinfo.xres, bottom->vinfo.yres,
                  ltdc_color(&bottom->vinfo, LTDC_BLUE));

  dbg("Blend in blue subjacent layer\n");

  for (i = 255; i >= 0; i--)
    {
      top->layer->setalpha(top->layer, i);
      top->layer->update(top->layer, LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);
    }

  dbg("Fill bottom layer1 with color white\n");

  ltdc_drawcolor(&bottom->vinfo, bottom->pinfo.fbmem,
                  bottom->vinfo.xres, bottom->vinfo.yres,
                  ltdc_color(&bottom->vinfo, LTDC_WHITE));

  dbg("Blend in white subjacent layer\n");

  for (i = 255; i >= 0; i--)
    {
      top->layer->setalpha(top->layer, i);
      top->layer->update(top->layer, LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);
    }

  /* Restore settings */

  area.xpos = 0;
  area.ypos = 0;
  area.xres = top->vinfo.xres;
  area.yres = top->vinfo.yres;

  top->layer->setarea(top->layer, &area, area.xpos, area.ypos);
  top->layer->setalpha(top->layer, 255);
  top->layer->setblendmode(top->layer, LTDC_BLEND_NONE);
  top->layer->update(top->layer, LTDC_UPDATE_SIM|LTDC_SYNC_VBLANK);
}

/******************************************************************************
 * Name: ltdc_flip_test
 *
 * Description:
 *   Perform layer flip test
 *
 *****************************************************************************/

static void ltdc_flip_test(void)
{
  FAR struct surface *active = ltdc_get_surface(LTDC_LAYER_ACTIVE);
  FAR struct surface *inactive = ltdc_get_surface(LTDC_LAYER_INACTIVE);

  /* Flip with non blend */

  dbg("Perform flip test without blending\n");

  dbg("active->pinfo.fbmem = %p\n", active->pinfo.fbmem);
  dbg("inactive->pinfo.fbmem = %p\n", inactive->pinfo.fbmem);

  dbg("Ensure that both layer opaque\n");
  active->layer->setalpha(active->layer, 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  active->layer->setblendmode(active->layer, LTDC_BLEND_NONE);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_NONE);

  dbg("Set the active layer to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));

  usleep(1000000);

  dbg("Set invisible layer to fullscreen blue\n");
  ltdc_drawcolor(&inactive->vinfo, inactive->pinfo.fbmem,
                    inactive->vinfo.xres, inactive->vinfo.yres,
                    ltdc_color(&inactive->vinfo, LTDC_BLUE));

  usleep(1000000);

  dbg("Flip layer to see the blue fullscreen\n");
  inactive->layer->update(inactive->layer,
                            LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Active layer is now inactive */

  dbg("Set invisible layer to fullscreen green\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_GREEN));

  usleep(1000000);

  dbg("Flip layer to see the green fullscreen\n");
  inactive->layer->update(inactive->layer,
                            LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  usleep(1000000);

  dbg("Set invisible layer to fullscreen red\n");
  ltdc_drawcolor(&inactive->vinfo, inactive->pinfo.fbmem,
                inactive->vinfo.xres, inactive->vinfo.yres,
                ltdc_color(&inactive->vinfo, LTDC_RED));

  usleep(1000000);

  dbg("Flip layer to see the red fullscreen\n");
  inactive->layer->update(inactive->layer, LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Flip with alpha blend */

  dbg("Perform flip test with alpha blending\n");

  /* Set the bottom layer to the current active layer */

  active = ltdc_get_surface(LTDC_LAYER_BOTTOM);
  inactive = ltdc_get_surface(LTDC_LAYER_TOP);

  dbg("Ensure that both layer fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));
  ltdc_drawcolor(&inactive->vinfo, inactive->pinfo.fbmem,
                    inactive->vinfo.xres, inactive->vinfo.yres,
                    ltdc_color(&inactive->vinfo, LTDC_BLACK));

  dbg("Ensure that both layer semitransparent\n");
  active->layer->setalpha(active->layer, 0x7f);
  inactive->layer->setalpha(inactive->layer, 0x7f);
  active->layer->setblendmode(active->layer, LTDC_BLEND_ALPHA);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_ALPHA);

  dbg("Enter in the flip mode sequence\n");
  dbg("Set the bottom layer to the active layer\n");
  dbg("Also update both layer simultaneous\n");
  active->layer->update(active->layer,LTDC_UPDATE_ACTIVATE|
                                      LTDC_UPDATE_SIM|
                                      LTDC_UPDATE_FLIP|
                                      LTDC_SYNC_VBLANK);

  usleep(1000000);

  dbg("Set invisible layer to fullscreen blue\n");
  ltdc_drawcolor(&inactive->vinfo, inactive->pinfo.fbmem,
                    inactive->vinfo.xres, inactive->vinfo.yres,
                    ltdc_color(&inactive->vinfo, LTDC_BLUE));

  usleep(1000000);

  dbg("Flip layer to see the blue fullscreen\n");
  inactive->layer->update(active->layer, LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Active layer is top now */

  dbg("Set invisible layer to fullscreen green\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_GREEN));

  usleep(1000000);

  dbg("Flip layer to see the green fullscreen\n");
  inactive->layer->update(active->layer,
                            LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Active layer is bottom now */

  dbg("Set invisible layer to fullscreen red\n");
  ltdc_drawcolor(&inactive->vinfo, inactive->pinfo.fbmem,
                    inactive->vinfo.xres, inactive->vinfo.yres,
                    ltdc_color(&inactive->vinfo, LTDC_RED));

  usleep(1000000);

  dbg("Flip layer to see the red fullscreen\n");
  inactive->layer->update(active->layer, LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  usleep(1000000);

  /* Active layer is top now */

  dbg("Set bottom layer back to fullscreen black\n");
  ltdc_drawcolor(&active->vinfo, active->pinfo.fbmem,
                    active->vinfo.xres, active->vinfo.yres,
                    ltdc_color(&active->vinfo, LTDC_BLACK));

  dbg("Set bottom layer to alpha %d and disable blend mode\n", 0xff);
  inactive->layer->setalpha(active->layer, 0xff);
  inactive->layer->setblendmode(active->layer, LTDC_BLEND_NONE);

  usleep(1000000);

  dbg("Flip layer to see the black fullscreen\n");
  inactive->layer->update(active->layer,
                            LTDC_UPDATE_FLIP|LTDC_SYNC_VBLANK);

  /* Active layer is bottom now */

  usleep(1000000);

  /* Disable flip sequence. Restore layers with there current settings and
   * activate them.
   */

  /* Restore settings  */

  dbg("Finally set the top layer back to fullscreen black\n");
  ltdc_drawcolor(&inactive->vinfo, inactive->pinfo.fbmem,
                    inactive->vinfo.xres, inactive->vinfo.yres,
                    ltdc_color(&inactive->vinfo, LTDC_BLACK));

  dbg("Set top layer to alpha %d and disable blend mode\n", 0xff);
  inactive->layer->setalpha(inactive->layer, 0xff);
  inactive->layer->setblendmode(inactive->layer, LTDC_BLEND_NONE);

  dbg("Flip to the top layer\n");
  inactive->layer->update(inactive->layer,
                            LTDC_UPDATE_ACTIVATE|LTDC_SYNC_VBLANK);

}
#endif /* CONFIG_STM32_LTDC_L2 */
#endif /* CONFIG_STM32_LTDC_INTERFACE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ltdc_clrcolor
 *
 * Description:
 *   Fills the color value table with an specific color.
 *   This works like memset but for 8-bit aligned memory.
 *
 * Parameter:
 *   color - Pointer to the color value table
 *   value - The color to set
 *   size  - the size of the color value table
 *
 ***************************************************************************/

void ltdc_clrcolor(uint8_t *color, uint8_t value, size_t size)
{
  while (size--)
    {
      *color++ = value;
    }
}

/****************************************************************************
 * Name: ltdc_cmpcolor
 *
 * Description:
 *   Compares two color value tables
 *
 * Parameter:
 *   color1 - color value table 1
 *   color2 - color value table 2
 *   size   - the size of the color value table
 *
 * Return:
 *   0 - if equal otherwise unequal to 0
 *
 ***************************************************************************/

int ltdc_cmpcolor(uint8_t *color1, uint8_t *color2, size_t size)
{
  int n;

  for (n = 0; !n && size; --size)
    {
      n = *color1++ - *color2++;
    }

  return n;
}

/****************************************************************************
 * Name: ltdc_init_cmap
 *
 * Description:
 *   Initialize the color lookup table
 *
 ***************************************************************************/

#ifdef CONFIG_FB_CMAP
void ltdc_init_cmap(void)
{
  memset(&g_color, 0, sizeof(g_color));

  /* CLUT color format definition
   *
   * Position value
   * 00       0x000000    black
   * 01       0xff0000    red
   * 02       0x00ff00    green
   * 03       0x0000ff    blue
   * 04       0xffffff    white
   *
   */
  g_cmap.red[1]    = 0xff;
  g_cmap.green[2]  = 0xff;
  g_cmap.blue[3]   = 0xff;
  g_cmap.red[4]    = 0xff;
  g_cmap.green[4]  = 0xff;
  g_cmap.blue[4]   = 0xff;
#ifdef CONFIG_FB_TRANSPARENCY
  g_cmap.transp[0] = 0xff;
  g_cmap.transp[1] = 0xff;
  g_cmap.transp[2] = 0xff;
  g_cmap.transp[3] = 0xff;
  g_cmap.transp[4] = 0xff;
#endif
}


/****************************************************************************
 * Name: ltdc_createcmap
 *
 * Description:
 *   Initialize
 *
 ***************************************************************************/

FAR struct fb_cmap_s * ltdc_createcmap(uint16_t ncolors)
{
  FAR struct fb_cmap_s *cmap = malloc(sizeof(struct fb_cmap_s));

  if (cmap)
    {
#ifdef CONFIG_FB_TRANSPARENCY
      uint8_t *clut = malloc(4 * ncolors * sizeof(uint8_t));
#else
      uint8_t *clut = malloc(3 * ncolors * sizeof(uint8_t));
#endif

      if (!clut)
        {
          dbg("malloc() failed\n");
          free(cmap);
          return NULL;;
        }

#ifdef CONFIG_FB_TRANSPARENCY
      cmap->transp = &clut[0];
      cmap->red = &clut[1 * ncolors * sizeof(uint8_t)];
      cmap->green = &clut[2 * ncolors * sizeof(uint8_t)];
      cmap->blue = &clut[3 * ncolors * sizeof(uint8_t)];
#else
      cmap->red = &clut[0];
      cmap->green = &clut[1 * ncolors * sizeof(uint8_t)];
      cmap->blue = &clut[2 * ncolors * sizeof(uint8_t)];
#endif
      cmap->first = 0;
      cmap->len   = ncolors;

    }

  return cmap;
}


/****************************************************************************
 * Name: ltdc_deletecmap
 *
 * Description:
 *   Initialize
 *
 ***************************************************************************/

void ltdc_deletecmap(FAR struct fb_cmap_s *cmap)
{
  if (cmap)
    {
#ifdef CONFIG_FB_TRANSPARENCY
      free(cmap->transp);
#else
      free(cmap->red);
#endif
      free(cmap);
    }
}
#endif

/****************************************************************************
 * Name: ltdc_color
 *
 * Description:
 *   Get the correct color value to the pixel format
 *
 ***************************************************************************/

uint32_t ltdc_color(FAR struct fb_videoinfo_s *vinfo, uint8_t color)
{
  uint32_t value;

  switch (vinfo->fmt)
    {
#if defined(CONFIG_STM32_LTDC_L1_L8) || defined(CONFIG_STM32_LTDC_L2_L8)
        case FB_FMT_RGB8:
          value = color;
          break;
#endif
#if defined(CONFIG_STM32_LTDC_L1_RGB565) || defined(CONFIG_STM32_LTDC_L2_RGB565)
        case FB_FMT_RGB16_565:
          value = g_rgb16[color];
          break;
#endif
#if defined(CONFIG_STM32_LTDC_L1_RGB888) || \
        defined(CONFIG_STM32_LTDC_L2_RGB888)
        case FB_FMT_RGB24:
          value = g_rgb24[color];
          break;
#endif
        default:
          dbg("Unsupported pixel format %d\n", vinfo->fmt);
          value = 0;
          break;
    }

  return value;
}

/***************************************************************************
 * Name: ltdc_simple_draw
 *
 * Description:
 *   Draw four different colored rectangles on the whole screen
 *
 ***************************************************************************/

void ltdc_simple_draw(FAR struct fb_videoinfo_s *vinfo,
                                FAR struct fb_planeinfo_s *pinfo)
{
  volatile int x, y;
  uint16_t xres = vinfo->xres;
  uint16_t yres = vinfo->yres;

  dbg("draw a red and green rectangle in the upper half\n");
  dbg("draw a white and blue rectangle in the lower half\n");

#if defined(CONFIG_STM32_LTDC_L1_L8) || defined(CONFIG_STM32_LTDC_L2_L8)
  if (vinfo->fmt == FB_FMT_RGB8)
    {
      uint8_t color;
      uint8_t *buf = (uint8_t *)pinfo->fbmem;

      for (y = 0; y < yres/2; y++)
        {
          color = ltdc_color(vinfo, LTDC_RED);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }

          color = ltdc_color(vinfo, LTDC_GREEN);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }
        }

      for (y = 0; y < yres/2; y++)
        {
          color = ltdc_color(vinfo, LTDC_WHITE);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }

          color = ltdc_color(vinfo, LTDC_BLUE);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }
        }
    }
#endif

#if defined(CONFIG_STM32_LTDC_L1_RGB565) || defined(CONFIG_STM32_LTDC_L2_RGB565)
  if(vinfo->fmt == FB_FMT_RGB16_565)
    {
      uint16_t color;
      uint16_t *buf = (uint16_t *)pinfo->fbmem;

      for (y = 0; y < yres/2; y++)
        {
          color = ltdc_color(vinfo, LTDC_RED);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }

          color = ltdc_color(vinfo, LTDC_GREEN);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }
        }

      for (y = 0; y < yres/2; y++)
        {
          color = ltdc_color(vinfo, LTDC_WHITE);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }

          color = ltdc_color(vinfo, LTDC_BLUE);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
            }
        }
    }
#endif

#if defined(CONFIG_STM32_LTDC_L1_RGB888) || defined(CONFIG_STM32_LTDC_L2_RGB888)
  if(vinfo->fmt == FB_FMT_RGB24)
    {
      uint32_t color;
      uint8_t *buf = (uint8_t *)pinfo->fbmem;

      for (y = 0; y < yres/2; y++)
        {
          color = ltdc_color(vinfo, LTDC_RED);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
              *buf++ = (color >> 8);
              *buf++ = (color >> 16);
            }

          color = ltdc_color(vinfo, LTDC_GREEN);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
              *buf++ = (color >> 8);
              *buf++ = (color >> 16);
            }
        }

      for (y = 0; y < yres/2; y++)
        {
          color = ltdc_color(vinfo, LTDC_WHITE);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
              *buf++ = (color >> 8);
              *buf++ = (color >> 16);
            }

          color = ltdc_color(vinfo, LTDC_BLUE);

          for (x = 0; x < xres/2; x++)
            {
              *buf++ = color;
              *buf++ = (color >> 8);
              *buf++ = (color >> 16);
            }
        }
    }
#endif
}

#ifdef CONFIG_STM32_LTDC_L2
/******************************************************************************
 * Name: ltdc_drawcolor
 *
 * Description:
 *   Draw a specific color to the framebuffer
 *
 *****************************************************************************/

void ltdc_drawcolor(FAR struct fb_videoinfo_s *vinfo, void *buffer,
                           uint16_t xres, uint16_t yres, uint32_t color)
{
  int x,y;

  /* draw a blue rectangle */

  dbg("draw a full screen rectangle with color %08x\n", color);

#if defined(CONFIG_STM32_LTDC_L1_L8) || defined(CONFIG_STM32_LTDC_L2_L8)
  if (vinfo->fmt == FB_FMT_RGB8)
    {
      uint8_t *buf = (uint8_t *)buffer;

      for (y = 0; y < yres; y++)
        {
          for (x = 0; x < xres; x++)
            {
              *buf++ = color;
            }
        }
    }
#endif

#if defined(CONFIG_STM32_LTDC_L1_RGB565) || defined(CONFIG_STM32_LTDC_L2_RGB565)
  if (vinfo->fmt == FB_FMT_RGB16_565)
    {
      uint16_t *buf = (uint16_t *)buffer;

      for (y = 0; y < yres; y++)
        {
          for (x = 0; x < xres; x++)
            {
              *buf++ = color;
            }
        }
    }
#endif

#if defined(CONFIG_STM32_LTDC_L1_RGB888) || defined(CONFIG_STM32_LTDC_L2_RGB888)
  if (vinfo->fmt == FB_FMT_RGB24)
    {
      uint8_t *buf = (uint8_t *)buffer;
      uint8_t r;
      uint8_t g;
      uint8_t b;

      r = (uint8_t)(color >> 16);
      g = (uint8_t)(color >> 8);
      b = (uint8_t)color;

      for (y = 0; y < yres; y++)
        {
          for (x = 0; x < xres; x++)
            {
              *buf++ = b;
              *buf++ = g;
              *buf++ = r;
            }
        }
    }
#endif
}
#endif

#ifdef CONFIG_STM32_LTDC_INTERFACE
/******************************************************************************
 * Name: ltdc_get_surface
 *
 * Description:
 *   Get a reference to a specific layer
 *
 *****************************************************************************/

struct surface * ltdc_get_surface(uint32_t mode)
{
  int ret;
  int lid;
  FAR struct surface *sur = &g_surface[0];

  ret = sur->layer->getlid(sur->layer, &lid, mode);

  if (ret != OK)
    {
      dbg("getlid() failed\n");
      _exit(1);
    }

  if (lid < 0 || lid > 1)
    {
      dbg("invalid layer id %d\n", lid);
      _exit(1);
    }

  return &g_surface[lid];
}
#endif


/****************************************************************************
 * ltdc_main
 ****************************************************************************/

int ltdc_main(int argc, char *argv[])
{
  FAR struct fb_planeinfo_s pinfo;
  FAR struct fb_videoinfo_s vinfo;
  FAR struct fb_vtable_s *fbtable;

  if (up_fbinitialize()<0)
    {
      dbg("up_fbinitialize() failed\n");
      return -1;
    }

  fbtable = up_fbgetvplane(0);

  if (!fbtable)
    {
      dbg("up_fbgetvplane() failed\n");
      return -1;
    }

  if (fbtable->getvideoinfo(fbtable, &vinfo)<0)
    {
      dbg("getvideoinfo failed\n");
      return -1;
    }

  if (fbtable->getplaneinfo(fbtable, 0, &pinfo)<0)
    {
      dbg("getplaneinfo failed\n");
      return -1;
    }

  dbg("fb is configured with: xres = %d, yres = %d, \
          fb start address = %p, fb size = %d, fmt = %d, bpp = %d\n",
          vinfo.xres, vinfo.yres, pinfo.fbmem, pinfo.fblen,
          vinfo.fmt, pinfo.bpp);

#ifdef CONFIG_FB_CMAP
  ltdc_init_cmap();

  if (vinfo.fmt == FB_FMT_RGB8)
    {
      if (fbtable->putcmap(fbtable, &g_cmap) != OK)
        {
          dbg("putcmap() failed\n");
          return -1;
        }
    }
#endif

  /* Tests */

  ltdc_simple_draw(&vinfo, &pinfo);

  usleep(1000000);

#ifdef CONFIG_STM32_LTDC_INTERFACE
  ltdc_init_surface(0, LTDC_LAYER_ACTIVE);
#ifdef CONFIG_STM32_LTDC_L2
  ltdc_init_surface(1, LTDC_LAYER_INACTIVE);
#endif
  usleep(1000000);

  ltdc_setget_test();

  usleep(1000000);

  ltdc_area_test();

  usleep(1000000);

  ltdc_color_test();

  usleep(1000000);

  ltdc_colorkey_test();

  usleep(1000000);

  ltdc_common_test();

#ifdef CONFIG_STM32_LTDC_L2

  usleep(1000000);

  ltdc_alpha_blend_test();

  usleep(1000000);

  ltdc_flip_test();

#ifdef CONFIG_STM32_DMA2D
  ltdc_dma2d_main();
#endif

#endif /* CONFIG_STM32_LTDC_L2 */
#endif /* CONFIG_STM32_LTDC_INTERFACE */
  return 0;
}
