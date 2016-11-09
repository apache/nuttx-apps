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

#include <stdio.h>
#include <stdlib.h>
#include <debug.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#ifndef NO_XSHM
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#include "trv_types.h"
#include "trv_graphics.h"
#include "debug.h"
#include "wld_mem.h"
#include "wld_bitmaps.h"
#include "wld_plane.h"
#include "wld_utils.h"
#include "x11edit.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Definitions
 ***************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void x11_CreateWindow(tcl_window_t *w);
static void x11_LoadPalette(tcl_window_t *w);
static boolean x11_AllocateColors(tcl_window_t *w, Colormap colormap);
static void x11_MapSharedMemory(tcl_window_t *w, int depth);
static void x11_UnMapSharedMemory(tcl_window_t *w);
static void x11_UnMapAllSharedMemory(void);

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static GC gc;
#ifndef NO_XSHM
static XShmSegmentInfo xshminfo;
static int xError;
#endif
static int shmCheckPoint = 0;
static int useShm;

/****************************************************************************
 * Name: x11_InitGraphics
 * Description:
 ***************************************************************************/

void x11_InitGraphics(tcl_window_t *w)
{
  XWindowAttributes windowAttributes;

  /* Create the X11 window */

  x11_CreateWindow(w);

  /* Determine the supported pixel depth of the current window */

  XGetWindowAttributes(w->display, DefaultRootWindow(w->display),
                       &windowAttributes);

  printf("Pixel depth is %d bits\n", windowAttributes.depth);
  if (windowAttributes.depth != 24)
    {
      wld_fatal_error("Unsupported pixel depth: %d", windowAttributes.depth);
    }

  x11_LoadPalette(w);
  x11_MapSharedMemory(w, windowAttributes.depth);
}

/****************************************************************************
 * Name: x11_EndGraphics
 * Description:
 ***************************************************************************/

void x11_EndGraphics(tcl_window_t *w)
{
  x11_UnMapAllSharedMemory();
  XCloseDisplay(w->display);
}

/****************************************************************************
 * Name: x11_CreateWindow
 * Description:
 ***************************************************************************/

static void x11_CreateWindow(tcl_window_t *w)
{
  XGCValues gcValues;
  char *argv[2] = { "xast", NULL };
  char *iconName = "xast";
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
  sizeHints.width= sizeHints.min_width = sizeHints.max_width = w->width;
  sizeHints.height= sizeHints.min_height = sizeHints.max_height = w->height;

  XSetWMProperties(w->display, w->win, &wNameProp, &iNameProp, argv, 1,
                   &sizeHints, NULL, NULL);

  XMapWindow(w->display, w->win);
  XSelectInput(w->display, w->win,
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | KeyPressMask | ExposureMask);
  gcValues.graphics_exposures = FALSE;
  gc = XCreateGC(w->display, w->win, GCGraphicsExposures, &gcValues);
}

/****************************************************************************
 * Name: x11_LoadPalette
 * Description:
 ***************************************************************************/

static void x11_LoadPalette(tcl_window_t *w)
{
  Colormap cMap;

  cMap = DefaultColormap(w->display, w->screen);
  printf("Using Default Colormap: ");

  if (!astAllocateColors(w, cMap))
    {
      printf("failed\nCreating Colormap: ");

      XFreeColors(w->display, cMap, w->colorLookup, w->ncolors, 0);

      cMap = XCreateColormap(w->display, w->win,
                             DefaultVisual(w->display, w->screen),
                             AllocNone);

      if (!astAllocateColors(w, cMap))
        {
          printf("failed\n");
          wld_fatal_error("Unable to allocate enough color cells.");
        }
      XSetWindowColormap(w->display, w->win, cMap);
    }

  printf("%d colors allocated\n", w->ncolors);
}

/****************************************************************************
 * Name: x11_AllocateColors
 * Description:
 ***************************************************************************/

