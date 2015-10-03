/****************************************************************************
 * apps/graphics/traveler/src/trv_bitmapfile.c
 * This file contains the logic which loads texture bitmaps
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
#include "trv_mem.h"
#include "trv_world.h"
#include "trv_bitmaps.h"
#include "trv_fsutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_read_filename
 *
 * Description:
 *   Read a file name from the input stream
 *
 ****************************************************************************/

static int trv_read_filename(FAR FILE  *fp, FAR char *filename)
{
  int16_t nbytes;
  int ch;

  /* Skip over any leading whitespace */

  do
    {
      ch = getc(fp);
    }
  while (isspace(ch));

  if (ch == EOF)
    {
      return -ENOENT;
    }

  /* Get the file name from the file */

  nbytes = 0;
  for (;;)
    {
      /* Everything up to the next whitespace must be the filename */

      if (isspace(ch) || ch == EOF)
        {
          /* End of the string. Don't forget the ASCIIZ terminator */

          filename[nbytes] = '\0';
          break;
        }

      /* Make sure that the file name is not too large */

      else if (nbytes >= FILENAME_MAX)
        {
          return -E2BIG;
        }

      /* Add the new character to the file name */

      else
        {
          filename[nbytes] = (char)ch;
          nbytes++;
        }

      /* Get the character for the next time through the loop.  Every file
       * name should be terminated with a space or a new line (or, possibily,
       * EOF)
       */

      ch = getc(fp);
    }

  return OK;
}

/****************************************************************************
 * Name: trv_load_bitmaps
 *
 * Description:
 *   This function loads the world data from the input file
 *
 ****************************************************************************/

static int trv_load_bitmaps(FAR FILE *fp, FAR const char *wldpath)
{
  FAR char *fullpath;
  char filename[FILENAME_MAX];
  int mapndx;
  int ret = OK;

  /* Discard any bitmaps that we may currently have buffered */

  trv_release_bitmaps();

  /* Get the number of bitmaps in the bitmap file */

  g_trv_nbitmaps = trv_read_decimal(fp);
  if (g_trv_nbitmaps >= MAX_BITMAPS)
    {
      return -E2BIG;
    }

  /* Read the colors used to rend the sky and ground */

  g_sky_color    = trv_read_decimal(fp);
  g_ground_color = trv_read_decimal(fp);

  /* Load each bitmap file */

  for (mapndx = 0; mapndx < g_trv_nbitmaps && ret == OK; mapndx++)
    {
      /* Load the even bitmap */
      /* Get the name of the file which contains the even bitmap */

      ret = trv_read_filename(fp, filename);
      if (ret < 0)
        {
          return ret;
        }

      /* Get the full path to the even bit map file */

      fullpath = trv_fullpath(wldpath, filename);

      /* Read the bitmap texture file */

      g_even_bitmaps[mapndx] = trv_read_texture(fullpath);
      free(fullpath);

      if (!g_even_bitmaps[mapndx])
        {
          return -EIO;
        }

      /* Load the odd bitmap */
      /* Get the name of the file which contains the odd bitmap */

      ret = trv_read_filename(fp, filename);
      if (ret < 0)
        {
          return ret;
        }

#ifndef WEDIT
      /* Get the full path to the even bit map file */

      fullpath = trv_fullpath(wldpath, filename);

      /* Read the bitmap texture file */

      g_odd_bitmaps[mapndx] = trv_read_texture(fullpath);
      free(fullpath);

      if (!g_odd_bitmaps[mapndx])
        {
          return -EIO;
        }
#endif
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_load_bitmapfile
 *
 * Description:
 *   This function opens the input file and loads the world data from it
 *
 ****************************************************************************/

int trv_load_bitmapfile(FAR const char *bitmapfile, FAR const char *wldpath)
{
  FAR FILE *fp;
  int ret;

  /* Open the file which contains the names of all of the bitmap files */

  fp = fopen(bitmapfile, "r");
  if (!fp)
    {
      int errcode = errno;
      return -errcode;
    }

  /* Load all of the bitmaps */

  ret = trv_load_bitmaps(fp, wldpath);
  if (ret < 0)
    {
      trv_release_bitmaps();
    }

  /* We are all done with the file, so close it */

  fclose(fp);
  return ret;
}

