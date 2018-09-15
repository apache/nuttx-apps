/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_saveplanes.c
 * This file contains the logic which stores the world data into the specified
 * file
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
 * Name: wld_count_rectangles
 * Description:
 * This function counts the number of rectangles in one plane
 ************************************************************************/

static uint16_t wld_count_rectangles(rect_list_t * rect)
{
  uint16_t count;
  for (count = 0; (rect); count++, rect = rect->flink);
  return count;
}

/*************************************************************************
 * Name: wld_save_worldplane
 * Description:
 * This function stores the world data for one plane
 ************************************************************************/

static uint8_t wld_save_worldplane(FILE * fp, rect_list_t * rect)
{
  /* For each rectangle in the list */

  for (; (rect); rect = rect->flink)
    {
      /* Write the rectangle to the output file */

      if (fwrite((char *)&rect->d, SIZEOF_RECTDATATYPE, 1, fp) != 1)
        return PLANE_DATA_WRITE_ERROR;
    }

  return PLANE_SUCCESS;
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_save_planes
 * Description:
 * This function stores the world data into the specified file
 ************************************************************************/

uint8_t wld_save_planes(const char *wldfile)
{
  FILE *fp;
  plane_file_header_t fileHeader;
  uint8_t result;

  /* Open the file which contains the description of the world */

  fp = fopen(wldfile, "wb");
  if (!fp)
    {
      return PLANE_WRITE_OPEN_ERROR;
    }

  /* Create world file header */

  fileHeader.num_xrects = wld_count_rectangles(g_xplane_list.head);
  fileHeader.num_yrects = wld_count_rectangles(g_yplane_list.head);
  fileHeader.num_zrects = wld_count_rectangles(g_zplane_list.head);

  /* Write the file header to the output file */

  if (fwrite((char *)&fileHeader, SIZEOF_PLANEFILEHEADERTYPE, 1, fp) != 1)
    {
      result = PLANE_HEADER_WRITE_ERROR;
    }

  /* Save the world, one plane at a time */

  else
    {
      result = wld_save_worldplane(fp, g_xplane_list.head);
      if (result == PLANE_SUCCESS)
        {
          result = wld_save_worldplane(fp, g_yplane_list.head);
        }

      if (result == PLANE_SUCCESS)
        {
          wld_save_worldplane(fp, g_zplane_list.head);
        }
    }

  /* Close the file */

  fclose(fp);
  return result;
}
