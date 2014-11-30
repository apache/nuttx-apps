/****************************************************************************
 * apps/graphics/traveler/include/trv_graphics.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct trv_palette_s
{
  int ncolors;                  /* Number of colors in the look-up table */
  FAR nxgl_mxpixel_t *lut;      /* Color lookup table */
};

struct trv_graphics_info_s
{
  nxgl_coord_t width;           /* Image width */
  nxgl_coord_t height;          /* Image height */
  struct trv_palette_s palette; /* Color palette */
  FAR nxgl_mxpixel_t *buffer;   /* Hardware framebuffer */
};

struct trv_framebuffer_s
{
  nxgl_coord_t width;           /* Image width */
  nxgl_coord_t height;          /* Image height */
  FAR trv_pixel_t *buffer;      /* Software render buffer */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int trv_graphics_initialize(uint16_t width, uint16_t height,
                            uint8_t scale_factor,
                            FAR struct trv_graphics_info_s *ginfo);
void trv_graphics_terminate(void);
trv_pixel_t trv_graphics_index2pixel(int index);
void trv_display_update(struct trv_framebuffer_s *fb);
trv_pixel_t *trv_get_renderbuffer(uint16_t width, uint16_t height);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICS_H */
