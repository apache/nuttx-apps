/****************************************************************************
 * examples/ltdc/ltdc.h
 *
 *   Copyright (C) 2015 Marco Krahl. All rights reserved.
 *   Author: Marco Krahl <ocram.lhark@gmail.com>
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

#ifndef _EXAMPLES_LTDC_LTDC_H
#define _EXAMPLES_LTDC_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>

#include <nuttx/video/rgbcolors.h>
#include <nuttx/video/fb.h>

#include <arch/chip/ltdc.h>
#ifdef CONFIG_STM32_DMA2D
# include <arch/chip/dma2d.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LTDC_EXAMPLE_NCOLORS    5

#ifdef CONFIG_STM32_LTDC_INTERFACE
struct surface
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR struct ltdc_layer_s *layer;
#ifdef CONFIG_STM32_DMA2D
  FAR struct dma2d_layer_s *dma2d;
#endif
};

# ifdef CONFIG_STM32_DMA2D
struct dma2d_surface
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR struct dma2d_layer_s *dma2d;
};
# endif
#endif

enum example_colors
{
  LTDC_BLACK,
  LTDC_RED,
  LTDC_GREEN,
  LTDC_BLUE,
  LTDC_WHITE
};

static const uint32_t g_rgb24[LTDC_EXAMPLE_NCOLORS] =
{
  RGB24_BLACK,
  RGB24_RED,
  RGB24_GREEN,
  RGB24_BLUE,
  RGB24_WHITE,
};

static const uint32_t g_rgb16[LTDC_EXAMPLE_NCOLORS] =
{
  RGB16_BLACK,
  RGB16_RED,
  RGB16_GREEN,
  RGB16_BLUE,
  RGB16_WHITE,
};

/****************************************************************************
 * Public Functions
****************************************************************************/

void ltdc_clrcolor(uint8_t *color, uint8_t value, size_t size);
int ltdc_cmpcolor(uint8_t *color1, uint8_t *color2, size_t size);

#ifdef CONFIG_FB_CMAP
void ltdc_init_cmap(void);
FAR struct fb_cmap_s * ltdc_createcmap(uint16_t ncolors);
void ltdc_deletecmap(FAR struct fb_cmap_s *cmap);
#endif
uint32_t ltdc_color(FAR struct fb_videoinfo_s *vinfo, uint8_t color);
void ltdc_simple_draw(FAR struct fb_videoinfo_s *vinfo,
                        FAR struct fb_planeinfo_s *pinfo);

#ifdef CONFIG_STM32_LTDC_L2
void ltdc_drawcolor(FAR struct fb_videoinfo_s *vinfo, void *buffer,
                           uint16_t xres, uint16_t yres, uint32_t color);
#endif

#ifdef CONFIG_STM32_LTDC_INTERFACE
struct surface * ltdc_get_surface(uint32_t mode);
#endif

#ifdef CONFIG_STM32_DMA2D
void ltdc_dma2d_main(void);
#endif
#endif /* _EXAMPLES_LTDC_LTDC_H */
