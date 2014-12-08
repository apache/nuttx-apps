/****************************************************************************
 * apps/graphics/traveler/src/trv_graphics.c
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
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_main.h"
#include "trv_mem.h"
#include "trv_color.h"
#include "trv_raycntl.h"
#include "trv_debug.h"
#include "trv_graphics.h"

#include <string.h>
#ifdef CONFIG_NX_MULTIUSER
#  include <semaphore.h>
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_NX
FAR const struct nx_callback_s *g_trv_nxcallback;
sem_t g_trv_nxevent = SEM_INITIZIALIZER(0);
bool g_trv_nxresolution = false;
#ifdef CONFIG_NX_MULTIUSER
bool g_trv_nxrconnected = false;
#endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_get_fbdev
 *
 * Description:
 *   Get the system framebuffer device
 *
 ****************************************************************************/

#ifndef CONFIG_NX_MULTIUSER
static FAR struct fb_vtable_s *trv_get_fbdev(void)
{
  FAR struct fb_vtable_s *fbdev;
  int ret;

  /* Initialize the frame buffer device */

  ret = up_fbinitialize();
  if (ret < 0)
    {
      trv_abort("ERROR: up_fbinitialize failed: %d\n", -ret);
    }

  /* Set up to use video plane 0.  There is no support for anything but
   * video plane 0.
   */

  fbdev = up_fbgetvplane(0);
  if (!fbdev)
    {
      trv_abort("ERROR: up_fbgetvplane(0) failed\n");
    }

  return fbdev;
}
#endif

/****************************************************************************
 * Name: trv_fb_initialize
 *
 * Description:
 *   Get the system framebuffer device
 *
 ****************************************************************************/

#if !defined(CONFIG_NX_MULTIUSER) && !defined(CONFIG_NX)
static void trv_fb_initialize(FAR struct trv_graphics_info_s *ginfo)
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR struct fb_vtable_s *fbdev;
  int ret;

  /* Get the framebuffer device */

  fbdev = trv_get_fbdev();

  /* Get information about video plane 0 */

  ret = fbdev->getvideoinfo(fbdev, &vinfo);
  if (ret < 0)
    {
      trv_abort("ERROR: getvideoinfo() failed\n");
    }

  ginfo->xres = vinfo.xres;
  ginfo->yres = vinfo.yres;

  ret = fbdev->getplaneinfo(fbdev, 0, &pinfo);
  if (ret < 0)
    {
      trv_abort("ERROR: getplaneinfo() failed\n");
    }

  ginfo->stride   = pinfo.stride;
  ginfo->hwbuffer = pinfo.fbmem;

  if (vinfo.fmt != TRV_COLOR_FMT || pinfo.bpp != TRV_BPP)
    {
      trv_abort("ERROR: Bad color format(%d)/bpp(%d)\n", vinfo.fmt, pinfo.bpp);
    }
}
#endif

/****************************************************************************
 * Name: trv_use_bgwindow
 *
 * Description:
 *   Get the NX background window
 *
 ****************************************************************************/

#ifdef CONFIG_NX
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
 * Name: trv_nxsu_initialize
 ****************************************************************************/

#if defined(CONFIG_NX) && !defined(CONFIG_NX_MULTIUSER)
static inline int trv_nxsu_initialize(FAR struct trv_graphics_info_s *ginfo)
{
  FAR struct fb_vtable_s *fbdev;
  int ret;

  /* Get the framebuffer device */

  fbdev = trv_get_fbdev();

  /* Open NX */

  ginfo->hnx = nx_open(fbdev);
  if (!ginfo->hnx)
    {
      trv_abort("trv_nxsu_initialize: nx_open failed: %d\n", errno);
    }

  /* And use the background window */

  trv_use_bgwindow(ginfo);
}
#endif

/****************************************************************************
 * Name: trv_servertask
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
int trv_servertask(int argc, char *argv[])
{
  FAR struct fb_vtable_s *fbdev;
  int ret;

  /* Get the framebuffer device */

  fbdev = trv_get_fbdev();

  /* Then start the server */

  ret = nx_run(dev);
  trv_abort("nx_run returned: %d\n", errno);
}
#endif

/****************************************************************************
 * Name: trv_nxmu_initialize
 ****************************************************************************/

#ifdef CONFIG_NX_MULTIUSER
static inline int trv_nxmu_initialize(FAR struct trv_graphics_info_s *ginfo)
{
  struct sched_param param;
  pthread_t thread;
  pid_t servrid;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NX_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxeg_initialize: sched_setparam failed: %d\n" , ret);
      g_exitcode = NXEXIT_SCHEDSETPARAM;
      return ERROR;
    }

  /* Start the server task */

  printf("nxeg_initialize: Starting trv_servertask task\n");
  servrid = task_create("NX Server", CONFIG_EXAMPLES_NX_SERVERPRIO,
                        CONFIG_EXAMPLES_NX_STACKSIZE, trv_servertask, NULL);
  if (servrid < 0)
    {
      printf("nxeg_initialize: Failed to create trv_servertask task: %d\n", errno);
      g_exitcode = NXEXIT_TASKCREATE;
      return ERROR;
    }

  /* Wait a bit to let the server get started */

  sleep(1);

  /* Connect to the server */

  ginfo->hnx = nx_connect();
  if (ginfo->hnx)
    {
       pthread_attr_t attr;

       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       (void)pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NX_LISTENERPRIO;
       (void)pthread_attr_setschedparam(&attr, &param);
       (void)pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NX_STACKSIZE);

       ret = pthread_create(&thread, &attr, trv_nxlistener, NULL);
       if (ret != 0)
         {
            printf("nxeg_initialize: pthread_create failed: %d\n", ret);
            g_exitcode = NXEXIT_PTHREADCREATE;
            return ERROR;
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
      printf("nxeg_initialize: nx_connect failed: %d\n", errno);
      g_exitcode = NXEXIT_NXCONNECT;
      return ERROR;
    }

  /* And use the background window */

  trv_use_bgwindow(ginfo);
}
#endif

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

#ifdef CONFIG_NX
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

#if !defined(CONFIG_NX)
  trv_fb_initialize(ginfo);
#elif defined(CONFIG_NX_MULTIUSER)
  trv_nxmu_initialize(ginfo);
#else
  trv_nxsu_initialize(ginfo);
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

#ifdef CONFIG_NX
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

#ifdef CONFIG_NX
  if (ginfo->hwbuffer)
    {
      trv_free(ginfo->hwbuffer);
      ginfo->hwbuffer = NULL;
    }

  /* Close/disconnect NX */
#warning "Missing Logic"
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
#ifdef CONFIG_NX
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

#ifdef CONFIG_NX
  destrow = 0;
#endif

  for (srcrow = 0; srcrow < TRV_SCREEN_HEIGHT; srcrow++)
    {
      /* Transfer the row to the device row/buffer */

      trv_row_update(ginfo, (FAR const trv_pixel_t *)src,
                     (FAR dev_pixel_t *)dest);

#ifdef CONFIG_NX
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
#ifdef CONFIG_NX
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

