/****************************************************************************
 * apps/graphics/traveler/src/trv_planefiles.c
 * This file contains the logic to manage the world data files
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

/****************************************************************************
 * Included files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_plane.h"

#include <stdio.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_load_worldplane
 *
 * Description:
 *
 *   This function loads the world data for one plane
 *
 ****************************************************************************/

static int trv_load_worldplane(FAR FILE *fp, FAR struct trv_rect_head_s *head,
                               uint8_t nrects)
{
  FAR struct trv_rect_list_s *rect;
  int ret;
  int i;

  for (i = 0; i < nrects; i++)
    {
      /* Allocate space for the next rectangle */

      rect = trv_new_plane();
      if (!rect)
        {
          return -ENOMEM;
        }

      /* Read the next rectangle from the input file */

      ret = fread((char*)&rect->d, RESIZEOF_TRVRECTDATA_T, 1, fp);
      if (ret != 1)
        {
          int errcode = errno;
          fprintf(stderr, "Error: read of rectangle %d (of %d) failed\n",
                  i, nrects);
          fprintf(stderr, "feof=%d ferror=%d errno=%d\n",
                  feof(fp), ferror(fp), errcode);
          return -errcode;
        }

      /* Put the new rectangle into the plane list */

      trv_add_plane(rect, head);
    }

  return OK;
}

/****************************************************************************
 * Function: trv_load_planes
 * Description:
 * This function loads the world data from the opened file
 ****************************************************************************/

static int trv_load_planes(FAR FILE *fp)
{
  struct trv_planefile_header_s header;
  int ret;

  /* Read the plane file header */

  ret = fread((char*)&header, SIZEOF_TRVPLANEFILEHEADER_T, 1, fp);
  if (ret != 1)
    {
      int errcode = errno;
      fprintf(stderr, "Error: Failed to read of file header\n");
      fprintf(stderr, "feof=%d ferror=%d errno=%d\n",
              feof(fp), ferror(fp), errcode);
      return -errcode;
    }

  /* Then load each grid, performing run length (rle) decoding */

  ret = trv_load_worldplane(fp, &g_xplane, header.nxrects);
  if (ret == OK)
    {
      ret = trv_load_worldplane(fp, &g_yplane, header.nyrects);
    }

  if (ret == OK)
    {
      ret = trv_load_worldplane(fp, &g_zplane, header.nzrects);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_load_planefile
 *
 * Description:
 *
 * This function opens the input file and loads the world plane data from it
 *
 ****************************************************************************/

int trv_load_planefile(FAR const char *wldfile)
{
  FAR FILE *fp;
  int ret;

  /* Open the map file which contains the description of the world */

  fp = fopen(wldfile, "rb");
  if (fp == NULL)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Could not open plane file=\"%s\": %d\n",
              wldfile, errcode);
      return -errcode;
    }

  /* Load the world data from the file */

  ret = trv_load_planes(fp);

  /* Close the file */

  fclose(fp);
  return ret;
}

