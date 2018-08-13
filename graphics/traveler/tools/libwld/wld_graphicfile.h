/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_graphicfile.h
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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

/*************************************************************************
 * FILE: wld_graphicfile.h
 * DESCRIPTION:
 *************************************************************************/

#ifndef __APPS_GRAPHICS_TRAVELER_TOOLS_LIBWLD_WLD_GRAPHIFILE_H
#define __APPS_GRAPHICS_TRAVELER_TOOLS_LIBWLD_WLD_GRAPHIFILE_H

/*************************************************************************
 * Included files
 *************************************************************************/

#include "wld_color.h"

/*************************************************************************
 * Compilation switches
 *************************************************************************/

/*************************************************************************
 * Definitions
 *************************************************************************/

/* MAGIC_LENGTH must be the length of the longest MAGIC string. */

#define MAGIC_LENGTH 5
#define GIF_MAGIC    "GIF87"
#define GIF89_MAGIC  "GIF89"
#define PPM_MAGIC    "P6"

/*************************************************************************
 * Public Type Definitions
 ************************************************************************/

enum graphic_file_format_e
{
  FORMAT_GIT87,
  FORMAT_GIT89,
  FORMAT_PPM,
  FORMAT_PCX,
  FORMAT_UNKNOWN
};

typedef enum graphic_file_format_e graphic_file_format_t;

enum graphic_file_enum_e
{
  GFILE_TRUECOLOR,
  GFILE_PALETTED
};

typedef enum graphic_file_enum_e graphic_file_enum_t;

struct graphic_file_e
{
  graphic_file_enum_t type;
  uint16_t height;
  uint16_t width;
  int palette_entries;
  long transparent_entry;
  color_rgb_t *palette;
  uint8_t *bitmap;
};

typedef struct graphic_file_e graphic_file_t;

/*************************************************************************
 * Public Function Prototypes
 *************************************************************************/

graphic_file_t *wld_new_graphicfile(void);
void            wld_free_graphicfile(graphic_file_t *gFile);
color_rgb_t     wld_graphicfile_pixel(graphic_file_t *gFile,
                                      int x, int y);
graphic_file_t *wld_readgraphic_file(char *filename);

#endif /* __APPS_GRAPHICS_TRAVELER_TOOLS_LIBWLD_WLD_GRAPHIFILE_H */