static boolean x11_AllocateColors(tcl_window_t *w, Colormap colormap)
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

      color.red   = ((unsigned short)w->palette[i].red   << 8);
      color.green = ((unsigned short)w->palette[i].green << 8);
      color.blue  = ((unsigned short)w->palette[i].blue  << 8);
      color.flags = DoRed | DoGreen | DoBlue;

      /* Then allocate a color for this selection */

      if (!XAllocColor(w->display, colormap, &color))
        {
          return FALSE;
        }

      /* Save the RGB to pixel lookup data */

      ginfo("%d.%d {%02x,%02x,%02x}->0x%06lx\n",
           w->plane, i,
           w->palette[i].red, w->palette[i].green, w->palette[i].blue,
           color.pixel);

      w->colorLookup[i] = color.pixel;
      w->ncolors++;
    }
  return TRUE;
}

/****************************************************************************
 * Name: errorHandler
 * Description:
 ***************************************************************************/

#ifndef NO_XSHM
static int errorHandler(Display *display, XErrorEvent *event)
{
  xError = 1;

  return 0;
}
#endif

/****************************************************************************
 * Name: trapErrors
 * Description:
 ***************************************************************************/

#ifndef NO_XSHM
static void trapErrors(void)
{
  xError = 0;
  XSetErrorHandler(errorHandler);
}
#endif

/****************************************************************************
 * Name: untrapErrors
 * Description:
 ***************************************************************************/

#ifndef NO_XSHM
static int untrapErrors(Display *display)
{
  XSync(display,0);
  XSetErrorHandler(NULL);
  return xError;
}
#endif

/****************************************************************************
 * Name: x11_MapSharedMemory
 * Description:
 ***************************************************************************/

static void x11_MapSharedMemory(tcl_window_t *w, int depth)
{
#ifndef NO_XSHM
  Status result;
#endif

  shmCheckPoint = 0;
  x11_UnMapSharedMemory(w);

  if (!shmCheckPoint) atexit(astUnMapAllSharedMemory);
  shmCheckPoint = 1;

  useShm = 0;

#ifndef NO_XSHM
  if (XShmQueryExtension(w->display))
    {
      useShm = 1;
      printf("Using shared memory.\n");

      trapErrors();
      w->image = XShmCreateImage(w->display,
                                 DefaultVisual(w->display, w->screen), 
                                 depth, ZPixmap, NULL, &xshminfo,
                                 w->width, w->height);

      if (untrapErrors(w->display))
        {
          x11_UnMapSharedMemory(w);
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
          x11_UnMapSharedMemory(w);
          goto shmerror;
        }
      shmCheckPoint++;
      
      w->image->data = (char *) shmat(xshminfo.shmid, 0, 0);
      if (image->data == ((char *) -1))
        {
          x11_UnMapSharedMemory(w);
          goto shmerror;
        }
      shmCheckPoint++;

      xshminfo.shmaddr = w->image->data;
      xshminfo.readOnly = FALSE;

      trapErrors();
      result = XShmAttach(w->display, &xshminfo);
      if (untrapErrors(w->display) || !result)
        {
          x11_UnMapSharedMemory(w);
          goto shmerror;
        }

      shmCheckPoint++;
    } else
#endif

      if (!useShm)
        {
#ifndef NO_XSHM
        shmerror:
#endif
          useShm = 0;

          w->frameBuffer = (dev_pixel_t*)
            x11_Malloc(w->width * w->height * sizeof(dev_pixel_t));

          w->image = XCreateImage(w->display,
                                  DefaultVisual(w->display, w->screen),
                                  depth,
                                  ZPixmap,
                                  0,
                                  (char*)w->frameBuffer,
                                  w->width, w->height,
                                  8, 0);

          if (w->image == NULL)
            {
              wld_fatal_error("Unable to create image.");
            }
          shmCheckPoint++;
        }
}

/****************************************************************************
 * Name: x11_UnMapSharedMemory
 * Description:
 ***************************************************************************/

static void x11_UnMapSharedMemory(tcl_window_t *w)
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
          x11_Free(w->frameBuffer);
        }
    }

  if (shmCheckPoint > 0)
    {
      shmCheckPoint = 1;
    }
}

/****************************************************************************
 * Name: x11_UnMapSharedMemory
 * Description:
 ***************************************************************************/

static void x11_UnMapAllSharedMemory(void)
{
  int i;

  for (i = 0; i < NUM_PLANES; i++)
    {
      x11_UnMapSharedMemory(&windows[i]);
    }
}

/****************************************************************************
 * Name: x11_UpdateScreen
 * Description:
 ***************************************************************************/

void x11_UpdateScreen(tcl_window_t *w)
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
