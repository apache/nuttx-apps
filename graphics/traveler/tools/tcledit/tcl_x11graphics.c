/****************************************************************************
 * apps/graphics/traveler/tools/tcledit/tcl_x11graphics.c
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

/****************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#ifndef NO_XSHM
#  include <sys/shm.h>
#  include <X11/extensions/XShm.h>
#endif

#include "trv_types.h"
#include "wld_debug.h"

#include "wld_mem.h"
#include "wld_debug.h"
#include "wld_bitmaps.h"
#include "wld_plane.h"
#include "wld_utils.h"
#include "tcl_x11graphics.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static GC gc;
#ifndef NO_XSHM
static XShmSegmentInfo xshminfo;
static int xerror;
#endif
static int shmCheckPoint = 0;
static int useShm;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void x11_create_window(tcl_window_t * w);
static void x11_load_palette(tcl_window_t * w);
static bool x11_allocate_colors(tcl_window_t * w, Colormap colormap);
static void x11_map_sharedmemory(tcl_window_t * w, int depth);
static void x11_unmap_sharedmemory(tcl_window_t * w);
static void x11_unmap_all_sharedmemory(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: x11_create_window
 * Description:
 ***************************************************************************/

static void x11_create_window(tcl_window_t * w)
{
  XGCValues gcValues;
  char *argv[2] = { "tcledit", NULL };
  char *iconName = "tcledit";
  XTextProperty wNameProp, iNameProp;
  XSizeHints sizeHints;

  w->display = XOpenDisplay(NULL);
  if (w->display == NULL)
    wld_fatal_error("Unable to open display.\n");

  w->screen = DefaultScreen(w->display);

  w->win = XCreateSimpleWindow(w->display, DefaultRootWindow(w->display),
                               0, 0, w->width, w->height, 2,
                               BlackPixel(w->display, w->screen),
                               BlackPixel(w->display, w->screen));

  XStringListToTextProperty(&w->title, 1, &wNameProp);
  XStringListToTextProperty(&iconName, 1, &iNameProp);

  sizeHints.flags = PSize | PMinSize | PMaxSize;
  sizeHints.width = sizeHints.min_width = sizeHints.max_width = w->width;
  sizeHints.height = sizeHints.min_height = sizeHints.max_height = w->height;

  XSetWMProperties(w->display, w->win, &wNameProp, &iNameProp, argv, 1,
                   &sizeHints, NULL, NULL);

  XMapWindow(w->display, w->win);
  XSelectInput(w->display, w->win,
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | KeyPressMask | ExposureMask);
  gcValues.graphics_exposures = false;
  gc = XCreateGC(w->display, w->win, GCGraphicsExposures, &gcValues);
}

/****************************************************************************
 * Name: x11_load_palette
 * Description:
 ***************************************************************************/

static void x11_load_palette(tcl_window_t * w)
{
  Colormap cMap;

  cMap = DefaultColormap(w->display, w->screen);
  printf("Using Default Colormap: ");

  if (!x11_allocate_colors(w, cMap))
    {
      printf("failed\nCreating Colormap: ");

      XFreeColors(w->display, cMap, w->colorLookup, w->ncolors, 0);

      cMap = XCreateColormap(w->display, w->win,
                             DefaultVisual(w->display, w->screen), AllocNone);

      if (!x11_allocate_colors(w, cMap))
        {
          printf("failed\n");
          wld_fatal_error("Unable to allocate enough color cells.");
        }
      XSetWindowColormap(w->display, w->win, cMap);
    }

  printf("%d colors allocated\n", w->ncolors);
}

/****************************************************************************
 * Name: x11_allocate_colors
 * Description:
 ***************************************************************************/

static bool x11_allocate_colors(tcl_window_t * w, Colormap colormap)
{
  int i;

  /* Allocate each color */

  w->ncolors = 0;

  for (i = 0; i < PALETTE_SIZE; i++)
    {
      XColor color;

      /* The red, green, and blue values are always in the range 0 to 65535
       * inclusive, independent of the number of bits actually used in the
       * display hardware. Black is represented by (0,0,0), and white is
       * represented by (65535,65535,65535).
       */

      color.red = ((unsigned short)w->palette[i].red << 8);
      color.green = ((unsigned short)w->palette[i].green << 8);
      color.blue = ((unsigned short)w->palette[i].blue << 8);
      color.flags = DoRed | DoGreen | DoBlue;

      /* Then allocate a color for this selection */

      if (!XAllocColor(w->display, colormap, &color))
        {
          return false;
        }

      /* Save the RGB to pixel lookup data */

      info("%d.%d {%02x,%02x,%02x}->0x%06lx\n",
            w->plane, i,
            w->palette[i].red, w->palette[i].green, w->palette[i].blue,
            color.pixel);

      w->colorLookup[i] = color.pixel;
      w->ncolors++;
    }
  return true;
}

/****************************************************************************
 * Name: x11_error_handler
 * Description:
 ***************************************************************************/

#ifndef NO_XSHM
static int x11_error_handler(Display * display, XErrorEvent * event)
{
  xerror = 1;

  return 0;
}
#endif

/****************************************************************************
 * Name: x11_trap_errors
 * Description:
 ***************************************************************************/

#ifndef NO_XSHM
static void x11_trap_errors(void)
{
  xerror = 0;
  XSetErrorHandler(x11_error_handler);
}
#endif

/****************************************************************************
 * Name: x11_untrap_errors
 * Description:
 ***************************************************************************/

#ifndef NO_XSHM
static int x11_untrap_errors(Display * display)
{
  XSync(display, 0);
  XSetErrorHandler(NULL);
  return xerror;
}
#endif

/****************************************************************************
 * Name: x11_map_sharedmemory
 * Description:
 ***************************************************************************/

