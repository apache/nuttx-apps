/****************************************************************************
 * apps/graphics/traveler/include/trv_graphicfile.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICFILE_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICFILE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* MAGIC_LENGTH must be the length of the longest MAGIC string. */

#define MAGIC_LENGTH 5
#define GIF_MAGIC    "GIF87"
#define GIF89_MAGIC  "GIF89"
#define PPM_MAGIC    "P6"

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

enum trv_graphicfile_format_e
{
  GRAPHICFILE_UNKNOWN = 0,
  GRAPHICFILE_GIF87,
  GRAPHICFILE_GIF89,
  GRAPHICFILE_PPM,
  GRAPHICFILE_PCX
};

enum trv_graphicfile_type_e
{
  GRAPHICFILE_TRUECOLOR = 0,
  GRAPHICFILE_PALETTED
};

struct trv_graphicfile_s
{
  uint8_t type;                 /* See enum trv_graphicfile_type_e */
  uint16_t height;
  uint16_t width;
  int palette_entries;
  long transparent_entry;
  FAR struct trv_color_rgb_s *palette;
  FAR uint8_t *bitmap;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct trv_graphicfile_s *trv_graphicfile_new(void);
void trv_graphicfile_free(struct trv_graphicfile_s *gfile);
struct trv_color_rgb_s
  trv_graphicfile_pixel(FAR struct trv_graphicfile_s *gfile, int x, int y);
struct trv_graphicfile_s *tvr_graphicfile_read(FAR char *filename);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_GRAPHICFILE_H */
