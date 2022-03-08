/****************************************************************************
 * apps/examples/nxdemo/nxdemo_main.c
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
      printf("nxdemo_initialize: "
             "Failed to start the NX server: %d\n", errno);
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

      /* Start a separate thread to listen for server events.
       * This is probably the least efficient way to do this,
       * but it makes this example flow more smoothly.
       */

      pthread_attr_init(&attr);
      param.sched_priority = CONFIG_EXAMPLES_NXDEMO_LISTENERPRIO;
      pthread_attr_setschedparam(&attr, &param);
      pthread_attr_setstacksize(&attr,
                                CONFIG_EXAMPLES_NXDEMO_LISTENER_STACKSIZE);

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
