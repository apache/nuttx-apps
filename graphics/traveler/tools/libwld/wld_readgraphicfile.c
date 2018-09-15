/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_readgraphicfile.c
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

#include <string.h>
#include <ctype.h>

#include "trv_types.h"
#include "wld_mem.h"
#include "wld_bitmaps.h"
#include "wld_graphicfile.h"
#include "wld_pcx.h"
#include "wld_utils.h"

graphic_file_t *wld_LoadPPM(FILE * fp, char *filename);
graphic_file_t *wld_LoadGIF(FILE * fp, char *filename);

/*************************************************************************
 * Private Data
 *************************************************************************/

static const char g_pcx_extension[] = ".PCX";

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_check_format
 * Description:
 ************************************************************************/

static graphic_file_format_t wld_check_format(FILE * fp, char *filename)
{
  char magic[MAGIC_LENGTH];

  if (fread(magic, 1, MAGIC_LENGTH, fp) != MAGIC_LENGTH)
    {
      wld_fatal_error("ERROR: Reading texture %s.", filename);
    }

  if (strncmp(magic, PPM_MAGIC, sizeof(PPM_MAGIC) - 1) == 0)
    {
      return FORMAT_PPM;
    }
  else if (strncmp(magic, GIF_MAGIC, sizeof(GIF_MAGIC) - 1) == 0)
    {
      return FORMAT_GIT87;
    }
  else if (strncmp(magic, GIF89_MAGIC, sizeof(GIF89_MAGIC) - 1) == 0)
    {
      return FORMAT_GIT89;
    }
  else
    {
      char *ptr1;
      char *ptr2;

      /* MS-DOS PCX files will have no magic number, we'll have to make an
       * educated guess based on the file extension.
       */

      /* Search for the PCX extension */

      for (ptr1 = filename; ((*ptr1) && (*ptr1 != '.')); ptr1++);

      /* Check if the extension matches */

      for (ptr2 = (char *)g_pcx_extension; ((*ptr1) && (*ptr2)); ptr1++, ptr2++)
        {
          if (toupper((int)*ptr1) != *ptr2)
            {
              return FORMAT_UNKNOWN;
            }
        }

      /* If it is an exact match, both pointers should refer to the NULL
       * terminator.
       */

      if (!(*ptr1) && !(*ptr2))
        {
          return FORMAT_PCX;
        }
    }

  return FORMAT_UNKNOWN;
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_readgraphic_file
 *
 * Description:
 *
 ************************************************************************/

graphic_file_t *wld_readgraphic_file(char *filename)
{
  FILE *fp;
  graphic_file_format_t format;
  graphic_file_t *gfile = NULL;

  if ((fp = fopen(filename, "rb")) == NULL)
    {
      wld_fatal_error("Could not open texture %s", filename);
    }

  format = wld_check_format(fp, filename);
  fseek(fp, 0, SEEK_SET);

  switch (format)
    {
    case FORMAT_GIT89:
    case FORMAT_GIT87:
      gfile = wld_LoadGIF(fp, filename);
      break;

    case FORMAT_PPM:
      gfile = wld_LoadPPM(fp, filename);
      break;

    case FORMAT_PCX:
      gfile = wld_loadpcx(fp, filename);
      break;

    case FORMAT_UNKNOWN:
      wld_fatal_error("Unknown graphic file format.\n");
      break;

    default:
      wld_fatal_error("The graphic file reading code is really broken.\n");
      break;
    }

  if (gfile == NULL)
    {
      wld_fatal_error("ERROR: Reading texture %s\n");
    }

  fclose(fp);
  return gfile;
}
