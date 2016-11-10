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

#ifndef __ASTGRAPHICFILE_H
#define __ASTGRAPHICFILE_H

/*************************************************************************
 * Included files
 *************************************************************************/

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

typedef enum
{
  formatGIF87,
  formatGIF89,
  formatPPM,
  formatPCX,
  formatUnknown
} graphic_file_format_t;

typedef enum
{
  gfTrueColor,
  gfPaletted
} graphic_file_enum_t;

typedef struct
{
  graphic_file_enum_t type;
  uint16_t height, width;
  int palette_entries;
  long transparent_entry;
  RGBColor *palette;
  uint8_t *bitmap;
} graphic_file_t;

/*************************************************************************
 * Global Function Prototypes
 *************************************************************************/

/*************************************************************************
 * Global Variables
 *************************************************************************/

/*************************************************************************
 * Private Variables
 *************************************************************************/

graphic_file_t *wld_new_graphicfile(void);
void             wld_free_graphicfile(graphic_file_t *gFile);
RGBColor         wld_graphicfile_pixel(graphic_file_t *gFile,
                                            int x, int y);
graphic_file_t *wld_readgraphic_file(char *filename);

#endif /* __ASTGRAPHICFILE_H */
