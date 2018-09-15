/****************************************************************************
 * examples/nximage/nximage_main.c
 *
 *   Copyright (C) 2011, 2015-2017 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/boardctl.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#ifdef CONFIG_VNCSERVER
#  include <nuttx/video/vnc.h>
#endif

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nximage.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct nximage_data_s g_nximage =
{
  NULL,               /* hnx */
  NULL,               /* hbkgd */
  false,              /* connected */
  0,                  /* xres */
  0,                  /* yres */
  false,              /* havpos */
  SEM_INITIALIZER(0), /* eventsem */
  NXEXIT_SUCCESS      /* exit code */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nximage_initialize
 *
 * Description:
 *   Initialize the LCD or framebuffer device (single user mode only), then
 *   open NX.
 *
 ****************************************************************************/

static inline int nximage_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXIMAGE_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nximage_initialize: sched_setparam failed: %d\n" , ret);
      return ERROR;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nximage_initialize: Failed to start the NX server: %d\n", errno);
      return ERROR;
    }

  /* Connect to the server */

  g_nximage.hnx = nx_connect();
  if (g_nximage.hnx)
    {
       pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

      ret = vnc_default_fbinitialize(0, g_nximage.hnx);
      if (ret < 0)
        {
          printf("vnc_default_fbinitialize failed: %d\n", ret);
          nx_disconnect(g_nximage.hnx);
          return ERROR;
        }
#endif
       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       (void)pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXIMAGE_LISTENERPRIO;
       (void)pthread_attr_setschedparam(&attr, &param);
       (void)pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXIMAGE_LISTENER_STACKSIZE);

       ret = pthread_create(&thread, &attr, nximage_listener, NULL);
       if (ret != 0)
         {
            printf("nximage_initialize: pthread_create failed: %d\n", ret);
            return ERROR;
         }

       /* Don't return until we are connected to the server */

       while (!g_nximage.connected)
         {
           /* Wait for the listener thread to wake us up when we really
            * are connected.
            */

           (void)sem_wait(&g_nximage.eventsem);
         }
    }
  else
    {
      printf("nximage_initialize: nx_connect failed: %d\n", errno);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nximage_main
 *
 * Description:
 *   Main entry pointer.  Configures the basic display resources.
 *
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int nximage_main(int argc, char *argv[])
#endif
{
  nxgl_mxpixel_t color;
  int ret;

  /* Initialize NX */

  ret = nximage_initialize();
  printf("nximage_main: NX handle=%p\n", g_nximage.hnx);
  if (!g_nximage.hnx || ret < 0)
    {
      printf("nximage_main: Failed to get NX handle: %d\n", errno);
      g_nximage.code = NXEXIT_INIT;
      goto errout;
    }

  /* Set the background to the configured background color */

  color =  nximage_bgcolor();
  printf("nximage_main: Set background color=%u\n", color);

  ret = nx_setbgcolor(g_nximage.hnx, &color);
  if (ret < 0)
    {
      printf("nximage_main: nx_setbgcolor failed: %d\n", errno);
      g_nximage.code = NXEXIT_NXSETBGCOLOR;
      goto errout_with_nx;
    }

  /* Get the background window */

  ret = nx_requestbkgd(g_nximage.hnx, &g_nximagecb, NULL);
  if (ret < 0)
    {
      printf("nximage_main: nx_setbgcolor failed: %d\n", errno);
      g_nximage.code = NXEXIT_NXREQUESTBKGD;
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution. */

  while (!g_nximage.havepos)
    {
      (void)sem_wait(&g_nximage.eventsem);
    }

  printf("nximage_main: Screen resolution (%d,%d)\n", g_nximage.xres, g_nximage.yres);

  /* Now, put up the NuttX logo and wait a bit so that it visible. */

  nximage_image(g_nximage.hbkgd);
  sleep(5);

  /* Release background */

  (void)nx_releasebkgd(g_nximage.hbkgd);

  /* Close NX */

errout_with_nx:
  printf("nximage_main: Disconnect from the server\n");
  nx_disconnect(g_nximage.hnx);

errout:
  return g_nximage.code;
}
