/****************************************************************************
 * apps/graphics/tiff/tiff_main.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <apps/tiff.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_test
 *
 * Description:
 *   TIFF unit test.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_TIFF_BUILTIN
#  define MAIN_NAME tiff_main
#else
#  define MAIN_NAME user_start
#endif

int MAIN_NAME(int argc, char *argv[])
{
  struct tiff_info_s info;
  uint8_t strip[3*256];
  uint8_t *ptr;
  int green;
  int blue;
  int ret;

  /* Configure the interface structure */

  memset(&info, 0, sizeof(struct tiff_info_s));
  info.outfile   = "result.tif";
  info.tmpfile1  = "tmpfile1.dat";
  info.tmpfile2  = "tmpfile2.dat";
  info.colorfmt  = FB_FMT_RGB24;
  info.rps       = 1;
  info.imgwidth  = 256;
  info.imgheight = 256;
  info.iobuffer  = (uint8_t *)malloc(300);
  info.iosize    = 300;

  /* Initialize the TIFF library */

  ret = tiff_initialize(&info);
  if (ret < 0)
    {
      printf("tiff_initialize() failed: %d\n", ret);
      exit(1);
    }

  /* Add each strip to the TIFF file */

  for (green = 0, ptr = strip; green < 256; green++)
    {
      for (blue = 0; blue < 256; blue++)
        {
          *ptr++ = (green + blue) >> 1;
          *ptr++ = green;
          *ptr++ = blue;          
        }

      ret = tiff_addstrip(&info, strip);
      if (ret < 0)
        {
          printf("tiff_addstrip() #%d failed: %d\n", green, ret);
          exit(1);
        }
    }

  /* Then finalize the TIFF file */

  ret = tiff_finalize(&info);
  if (ret < 0)
    {
      printf("tiff_initialize() failed: %d\n", ret);
      exit(1);
    }
  return 0;
}
