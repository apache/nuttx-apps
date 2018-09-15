/****************************************************************************
 * apps/graphics/traveler/src/trv_graphics.c
 *
 *   Copyright (C) 2014, 2016-2017 Gregory Nutt. All rights reserved.
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
 ****************************************************************************/

#include "trv_types.h"
#include "trv_main.h"
#include "trv_mem.h"
#include "trv_color.h"
#include "trv_raycntl.h"
#include "trv_debug.h"
#include "trv_graphics.h"

#include <sys/boardctl.h>
#ifdef CONFIG_GRAPHICS_TRAVELER_FB
#  include <sys/ioctl.h>
#  include <sys/mman.h>
#  include <fcntl.h>
#  include <errno.h>
#endif
#include <string.h>
#include <semaphore.h>

#ifdef CONFIG_GRAPHICS_TRAVELER_FB
#  include <nuttx/video/fb.h>
#endif
#ifdef CONFIG_VNCSERVER
#  include <nuttx/video/vnc.h>
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
FAR const struct nx_callback_s *g_trv_nxcallback;
sem_t g_trv_nxevent = SEM_INITIZIALIZER(0);
bool g_trv_nxresolution = false;
bool g_trv_nxrconnected = false;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_use_bgwindow
 *
 * Description:
 *   Get the NX background window
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
static void trv_use_bgwindow(FAR struct trv_graphics_info_s *ginfo)
{
  /* Get the background window */

  ret = nx_requestbkgd(g_hnx, &g_trv_nxcallback, ginfo);
  if (ret < 0)
    {
      trv_abort("nx_requestbkgd failed: %d\n", errno);
    }

  /* Wait until we have the screen resolution.  We'll have this immediately
   * unless we are dealing with the NX server.
   */

  while (!g_trv_nxresolution)
    {
      (void)sem_wait(&g_trv_nxevent);
    }
}
#endif

/****************************************************************************
 * Name: trv_fb_initialize
 *
 * Description:
 *   Get the system framebuffer device (only used on simulator)
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_FB
static void trv_fb_initialize(FAR struct trv_graphics_info_s *ginfo)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  int ret;

  /* Open the framebuffer driver */

  ginfo->fb = open(CONFIG_GRAPHICS_TRAVELER_FBDEV, O_RDWR);
  if (ginfo->fb < 0)
    {
      int errcode = errno;
      trv_abort("ERROR: Failed to open %s: %d\n",
                CONFIG_GRAPHICS_TRAVELER_FBDEV, errcode);
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(ginfo->fb, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&vinfo));
  if (ret < 0)
    {
      int errcode = errno;
      close(ginfo->fb);

      trv_abort("ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n",
                errcode);
    }

  if (vinfo.fmt != TRV_COLOR_FMT)
    {
      close(ginfo->fb);
      trv_abort("ERROR:  Bad color format %u; supported format %u\n",
                vinfo.fmt, TRV_COLOR_FMT);
    }

  ginfo->xres = vinfo.xres;
  ginfo->yres = vinfo.yres;

  ret = ioctl(ginfo->fb, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      close(ginfo->fb);

      trv_abort("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
                errcode);
    }

  if (pinfo.bpp != TRV_BPP)
    {
      close(ginfo->fb);
      trv_abort("ERROR:  Bad pixel depth %u BPP; supported %u BPP\n",
                pinfo.bpp, TRV_BPP);
    }

  ginfo->stride = pinfo.stride;

  /* mmap() the framebuffer.
   *
   * NOTE: In the FLAT build the frame buffer address returned by the
   * FBIOGET_PLANEINFO IOCTL command will be the same as the framebuffer
   * address.  mmap(), however, is the preferred way to get the framebuffer
   * address because in the KERNEL build, it will perform the necessary
   * address mapping to make the memory accessible to the application.
   */

  ginfo->hwbuffer = mmap(NULL, pinfo.fblen, PROT_READ|PROT_WRITE,
                     MAP_SHARED|MAP_FILE, ginfo->fb, 0);
  if (ginfo->hwbuffer == MAP_FAILED)
    {
      int errcode = errno;
      close(ginfo->fb);

      trv_abort("ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
                errcode);
    }
}
#endif

