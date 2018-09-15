/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_pcx.h
 * This is the header file associated with wld_pcx.c
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

#ifndef __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PALTABLE_H
#define __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PALTABLE_H

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*************************************************************************
 * Included files
 *************************************************************************/

#include <stdio.h>

/*************************************************************************
 * Public Type Definitions
 ************************************************************************/

/* Return codes */

#if MSWINDOWS
enum
{
  PCX_SUCCESS = 0,
  PCX_ALLOCATION_FAILURE = 40,
  PCX_OPEN_ERROR
};
#endif

struct pcx_header_s
{
  char manufacturer;
  char version;
  char encoding;
  char bits_per_pixel;
  int16_t x, y;
  int16_t width, height;
  int16_t horz_res;
  int16_t vert_res;
  char ega_palette[48];
  char reserved;
  char num_color_planes;
  int16_t bytes_per_line;
  int16_t palette_type;
  char padding[58];
};

typedef struct pcx_header_s pcx_header_t;
typedef struct pcx_header_s *pcx_pheader_t;

#define SIZEOF_PCX_HEADER 128

#if MSWINDOWS
struct pcx_picture_t
{
  pcx_header_t header;
  uint8_t    *buffer;
  color_rgb_t *palette;
};

typedef struct pcx_picture_t
typedef struct pcx_ppicture_t
#endif

/*************************************************************************
 * Public Function Prototypes
 *************************************************************************/

#if MSWINDOWS
uint8_t wld_pcx_init(pcxppicture_t image, uint16_t height, uint16_t width,
                    color_rgb_t *palette, uint8_t *buffer);
uint8_t wld_loadpcx(char *filename, pcxppicture_t image);
#else
graphic_file_t *wld_loadpcx(FILE *fp, char *filename);
#endif

/*************************************************************************
 * Public Variables
 *************************************************************************/

#ifdef  __cplusplus
}
#endif

#endif /* __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_PALTABLE_H */
