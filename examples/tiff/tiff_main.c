/****************************************************************************
 * apps/graphics/tiff/tiff_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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

#include "graphics/tiff.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* This is a simple unit test for the TIFF creation library at apps/graphic/tiff.
 * It is configured to work in the Linux user-mode simulation and has not been
 * tested in any other environment.
 *
 * Other configuration options:
 *
 *  CONFIG_EXAMPLES_TIFF_OUTFILE - Name of the resulting TIFF file
 *  CONFIG_EXAMPLES_TIFF_TMPFILE1/2 - Names of two temporaries files that
 *    will be used in the file creation.
 */

#ifndef CONFIG_EXAMPLES_TIFF_OUTFILE
#  define CONFIG_EXAMPLES_TIFF_OUTFILE "/tmp/result.tif"
#endif

#ifndef CONFIG_EXAMPLES_TIFF_TMPFILE1
#  define CONFIG_EXAMPLES_TIFF_TMPFILE1 "/tmp/tmpfile1.dat"
#endif

#ifndef CONFIG_EXAMPLES_TIFF_TMPFILE2
#  define CONFIG_EXAMPLES_TIFF_TMPFILE2 "/tmp/tmpfile2.dat"
#endif

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
 * Name: tiff_main
 *
 * Description:
 *   TIFF unit test.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct tiff_info_s info;
  uint8_t strip[3*256];
  uint8_t *ptr;
  int green;
  int blue;
  int ret;

  /* Configure the interface structure */

  memset(&info, 0, sizeof(struct tiff_info_s));
  info.outfile   = CONFIG_EXAMPLES_TIFF_OUTFILE;
  info.tmpfile1  = CONFIG_EXAMPLES_TIFF_TMPFILE1;
  info.tmpfile2  = CONFIG_EXAMPLES_TIFF_TMPFILE2;
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
      ptr = strip;
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
      printf("tiff_finalize() failed: %d\n", ret);
      exit(1);
    }
  return 0;
}