/****************************************************************************
 * Name: trv_nx_initialize
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
static inline int trv_nx_initialize(FAR struct trv_graphics_info_s *ginfo)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_GRAPHICS_TRAVELER_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      trv_abort("trv_nx_initialize: sched_setparam failed: %d\n" , ret);
    }

  /* Start the NX server kernel thread */

  printf("trv_nx_initialize: Starting NX server\n");
  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      trv_abort("trv_nx_initialize: Failed to start the NX server: %d\n",
                errno);
    }

  /* Wait a bit to let the server get started */

  sleep(1);

  /* Connect to the server */

  ginfo->hnx = nx_connect();
  if (ginfo->hnx)
    {
      pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

      ret = vnc_default_fbinitialize(0, ginfo->hnx);
      if (ret < 0)
        {
          trv_abort("vnc_default_fbinitialize failed: %d\n", ret);
        }
#endif

      /* Start a separate thread to listen for server events.  This is probably
       * the least efficient way to do this, but it makes this example flow more
       * smoothly.
       */

      (void)pthread_attr_init(&attr);
      param.sched_priority = CONFIG_GRAPHICS_TRAVELER_LISTENERPRIO;
      (void)pthread_attr_setschedparam(&attr, &param);
      (void)pthread_attr_setstacksize(&attr, CONFIG_GRAPHICS_TRAVELER_STACKSIZE);

      ret = pthread_create(&thread, &attr, trv_nxlistener, NULL);
      if (ret != 0)
        {
           trv_abort("nxeg_initialize: pthread_create failed: %d\n", ret);
        }

      /* Don't return until we are connected to the server */

      while (!g_trv_nxrconnected)
        {
          /* Wait for the listener thread to wake us up when we really
           * are connected.
           */

          (void)sem_wait(&g_trv_nxevent);
        }
    }
  else
    {
      trv_abort("nxeg_initialize: nx_connect failed: %d\n", errno);
    }

  /* And use the background window */

  trv_use_bgwindow(ginfo);
}
#endif /* CONFIG_GRAPHICS_TRAVELER_NX */

/****************************************************************************
 * Name: trv_row_update
 *
 * Description:
 *   Expand one one either directly into the frame buffer or else into an
 *   intermediate line buffer.
 *
 ****************************************************************************/

void trv_row_update(struct trv_graphics_info_s *ginfo,
                    FAR const trv_pixel_t *src,
                    FAR dev_pixel_t *dest)
{
  dev_pixel_t pixel;
  trv_coord_t srccol;
  int i;

  /* Loop for each column in the src render buffer */

  for (srccol = 0; srccol < TRV_SCREEN_WIDTH; srccol++)
    {
      /* Map the source pixel */

      pixel = ginfo->palette.lut[*src++];

      /* Expand pixels horizontally via pixel replication */

      for (i = 0; i < ginfo->xscale; i++)
        {
          *dest++ = pixel;
        }
    }
}

/****************************************************************************
 * Name: trv_row_transfer
 *
 * Description:
 *   Transfer one line from the line buffer to the NX window.
 *
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
void trv_display_update(struct trv_graphics_info_s *ginfo,
                        FAR dev_pixel_t *dest, trv_coord_t destrow)
{
  /* Transfer the row buffer to the NX window */
#warning Missing logic

}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_graphics_initialize
 *
 * Description:
 ****************************************************************************/

