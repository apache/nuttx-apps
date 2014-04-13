/****************************************************************************
 * apps/examples/screenshot/screenshot_main.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Petteri Aimonen <jpa@kapsi.fi>
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
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <apps/tiff.h>

#include <semaphore.h>
#include <nuttx/config.h>
#include <nuttx/nx/nx.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SCREENSHOT_WIDTH
#  define CONFIG_SCREENSHOT_WIDTH 320
#endif

#ifndef CONFIG_SCREENSHOT_HEIGHT
#  define CONFIG_SCREENSHOT_HEIGHT 240
#endif

#ifndef CONFIG_SCREENSHOT_FORMAT
#  define CONFIG_SCREENSHOT_FORMAT FB_FMT_RGB16_565
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

static void replace_extension(FAR const char *filename, FAR const char *newext,
                              FAR char *dest, size_t size)
{
  FAR char *p = strrchr(filename, '.');
  int len = strlen(filename);

  if (p != NULL)
    {
      len = p - filename;
    }

  if (len > size)
    {
      len = size - strlen(newext);
    }

  strncpy(dest, filename, size);
  strncpy(dest + len, newext, size - len);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: save_screenshot
 *
 * Description:
 *   Takes a screenshot and saves it to a tif file.
 *
 ****************************************************************************/

int save_screenshot(FAR const char *filename)
{
  struct tiff_info_s info;
  FAR uint8_t *strip;
  int x;
  int y;
  int ret;
  char tempf1[64];
  char tempf2[64];
  NXHANDLE server;
  NXWINDOW window;
  struct nx_callback_s cb = {};
  struct nxgl_size_s size = {CONFIG_SCREENSHOT_WIDTH, CONFIG_SCREENSHOT_HEIGHT};

  replace_extension(filename, ".tm1", tempf1, sizeof(tempf1));
  replace_extension(filename, ".tm2", tempf2, sizeof(tempf2));

  /* Connect to NX server */

  server = nx_connect();
  if (!server)
  {
    perror("nx_connect");
    return 1;
  }

  /* Wait for "connected" event */

  if (nx_eventhandler(server) < 0)
  {
    perror("nx_eventhandler");
    nx_disconnect(server);
    return 1;
  }

  /* Open invisible dummy window for communication */

  window = nx_openwindow(server, &cb, NULL);
  if (!window)
  {
    perror("nx_openwindow");
    nx_disconnect(server);
    return 1;
  }

  nx_setsize(window, &size);

  /* Configure the TIFF structure */

  memset(&info, 0, sizeof(struct tiff_info_s));
  info.outfile   = filename;
  info.tmpfile1  = tempf1;
  info.tmpfile2  = tempf2;
  info.colorfmt  = CONFIG_SCREENSHOT_FORMAT;
  info.rps       = 1;
  info.imgwidth  = size.w;
  info.imgheight = size.h;
  info.iobuffer  = (uint8_t *)malloc(300);
  info.iosize    = 300;

  /* Initialize the TIFF library */

  ret = tiff_initialize(&info);
  if (ret < 0)
    {
      printf("tiff_initialize() failed: %d\n", ret);
      return 1;
    }

  /* Add each strip to the TIFF file */

  strip = malloc(size.w * 3);

  for (int y = 0; y < size.h; y++)
  {
    struct nxgl_rect_s rect = {{0, y}, {size.w - 1, y}};
    nx_getrectangle(window, &rect, 0, strip, 0);

    ret = tiff_addstrip(&info, strip);
    if (ret < 0)
      {
        printf("tiff_addstrip() #%d failed: %d\n", y, ret);
        break;
      }
  }

  free(strip);

  /* Then finalize the TIFF file */

  ret = tiff_finalize(&info);
  if (ret < 0)
    {
      printf("tiff_finalize() failed: %d\n", ret);
    }

  free(info.iobuffer);
  nx_closewindow(window);
  nx_disconnect(server);

  return 0;
}

/****************************************************************************
 * Name: screenshot_main
 *
 * Description:
 *   Main entry point for the screenshot NSH application.
 *
 ****************************************************************************/

int screenshot_main(int argc, char *argv[])
{
  if (argc != 2)
    {
      fprintf(stderr, "Usage: screenshot file.tif\n");
      return 1;
    }

  return save_screenshot(argv[1]);
}
