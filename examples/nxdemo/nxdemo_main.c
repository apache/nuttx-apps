/****************************************************************************
 * examples/nxdemo/nxdemo_main.c
 *
 *   Copyright (C) 2018-2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Cherciu Mihail <m_cherciu@yahoo.com>
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

#ifdef CONFIG_NX_LCDDRIVER
#  include <nuttx/lcd/lcd.h>
#else
#  include <nuttx/video/fb.h>
#endif

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nxdemo.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/
/* If not specified, assume that the hardware supports one video plane */

#ifndef CONFIG_EXAMPLES_NXDEMO_VPLANE
#  define CONFIG_EXAMPLES_NXDEMO_VPLANE 0
#endif

/* If not specified, assume that the hardware supports one LCD device */

#ifndef CONFIG_EXAMPLES_NXDEMO_DEVNO
#  define CONFIG_EXAMPLES_NXDEMO_DEVNO 0
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct nxdemo_data_s g_nxdemo =
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
 * Name: nxdemo_initialize
 ****************************************************************************/

static inline int nxdemo_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXDEMO_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxdemo_initialize: sched_setparam failed: %d\n" , ret);
      return ERROR;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nxdemo_initialize: Failed to start the NX server: %d\n", errno);
      return ERROR;
    }

  /* Connect to the server */

  g_nxdemo.hnx = nx_connect();
  if (g_nxdemo.hnx)
    {
       pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

       struct boardioc_vncstart_s vnc =
       {
         0,  g_nxdemo.hnx
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
           nx_disconnect(g_nxdemo.hnx);
           return ERROR;
         }
#endif

       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXDEMO_LISTENERPRIO;
       pthread_attr_setschedparam(&attr, &param);
       pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXDEMO_LISTENER_STACKSIZE);

       ret = pthread_create(&thread, &attr, nxdemo_listener, NULL);
       if (ret != 0)
         {
            printf("nxdemo_initialize: pthread_create failed: %d\n", ret);
            return ERROR;
         }

       /* Don't return until we are connected to the server */

       while (!g_nxdemo.connected)
         {
           /* Wait for the listener thread to wake us up when we really
            * are connected.
            */

           sem_wait(&g_nxdemo.eventsem);
         }
    }
  else
    {
      printf("nxdemo_initialize: nx_connect failed: %d\n", errno);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxdemo_main
 *
 * Description:
 *   Main entry pointer.  Configures the basic display resources.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  /* Initialize NX */

  ret = nxdemo_initialize();
  printf("nxdemo_main: NX handle=%p\n", g_nxdemo.hnx);
  if (!g_nxdemo.hnx || ret < 0)
    {
      printf("nxdemo_main: Failed to get NX handle: %d\n", errno);
      g_nxdemo.code = NXEXIT_EXTINITIALIZE;
      goto errout;
    }

  /* Get the background window */

  ret = nx_requestbkgd(g_nxdemo.hnx, &g_nxdemocb, NULL);
  if (ret < 0)
    {
      printf("nxdemo_main: nx_setbgcolor failed: %d\n", errno);
      g_nxdemo.code = NXEXIT_NXREQUESTBKGD;
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution. */

  while (!g_nxdemo.havepos)
    {
      sem_wait(&g_nxdemo.eventsem);
    }

  printf("nxdemo_main: Screen resolution (%d,%d)\n",
         g_nxdemo.xres, g_nxdemo.yres);

  nxdemo_hello(g_nxdemo.hbkgd);

  /* Release background */

  nx_releasebkgd(g_nxdemo.hbkgd);

  /* Close NX */

errout_with_nx:
  printf("nxdemo_main: Disconnect from the server\n");
  nx_disconnect(g_nxdemo.hnx);

errout:
  return g_nxdemo.code;
}
