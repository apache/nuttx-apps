/****************************************************************************
 * apps/examples/screenshot/screenshot_main.c
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

#include <nuttx/config.h>

#include <sys/boardctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>

#include "graphics/tiff.h"

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
  struct nx_callback_s cb = {};
  struct nxgl_size_s size = {CONFIG_SCREENSHOT_WIDTH, CONFIG_SCREENSHOT_HEIGHT};
#ifdef CONFIG_VNCSERVER
  struct boardioc_vncstart_s vnc;
#endif
  FAR uint8_t *strip;
  NXHANDLE server;
  NXWINDOW window;
  char tempf1[64];
  char tempf2[64];
  int row;
  int ret;

  replace_extension(filename, ".tm1", tempf1, sizeof(tempf1));
  replace_extension(filename, ".tm2", tempf2, sizeof(tempf2));

  /* Connect to NX server */

  server = nx_connect();
  if (!server)
    {
      perror("nx_connect");
      return 1;
    }

#ifdef CONFIG_VNCSERVER
   /* Setup the VNC server to support keyboard/mouse inputs */

   vnc.display = 0;
   vnc.handle  = server;

   ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
   if (ret < 0)
     {
       printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
       nx_disconnect(server);
       return 1;
     }
#endif

  /* Wait for "connected" event */

  if (nx_eventhandler(server) < 0)
    {
      perror("nx_eventhandler");
      nx_disconnect(server);
      return 1;
    }

  /* Open invisible dummy window for communication */

  window = nx_openwindow(server, 0, &cb, NULL);
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

  for (row = 0; row < size.h; row++)
  {
    struct nxgl_rect_s rect = {{0, row}, {size.w - 1, row}};
    nx_getrectangle(window, &rect, 0, strip, 0);

    ret = tiff_addstrip(&info, strip);
    if (ret < 0)
      {
        printf("tiff_addstrip() #%d failed: %d\n", row, ret);
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

int main(int argc, FAR char *argv[])
{
  if (argc != 2)
    {
      fprintf(stderr, "Usage: screenshot file.tif\n");
      return 1;
    }

  return save_screenshot(argv[1]);
}
