/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_loadppm.c
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
#include <stdlib.h>
#include <ctype.h>

#include "trv_types.h"
#include "wld_mem.h"
#include "wld_bitmaps.h"
#include "wld_graphicfile.h"
#include "wld_utils.h"

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: skip_cruft
 *
 * Description:
 *   Skip white space and comments.
 *
 ************************************************************************/

static void skip_cruft(FILE * fp)
{
  int c;

  c = getc(fp);
  for (;;)
    {
      if (c == '#')
        {
          while (c != '\n')
            {
              c = getc(fp);
            }
        }
      else if (c == EOF)
        {
          return;
        }
      else if (!isspace(c))
        {
          ungetc(c, fp);
          return;
        }
      else
        {
          c = getc(fp);
        }
    }
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_LoadPPM
 *
 * Description:
 *
 ************************************************************************/

graphic_file_t *wld_LoadPPM(FILE * fp, char *filename)
{
  int width, height;
  int unknown;
  graphic_file_t *gfile;

  skip_cruft(fp);
  if (getc(fp) != 'P' || getc(fp) != '6')
    {
      wld_fatal_error("%s is not a ppm file.", filename);
    }

  skip_cruft(fp);
  if (fscanf(fp, "%d", &width) != 1)
    {
      wld_fatal_error("%s: bad ppm file.", filename);
    }

  skip_cruft(fp);
  if (fscanf(fp, "%d", &height) != 1)
    {
      wld_fatal_error("%s: bad ppm file.", filename);
    }

  skip_cruft(fp);
  if (fscanf(fp, "%d\n", &unknown) != 1)
    {
      wld_fatal_error("%s: bad ppm file.", filename);
    }

  gfile = wld_new_graphicfile();
  gfile->type = GFILE_TRUECOLOR;
  gfile->palette = NULL;
  gfile->width = width;
  gfile->height = height;
  gfile->bitmap = (uint8_t *) wld_malloc(height * width * 3);

  if (fread(gfile->bitmap, height * width * 3, 1, fp) != 1)
    {
      wld_fatal_error("%s: incomplete data", filename);
    }

  return gfile;
}
