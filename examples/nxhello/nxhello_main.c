/****************************************************************************
 * apps/examples/nxhello/nxhello_main.c
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

#include "nxhello.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct nxhello_data_s g_nxhello =
{
  NULL,               /* hnx */
  NULL,               /* hbkgd */
  NULL,               /* hfont */
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
 * Name: nxhello_initialize
 ****************************************************************************/

static inline int nxhello_initialize(void)
{
  struct sched_param param;
  pthread_t thread;
  int ret;

  /* Set the client task priority */

  param.sched_priority = CONFIG_EXAMPLES_NXHELLO_CLIENTPRIO;
  ret = sched_setparam(0, &param);
  if (ret < 0)
    {
      printf("nxhello_initialize: sched_setparam failed: %d\n" , ret);
      return ERROR;
    }

  /* Start the NX server kernel thread */

  ret = boardctl(BOARDIOC_NX_START, 0);
  if (ret < 0)
    {
      printf("nxhello_initialize: Failed to start the NX server: %d\n", errno);
      return ERROR;
    }

  /* Connect to the server */

  g_nxhello.hnx = nx_connect();
  if (g_nxhello.hnx)
    {
       pthread_attr_t attr;

#ifdef CONFIG_VNCSERVER
      /* Setup the VNC server to support keyboard/mouse inputs */

       struct boardioc_vncstart_s vnc =
       {
         0, g_nxhello.hnx
       };

       ret = boardctl(BOARDIOC_VNC_START, (uintptr_t)&vnc);
       if (ret < 0)
         {
           printf("boardctl(BOARDIOC_VNC_START) failed: %d\n", ret);
           nx_disconnect(g_nxhello.hnx);
           return ERROR;
         }
#endif

       /* Start a separate thread to listen for server events.  This is probably
        * the least efficient way to do this, but it makes this example flow more
        * smoothly.
        */

       pthread_attr_init(&attr);
       param.sched_priority = CONFIG_EXAMPLES_NXHELLO_LISTENERPRIO;
       pthread_attr_setschedparam(&attr, &param);
       pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_NXHELLO_LISTENER_STACKSIZE);

       ret = pthread_create(&thread, &attr, nxhello_listener, NULL);
       if (ret != 0)
         {
            printf("nxhello_initialize: pthread_create failed: %d\n", ret);
            return ERROR;
         }

       /* Don't return until we are connected to the server */

       while (!g_nxhello.connected)
         {
           /* Wait for the listener thread to wake us up when we really
            * are connected.
            */

           sem_wait(&g_nxhello.eventsem);
         }
    }
  else
    {
      printf("nxhello_initialize: nx_connect failed: %d\n", errno);
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxhello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  nxgl_mxpixel_t color;
  int ret;

  /* Initialize NX */

  ret = nxhello_initialize();
  printf("nxhello_main: NX handle=%p\n", g_nxhello.hnx);
  if (!g_nxhello.hnx || ret < 0)
    {
      printf("nxhello_main: Failed to get NX handle: %d\n", errno);
      g_nxhello.code = NXEXIT_INIT;
      goto errout;
    }

  /* Get the default font handle */

  g_nxhello.hfont = nxf_getfonthandle(CONFIG_EXAMPLES_NXHELLO_FONTID);
  if (!g_nxhello.hfont)
    {
      printf("nxhello_main: Failed to get font handle: %d\n", errno);
      g_nxhello.code = NXEXIT_FONTOPEN;
      goto errout;
    }

  /* Set the background to the configured background color */

  printf("nxhello_main: Set background color=%d\n",
         CONFIG_EXAMPLES_NXHELLO_BGCOLOR);

  color = CONFIG_EXAMPLES_NXHELLO_BGCOLOR;
  ret = nx_setbgcolor(g_nxhello.hnx, &color);
  if (ret < 0)
    {
      printf("nxhello_main: nx_setbgcolor failed: %d\n", errno);
      g_nxhello.code = NXEXIT_NXSETBGCOLOR;
      goto errout_with_nx;
    }

  /* Get the background window */

  ret = nx_requestbkgd(g_nxhello.hnx, &g_nxhellocb, NULL);
  if (ret < 0)
    {
      printf("nxhello_main: nx_requestbkgd() failed: %d\n", errno);
      g_nxhello.code = NXEXIT_NXREQUESTBKGD;
      goto errout_with_nx;
    }

  /* Wait until we have the screen resolution. */

  while (!g_nxhello.havepos)
    {
      sem_wait(&g_nxhello.eventsem);
    }

  printf("nxhello_main: Screen resolution (%d,%d)\n", g_nxhello.xres, g_nxhello.yres);

  /* Now, say hello and exit, sleeping a little before each. */

  sleep(1);
  nxhello_hello(g_nxhello.hbkgd);
  sleep(5);

  /* Release background */

  nx_releasebkgd(g_nxhello.hbkgd);

  /* Close NX */

errout_with_nx:
  printf("nxhello_main: Disconnect from the server\n");
  nx_disconnect(g_nxhello.hnx);

errout:
  return g_nxhello.code;
}
