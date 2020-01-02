/****************************************************************************
 * examples/nxlines/nxlines_main.c
 *
 *   Copyright (C) 2011-2012, 2015-2017, 2019 Gregory Nutt. All rights
 *     reserved.
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
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#include "nxlines.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/
/* If not specified, assume that the hardware supports one video plane */

#ifndef CONFIG_EXAMPLES_NXLINES_VPLANE
#  define CONFIG_EXAMPLES_NXLINES_VPLANE 0
#endif

/* If not specified, assume that the hardware supports one LCD device */

#ifndef CONFIG_EXAMPLES_NXLINES_DEVNO
#  define CONFIG_EXAMPLES_NXLINES_DEVNO 0
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct nxlines_data_s g_nxlines =
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
 * Name: nxlines_initialize
 ****************************************************************************/

static inline int nxlines_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXLINES_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxlines_initialize: sched_setparam failed: %d\n" , ret);
      return ERROR;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nxlines_initialize: Failed to start the NX server: %d\n", errno);
      return ERROR;
    }

  /* Connect to the server */

  g_nxlines.hnx = nx_connect();
  if (g_nxlines.hnx)
    {
       pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

       struct boardioc_vncstart_s vnc =
       {
         0, g_nxlines.hnx
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
           nx_disconnect(g_nxlines.hnx);
           return ERROR;
         }
#endif

       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXLINES_LISTENERPRIO;
       pthread_attr_setschedparam(&attr, &param);
       pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXLINES_LISTENER_STACKSIZE);

       ret = pthread_create(&thread, &attr, nxlines_listener, NULL);
       if (ret != 0)
         {
            printf("nxlines_initialize: pthread_create failed: %d\n", ret);
            return ERROR;
         }

       /* Don't return until we are connected to the server */

       while (!g_nxlines.connected)
         {
           /* Wait for the listener thread to wake us up when we really
            * are connected.
            */

           sem_wait(&g_nxlines.eventsem);
         }
    }
  else
    {
      printf("nxlines_initialize: nx_connect failed: %d\n", errno);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxlines_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  nxgl_mxpixel_t color;
  int ret;

  /* Initialize NX */

  ret = nxlines_initialize();
  printf("nxlines_main: NX handle=%p\n", g_nxlines.hnx);
  if (!g_nxlines.hnx || ret < 0)
    {
      printf("nxlines_main: Failed to get NX handle: %d\n", errno);
      g_nxlines.code = NXEXIT_INIT;
      goto errout;
    }

  /* Set the background to the configured background color */

  printf("nxlines_main: Set background color=%d\n",
         CONFIG_EXAMPLES_NXLINES_BGCOLOR);

  color = CONFIG_EXAMPLES_NXLINES_BGCOLOR;
  ret = nx_setbgcolor(g_nxlines.hnx, &color);
  if (ret < 0)
    {
      printf("nxlines_main: nx_setbgcolor failed: %d\n", errno);
      g_nxlines.code = NXEXIT_NXSETBGCOLOR;
      goto errout_with_nx;
    }

  /* Get the background window */

  ret = nx_requestbkgd(g_nxlines.hnx, &g_nxlinescb, NULL);
  if (ret < 0)
    {
      printf("nxlines_main: nx_setbgcolor failed: %d\n", errno);
      g_nxlines.code = NXEXIT_NXREQUESTBKGD;
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution.  */

  while (!g_nxlines.havepos)
    {
      sem_wait(&g_nxlines.eventsem);
    }

  printf("nxlines_main: Screen resolution (%d,%d)\n", g_nxlines.xres, g_nxlines.yres);

  /* Now, say perform the lines (these test does not return so the remaining
   * logic is cosmetic).
   */

  nxlines_test(g_nxlines.hbkgd);

  /* Release background */

  nx_releasebkgd(g_nxlines.hbkgd);

  /* Close NX */

errout_with_nx:
  printf("nxhello_main: Disconnect from the server\n");
  nx_disconnect(g_nxlines.hnx);

errout:
  return g_nxlines.code;
}
