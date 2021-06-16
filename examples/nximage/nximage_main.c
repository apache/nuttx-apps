/****************************************************************************
 * apps/examples/nximage/nximage_main.c
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

       struct boardioc_vncstart_s vnc =
       {
         0, g_nximage.hnx
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
           nx_disconnect(g_nximage.hnx);
           return ERROR;
         }
#endif

       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXIMAGE_LISTENERPRIO;
       pthread_attr_setschedparam(&attr, &param);
       pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXIMAGE_LISTENER_STACKSIZE);

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

           sem_wait(&g_nximage.eventsem);
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

int main(int argc, FAR char *argv[])
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
      sem_wait(&g_nximage.eventsem);
    }

  printf("nximage_main: Screen resolution (%d,%d)\n", g_nximage.xres, g_nximage.yres);

  /* Now, put up the NuttX logo and wait a bit so that it visible. */

  nximage_image(g_nximage.hbkgd);
  sleep(5);

  /* Release background */

  nx_releasebkgd(g_nximage.hbkgd);

  /* Close NX */

errout_with_nx:
  printf("nximage_main: Disconnect from the server\n");
  nx_disconnect(g_nximage.hnx);

errout:
  return g_nximage.code;
}