static void x11_map_sharedmemory(tcl_window_t * w, int depth)
{
#ifndef NO_XSHM
  Status result;
#endif

  shmCheckPoint = 0;
  x11_unmap_sharedmemory(w);

  if (!shmCheckPoint)
    atexit(x11_unmap_all_sharedmemory);
  shmCheckPoint = 1;

  useShm = 0;

#ifndef NO_XSHM
  if (XShmQueryExtension(w->display))
    {
      useShm = 1;
      printf("Using shared memory.\n");

      x11_trap_errors();
      w->image = XShmCreateImage(w->display,
                                 DefaultVisual(w->display, w->screen),
                                 depth, ZPixmap, NULL, &xshminfo,
                                 w->width, w->height);

      if (x11_untrap_errors(w->display))
        {
          x11_unmap_sharedmemory(w);
          goto shmerror;
        }

      if (!w->image)
        {
          wld_fatal_error("Unable to create image.");
        }
      shmCheckPoint++;

      xshminfo.shmid = shmget(IPC_PRIVATE,
                              w->image->bytes_per_line * w->image->height,
                              IPC_CREAT | 0777);
      if (xshminfo.shmid < 0)
        {
          x11_unmap_sharedmemory(w);
          goto shmerror;
        }
      shmCheckPoint++;

      w->image->data = (char *)shmat(xshminfo.shmid, 0, 0);
      if (image->data == ((char *)-1))
        {
          x11_unmap_sharedmemory(w);
          goto shmerror;
        }
      shmCheckPoint++;

      xshminfo.shmaddr = w->image->data;
      xshminfo.readOnly = false;

      x11_trap_errors();
      result = XShmAttach(w->display, &xshminfo);
      if (x11_untrap_errors(w->display) || !result)
        {
          x11_unmap_sharedmemory(w);
          goto shmerror;
        }

      shmCheckPoint++;
    }
  else
#endif

  if (!useShm)
    {
#ifndef NO_XSHM
    shmerror:
#endif
      useShm = 0;

      w->frameBuffer = (dev_pixel_t *)
        wld_malloc(w->width * w->height * sizeof(dev_pixel_t));

      w->image = XCreateImage(w->display,
                              DefaultVisual(w->display, w->screen),
                              depth,
                              ZPixmap,
                              0,
                              (char *)w->frameBuffer,
                              w->width, w->height, 8, 0);

      if (w->image == NULL)
        {
          wld_fatal_error("Unable to create image.");
        }

      shmCheckPoint++;
    }
}

/****************************************************************************
 * Name: x11_unmap_sharedmemory
 * Description:
 ***************************************************************************/

static void x11_unmap_sharedmemory(tcl_window_t * w)
{
#ifndef NO_XSHM
  if (shmCheckPoint > 4)
    {
      XShmDetach(w->display, &xshminfo);
    }

  if (shmCheckPoint > 3)
    {
      shmdt(xshminfo.shmaddr);
    }

  if (shmCheckPoint > 2)
    {
      shmctl(xshminfo.shmid, IPC_RMID, 0);
    }
#endif

  if (shmCheckPoint > 1)
    {
      XDestroyImage(w->image);
      if (!useShm)
        {
          wld_free(w->frameBuffer);
        }
    }

  if (shmCheckPoint > 0)
    {
      shmCheckPoint = 1;
    }
}

/****************************************************************************
 * Name: x11_unmap_sharedmemory
 * Description:
 ***************************************************************************/

static void x11_unmap_all_sharedmemory(void)
{
  int i;

  for (i = 0; i < NUM_PLANES; i++)
    {
      x11_unmap_sharedmemory(&g_windows[i]);
    }
}

/****************************************************************************
 * Name: x11_UpdateScreen
 * Description:
 ***************************************************************************/

void x11_UpdateScreen(tcl_window_t * w)
{
#ifndef NO_XSHM
  if (useShm)
    {
      XShmPutImage(w->display, w->win, gc, w->image, 0, 0, 0, 0,
                   w->width, w->height, 0);
    }
  else
#endif
    {
      XPutImage(w->display, w->win, gc, w->image, 0, 0, 0, 0,
                w->width, w->height);
    }
  XSync(w->display, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: x11_initialize_graphics
 * Description:
 ***************************************************************************/

void x11_initialize_graphics(tcl_window_t * w)
{
  XWindowAttributes windowAttributes;

  /* Create the X11 window */

  x11_create_window(w);

  /* Determine the supported pixel depth of the current window */

  XGetWindowAttributes(w->display, DefaultRootWindow(w->display),
                       &windowAttributes);

  printf("Pixel depth is %d bits\n", windowAttributes.depth);
  if (windowAttributes.depth != 24)
    {
      wld_fatal_error("Unsupported pixel depth: %d", windowAttributes.depth);
    }

  x11_load_palette(w);
  x11_map_sharedmemory(w, windowAttributes.depth);
}

/****************************************************************************
 * Name: x11_end_graphics
 * Description:
 ***************************************************************************/

void x11_end_graphics(tcl_window_t * w)
{
  x11_unmap_all_sharedmemory();
  XCloseDisplay(w->display);
}

/*************************************************************************
 * Function: x11_update_screen
 * Description:
 ************************************************************************/

void x11_update_screen(tcl_window_t *w)
{
#ifndef NO_XSHM
  if (useShm)
    {
      XShmPutImage(w->display, w->win, gc, w->image, 0, 0, 0, 0,
                   w->width, w->height, 0);
    }
  else
#endif
    {
      XPutImage(w->display, w->win, gc, w->image, 0, 0, 0, 0,
                w->width, w->height);
    }

  XSync(w->display, 0);
}

