/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_loadplanes.c
 * This file contains the logic to load the world data from the opened file
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
 * Included files
 *************************************************************************/

#include <stdio.h>
#include "wld_plane.h"

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_load_worldplane
 * Description:
 * This function loads the world data for one plane
 ************************************************************************/

static uint8_t wld_load_worldplane(FILE *fp, rect_head_t *list,
                                   uint8_t nrects)
{
  rect_list_t *rect;
  int i;

  for (i = 0; i < nrects; i++)
    {
      /* Allocate space for the next rectangle */

      rect = wld_new_plane();
      if (!rect)
        {
          return PLANE_ALLOCATION_FAILURE;
        }

      /* Read the next rectangle from the input file */

      if (fread((char *)&rect->d, SIZEOF_RECTDATATYPE, 1, fp) != 1)
        {
          fprintf(stderr, "ERROR: read of rectangle %d (of %d) failed! ",
                  i, nrects);
          fprintf(stderr, "feof=%d ferror=%d\n", feof(fp), ferror(fp));
          return PLANE_DATA_READ_ERROR;
        }

      /* Put the new rectangle into the plane list */

      wld_add_plane(rect, list);
    }

  return PLANE_SUCCESS;
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_load_planes
 * Description:
 * This function loads the world data from the opened file
 ************************************************************************/

uint8_t wld_load_planes(FILE *fp)
{
  plane_file_header_t fileheader;
  uint8_t result;

  /* Read the plane file header */

  if (fread((char *)&fileheader, SIZEOF_PLANEFILEHEADERTYPE, 1, fp) != 1)
    {
      return PLANE_HEADER_READ_ERROR;
    }

  /* Then load each grid, performing run length (rle) decoding */

  result = wld_load_worldplane(fp, &g_xplane_list, fileheader.num_xrects);
  if (!result)
    {
      result = wld_load_worldplane(fp, &g_yplane_list,
                                   fileheader.num_yrects);
    }

  if (!result)
    {
      result = wld_load_worldplane(fp, &g_zplane_list,
                                   fileheader.num_zrects);
    }

  return result;
}
