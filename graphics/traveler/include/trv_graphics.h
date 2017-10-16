/****************************************************************************
 * apps/graphics/traveler/include/trv_graphics.h
 *
 *   Copyright (C) 2014, 2017 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICS_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include <semaphore.h>

#include <nuttx/video/fb.h>
#ifdef CONFIG_GRAPHICS_TRAVELER_NX
#  include <nuttx/nx/nx.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_GRAPHICS_TRAVELER_RGB16_565)
#  define TRV_BPP        16
#  define TRV_COLOR_FMT  FB_FMT_RGB16_565
#elif defined(CONFIG_GRAPHICS_TRAVELER_RGB32_888)
#  define TRV_BPP        32
#  define TRV_COLOR_FMT  FB_FMT_RGB32
#else
#  error No color format defined
#endif

#define MAX_REND_WIDTH   480
#define MAX_REND_HEIGHT  240

/****************************************************************************
 * Public Types
 ****************************************************************************/

#if TRV_BPP == 16
typedef uint16_t dev_pixel_t;  /* Width of one hardware pixel */
#elif TRV_BPP == 32
typedef uint32_t dev_pixel_t;  /* Width of one hardware pixel */
#endif

struct trv_palette_s
{
  int ncolors;                  /* Number of colors in the look-up table */
  FAR dev_pixel_t *lut;         /* Color lookup table */
};

struct trv_graphics_info_s
{
#ifdef CONFIG_GRAPHICS_TRAVELER_NX
  NXHANDLE hnx;                 /* The connection handle */
  NXHANDLE bgwnd;               /* Background window handle */
#else
  int fb;                       /* Framebuffer device file descriptor */
  trv_coord_t xoffset;          /* Horizontal offset to start of data (pixels) */
  trv_coord_t yoffset;          /* Vertical offset to start of data (rows) */
  trv_coord_t stride;           /* Length of a line (bytes) */
#endif
  trv_coord_t xres;             /* Physical display width (pixels) */
  trv_coord_t yres;             /* Physical display height (rows) */
  trv_coord_t imgwidth;         /* Width of visible display region (bytes) */
  uint8_t xscale;               /* Horizontal image scale factor */
  uint8_t yscale;               /* Vertical image scale factor */
  struct trv_palette_s palette; /* Color palette */
  FAR dev_pixel_t *hwbuffer;    /* Hardware frame buffer */
  FAR trv_pixel_t *swbuffer;    /* Software render buffer */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
extern FAR const struct nx_callback_s *g_trv_nxcallback;
extern sem_t g_trv_nxevent;
extern volatile bool g_trv_nxresolution;
extern volatile bool g_trv_nxrconnected;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int trv_graphics_initialize(FAR struct trv_graphics_info_s *ginfo);
void trv_graphics_terminate(FAR struct trv_graphics_info_s *ginfo);
trv_pixel_t trv_graphics_index2pixel(int index);
void trv_display_update(struct trv_graphics_info_s *fb);
trv_pixel_t *trv_get_renderbuffer(uint16_t width, uint16_t height);
FAR void *trv_nxlistener(FAR void *arg);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICS_H */