int trv_graphics_initialize(FAR struct trv_graphics_info_s *ginfo)
{
  int width;
  int height;
  int scale;

  /* Initialize the graphics device and get information about the display */

#ifdef CONFIG_GRAPHICS_TRAVELER_FB
  trv_fb_initialize(ginfo);
#else
  trv_nx_initialize(ginfo);
#endif

  /* Check the size of the display */

  width  = ginfo->xres;
  height = ginfo->yres;

  if (width < TRV_SCREEN_WIDTH || height < TRV_SCREEN_HEIGHT)
    {
      trv_abort("ERROR: Display is too small\n");
    }

  /* Check if we need to scale the image */

  scale = 0;

  while (width >= TRV_SCREEN_WIDTH)
    {
      width -= TRV_SCREEN_WIDTH;
      scale++;
    }

  ginfo->xscale   = scale;
  ginfo->xoffset  = (width >> 1);
  ginfo->imgwidth = scale * TRV_SCREEN_WIDTH * sizeof(dev_pixel_t);

  scale = 0;
  while (height >= TRV_SCREEN_HEIGHT)
    {
      height -= TRV_SCREEN_HEIGHT;
      scale++;
    }

  ginfo->yscale  = scale;
  ginfo->yoffset = (height >> 1);

  /* Allocate buffers
   *
   * ginfo->swbuffer - Software renders into this buffer using an 8-bit
   *   encoding and perhaps at a different resolution that the final
   *   image.
   */

   ginfo->swbuffer = (trv_pixel_t*)
     trv_malloc(TRV_SCREEN_WIDTH * TRV_SCREEN_HEIGHT * sizeof(trv_pixel_t));
   if (!ginfo->swbuffer)
     {
       trv_abort("ERROR: Failed to allocate render buffer\n");
     }

  /* Using the framebuffer driver:
   *   ginfo->hwbuffer - This address of the final, expanded frame image.
   *   This address is determined by hardware and is neither allocated
   *   nor freed.
   *
   * Using NX
   *   ginfo->hwbuffer - This address of one line of the final expanded
   *   image that must transferred to the window.
   */

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
   ginfo->hwbuffer = (trv_pixel_t*)trv_malloc(ginfo->imgwidth);
   if (!ginfo->hwbuffer)
     {
       trv_abort("ERROR: Failed to allocate hardware line buffer\n");
     }
#endif

  /* Allocate color mapping information */

  trv_color_allocate(&ginfo->palette);
  trv_vdebug("%d colors allocated\n", ginfo->palette.ncolors);
  return OK;
}

/****************************************************************************
 * Name: trv_graphics_terminate
 *
 * Description:
 ****************************************************************************/

void trv_graphics_terminate(FAR struct trv_graphics_info_s *ginfo)
{
  /* Free palette */

  trv_color_free(&ginfo->palette);

  /* Free image buffers */

  if (ginfo->swbuffer)
    {
      trv_free(ginfo->swbuffer);
      ginfo->swbuffer = NULL;
    }

#if defined(CONFIG_GRAPHICS_TRAVELER_NX)
  if (ginfo->hwbuffer)
    {
      trv_free(ginfo->hwbuffer);
      ginfo->hwbuffer = NULL;
    }

  /* Close/disconnect NX */
#warning "Missing Logic"

#elif defined(CONFIG_GRAPHICS_TRAVELER_FB)
  close(ginfo->fb);
  ginfo->hwbuffer = NULL;
  ginfo->fb       = -1;
#endif
}

/****************************************************************************
 * Name: trv_display_update
 *
 * Description:
 ****************************************************************************/

void trv_display_update(struct trv_graphics_info_s *ginfo)
{
  FAR const uint8_t *src;
  FAR uint8_t *dest;
  trv_coord_t srcrow;
#ifdef CONFIG_GRAPHICS_TRAVELER_NX
  trv_coord_t destrow;
#else
  FAR uint8_t *first;
#endif
  int i;

  /* Get the star tof the first source row */

  src = (FAR const uint8_t *)ginfo->swbuffer;

  /* Get the start of the first destination row */

  dest = (FAR uint8_t *)ginfo->hwbuffer +
         (ginfo->yoffset * ginfo->stride) +
         (ginfo->xoffset * sizeof(dev_pixel_t));

  /* Loop for each row in the src render buffer */

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
  destrow = 0;
#endif

  for (srcrow = 0; srcrow < TRV_SCREEN_HEIGHT; srcrow++)
    {
      /* Transfer the row to the device row/buffer */

      trv_row_update(ginfo, (FAR const trv_pixel_t *)src,
                     (FAR dev_pixel_t *)dest);

#ifdef CONFIG_GRAPHICS_TRAVELER_NX
      /* Transfer the row buffer to the NX window */

      trv_row_transfer(ginfo, dest, destrow);
      destrow++;
#else
      first = dest;
      dest += ginfo->stride;
#endif

      /* Then replicate as many times as is necessary */

      for (i = 1; i < ginfo->yscale; i++)
        {
#ifdef CONFIG_GRAPHICS_TRAVELER_NX
          /* Transfer the row buffer to the NX window */

          trv_row_transfer(ginfo, dest, destrow);
          destrow++;
#else
          /* Point to the next row in the frame buffer */

          memcpy(dest, first, ginfo->imgwidth);
          dest += ginfo->stride;
#endif
        }

      /* Point to the next src row */

      src += TRV_SCREEN_WIDTH;
    }
}

